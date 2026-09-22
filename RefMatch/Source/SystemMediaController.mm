#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#include <dlfcn.h>
#include "SystemMediaController.h"
#include "PlaybackFeedback.h"

namespace {
using SendCommand = Boolean (*)(int, NSDictionary*);
SendCommand resolveSendCommand()
{
    // Private API: dynamically resolve and fail safely if Apple removes it.
    // Keep the handle alive; no helper, subprocess or copied framework.
    static void* handle = dlopen("/System/Library/PrivateFrameworks/MediaRemote.framework/MediaRemote", RTLD_LAZY | RTLD_LOCAL);
    static auto send = handle ? reinterpret_cast<SendCommand>(dlsym(handle, "MRMediaRemoteSendCommand")) : nullptr;
    return send;
}
dispatch_queue_t mediaQueue()
{
    static dispatch_queue_t queue = dispatch_queue_create("com.refmatch.system.media", DISPATCH_QUEUE_SERIAL);
    return queue;
}
}
struct SystemMediaController::Impl {
    std::atomic<bool> active { true }, busy { false };
    PlaybackFeedback playback;
    NSUInteger artworkHash=0;
    bool artworkRead=false;
    juce::String artworkTrack;
    juce::Image artwork;

};
SystemMediaController::SystemMediaController() : impl(std::make_shared<Impl>()) {}
SystemMediaController::~SystemMediaController() { impl->active.store(false); }
bool SystemMediaController::isBusy() const { return impl->busy.load(); }
int SystemMediaController::playbackState() const {return impl->playback.state(juce::Time::getMillisecondCounterHiRes());}
bool SystemMediaController::playbackPending() const {return impl->playback.pending(juce::Time::getMillisecondCounterHiRes());}
void SystemMediaController::request(Command command, Completion completion)
{
    if (impl->busy.exchange(true)) { completion(false, {}, "Media command pending."); return; }
    const auto now=juce::Time::getMillisecondCounterHiRes();
    if(command==Command::play || command==Command::pause || command==Command::playPause)
        impl->playback.request(command==Command::play || (command==Command::playPause && playbackState()!=1),now);
    auto state = impl;
    dispatch_async(mediaQueue(), ^{
        @autoreleasepool {
            if (!state->active.load()) { state->busy.store(false); return; }
            const auto send = resolveSendCommand();
            bool ok = send != nullptr;
            // Separate PLAY and PAUSE, never toggle in the A/B sequence.
            const int commands[] = { -1, -1, 0, 1, 2, 4, 5 };
            const int operation = commands[static_cast<int>(command)];
            if (ok && operation >= 0) ok = send(operation, nil) != 0;
            SystemMediaInfo info;
            info.authorised = send != nullptr; // Capability only, not player-state verification.
            info.track = "System media source";
            info.artist = "Spotify, Music or browser";
            juce::String error;
            if (!send) error = "System media control unavailable on this macOS. MIX restored.";
            else if (!ok) error = "System rejected the media command. MIX restored.";
            // The return value is command acceptance, NOT proof of playback.
            // Never make success depend on restricted now-playing metadata.
            juce::MessageManager::callAsync([state, completion, ok, info, error] {
                state->busy.store(false);
                if(!ok)state->playback.failed();
                if (state->active.load()) completion(ok, info, error);
            });
        }
    });
}
bool SystemMediaController::openSearch(const juce::String& query, juce::String& error)
{
    if (query.trim().isEmpty()) { error = "Type a Spotify search first."; return false; }
    NSString* text = [NSString stringWithUTF8String:query.trim().toRawUTF8()];
    NSCharacterSet* allowed = [NSCharacterSet characterSetWithCharactersInString:@"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-._~"];
    NSString* escaped = [text stringByAddingPercentEncodingWithAllowedCharacters:allowed];
    NSURL* url = [NSURL URLWithString:[@"spotify:search:" stringByAppendingString:escaped]];
    if (!url || ![NSWorkspace.sharedWorkspace openURL:url]) { error = "Could not open Spotify."; return false; }
    error.clear(); return true;
}
void SystemMediaController::openAutomationSettings()
{
    [NSWorkspace.sharedWorkspace openURL:[NSURL URLWithString:@"x-apple.systempreferences:com.apple.preference.security?Privacy_ScreenCapture"]];
}

