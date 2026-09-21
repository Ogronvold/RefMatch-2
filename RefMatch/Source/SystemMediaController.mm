#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#include <dlfcn.h>
#include "SystemMediaController.h"

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
};
SystemMediaController::SystemMediaController() : impl(std::make_shared<Impl>()) {}
SystemMediaController::~SystemMediaController() { impl->active.store(false); }
bool SystemMediaController::isBusy() const { return impl->busy.load(); }
void SystemMediaController::request(Command command, Completion completion)
{
    if (impl->busy.exchange(true)) { completion(false, {}, "Media command pending."); return; }
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