void SystemMediaController::readPosition(PositionCompletion completion)
{
    // Metadata is optional and never shares the transport busy flag.
    using ReadInfo = void (*)(dispatch_queue_t, void (^)(CFDictionaryRef));
    using ReadPlaying = void (*)(dispatch_queue_t, void (^)(Boolean));
    static void* handle = dlopen("/System/Library/PrivateFrameworks/MediaRemote.framework/MediaRemote", RTLD_LAZY | RTLD_LOCAL);
    static auto read = handle ? reinterpret_cast<ReadInfo>(dlsym(handle,"MRMediaRemoteGetNowPlayingInfo")) : nullptr;
    static auto readPlaying=handle ? reinterpret_cast<ReadPlaying>(dlsym(handle,"MRMediaRemoteGetNowPlayingApplicationIsPlaying")) : nullptr;
    auto state=impl;
    if(readPlaying)readPlaying(dispatch_get_main_queue(), ^(Boolean playing) {
        if(state->active.load())state->playback.report(playing?1:0,juce::Time::getMillisecondCounterHiRes());
    });
    auto delivered=std::make_shared<std::atomic<bool>>(false);
    auto deliver=[state,delivered,completion](MediaPosition position) {
        if(!delivered->exchange(true) && state->active.load()) {
            if(!readPlaying && position.playbackKnown)state->playback.report(position.playbackKnown?(position.playing?1:0):-1,juce::Time::getMillisecondCounterHiRes());
            completion(position);
        }
    };
    if(!read) {deliver({});return;}
    read(dispatch_get_main_queue(), ^(CFDictionaryRef raw) {
        if(delivered->load() || !state->active.load())return;
        MediaPosition position;
        if(raw) {
            NSDictionary* info=(__bridge NSDictionary*)raw;
            auto get=[&](const char* name)->id {
                const auto address=reinterpret_cast<CFStringRef*>(dlsym(handle,name));
                return address && *address ? info[(__bridge NSString*)*address] : nil;
            };
            id elapsed=get("kMRMediaRemoteNowPlayingInfoElapsedTime");
            id duration=get("kMRMediaRemoteNowPlayingInfoDuration");
            id rate=get("kMRMediaRemoteNowPlayingInfoPlaybackRate");
            id stamp=get("kMRMediaRemoteNowPlayingInfoTimestamp");
            id title=get("kMRMediaRemoteNowPlayingInfoTitle");
            id artist=get("kMRMediaRemoteNowPlayingInfoArtist");
            id identifier=get("kMRMediaRemoteNowPlayingInfoUniqueIdentifier");
            if([title isKindOfClass:NSString.class])position.title=juce::String::fromUTF8([title UTF8String]);
            if([artist isKindOfClass:NSString.class])position.artist=juce::String::fromUTF8([artist UTF8String]);
            position.playbackKnown=[rate respondsToSelector:@selector(doubleValue)];
            position.playing=position.playbackKnown && [rate doubleValue]>0;
            // Prefer content identity: session identifiers can change while the same song plays.
            NSString* identity=([title isKindOfClass:NSString.class] && [title length]>0)
                ? [NSString stringWithFormat:@"%@|%@",title,artist ?: @""]
                : (identifier ? [identifier description] : @"");
            position.track=juce::String::fromUTF8(identity.UTF8String);
            if(state->artworkTrack!=position.track) {
                state->artwork={};state->artworkHash=0;state->artworkRead=false;state->artworkTrack=position.track;
            }
            NSData* imageData=get("kMRMediaRemoteNowPlayingInfoArtworkData");
            if([imageData isKindOfClass:NSData.class] && [imageData length]>0 && [imageData length]<10*1024*1024) {
                const auto hash=[imageData hash];
                if(!state->artworkRead || state->artworkHash!=hash) {
                    state->artwork=juce::ImageFileFormat::loadFrom([imageData bytes],size_t([imageData length]));
                    state->artworkHash=hash;state->artworkRead=true;
                }
            }
            position.artwork=state->artwork;
            if([duration respondsToSelector:@selector(doubleValue)])position.duration=[duration doubleValue];
            if(!std::isfinite(position.duration) || position.duration<0)position.duration=0;
            if([elapsed respondsToSelector:@selector(doubleValue)] && [rate respondsToSelector:@selector(doubleValue)]) {
                position.seconds=[elapsed doubleValue];position.playing=[rate doubleValue]>0;
                if([stamp isKindOfClass:NSDate.class] && position.playing)
                    position.seconds+=std::max(0.,std::min(86400.,-[stamp timeIntervalSinceNow]))*[rate doubleValue];
                position.valid=std::isfinite(position.seconds) && position.seconds>=0 && position.track.isNotEmpty();
            }
        }
        deliver(position);
    });
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC),dispatch_get_main_queue(), ^{deliver({});});
}
bool SystemMediaController::seekTo(double seconds)
{
    using Seek = void (*)(double);
    static void* handle = dlopen("/System/Library/PrivateFrameworks/MediaRemote.framework/MediaRemote", RTLD_LAZY | RTLD_LOCAL);
    static auto seek=handle ? reinterpret_cast<Seek>(dlsym(handle,"MRMediaRemoteSetElapsedTime")) : nullptr;
    if(!seek || !std::isfinite(seconds) || seconds<0)return false;
    seek(seconds);return true; // Delivery only; loop verifies subsequent position.
}
