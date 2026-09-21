#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#import <Carbon/Carbon.h>
#include "SpotifyDesktopController.h"

namespace {
juce::String utf8(NSString* value) { return value ? juce::String::fromUTF8(value.UTF8String) : juce::String(); }
// A single serial queue protects the in-process scripting engine across instances.
dispatch_queue_t controlQueue()
{
    static dispatch_queue_t queue = dispatch_queue_create("com.refmatch.optional.spotify", DISPATCH_QUEUE_SERIAL);
    return queue;
}
}
struct SpotifyDesktopController::Impl {
    std::atomic<bool> active { true }, busy { false };
};
SpotifyDesktopController::SpotifyDesktopController() : impl(std::make_shared<Impl>()) {}
SpotifyDesktopController::~SpotifyDesktopController() { impl->active.store(false); }
bool SpotifyDesktopController::isBusy() const { return impl->busy.load(); }
void SpotifyDesktopController::request(Command command, Completion completion)
{
    if (impl->busy.exchange(true)) { completion(false, {}, "Spotify is busy."); return; }
    auto state = impl;
    // Do not prompt from a host missing its own usage description. An AU plist
    // cannot supply this on behalf of Logic/AUHostingService.
    const bool canPrompt = [[NSBundle.mainBundle objectForInfoDictionaryKey:@"NSAppleEventsUsageDescription"] length] > 0;
    dispatch_async(controlQueue(), ^{
        @autoreleasepool {
            SpotifyDesktopInfo info;
            juce::String error;
            info.installed = [NSWorkspace.sharedWorkspace URLForApplicationWithBundleIdentifier:@"com.spotify.client"] != nil;
            info.running = [NSRunningApplication runningApplicationsWithBundleIdentifier:@"com.spotify.client"].count > 0;
            bool ok = false;
            if (!state->active.load()) { state->busy.store(false); return; }
            if (!info.running) error = "Open Spotify first. System REF works independently.";
            else {
                AEAddressDesc target = { typeNull, nullptr };
                const char bundle[] = "com.spotify.client";
                OSStatus status = AECreateDesc(typeApplicationBundleID, bundle, sizeof(bundle) - 1, &target);
                if (status == noErr) {
                    status = AEDeterminePermissionToAutomateTarget(&target, typeWildCard, typeWildCard,
                        command == Command::authorise && canPrompt);
                    AEDisposeDesc(&target);
                }
                info.authorised = status == noErr;
                if (!info.authorised)
                    error = "Host Automation unavailable (" + juce::String((int)status) + "). Use Spotify directly; system REF remains available.";
                else {
                    NSString* operations[] = { @"", @"", @"play", @"pause", @"playpause", @"next track", @"previous track" };
                    NSString* source = [NSString stringWithFormat:
                        @"with timeout of 3 seconds\ntell application id \"com.spotify.client\"\n%@\n"
                         "set s to player state as string\n"
                         "if s is \"stopped\" then return {s, \"\", \"\", 0, 0}\n"
                         "set t to current track\nreturn {s, name of t, artist of t, duration of t, (player position * 1000) as integer}\n"
                         "end tell\nend timeout", operations[static_cast<int>(command)]];
                    NSDictionary* details = nil;
                    NSAppleEventDescriptor* result = [[[NSAppleScript alloc] initWithSource:source] executeAndReturnError:&details];
                    ok = result && result.numberOfItems == 5;
                    if (ok) {
                        info.playing = [[result descriptorAtIndex:1].stringValue isEqualToString:@"playing"];
                        info.track = utf8([result descriptorAtIndex:2].stringValue);
                        info.artist = utf8([result descriptorAtIndex:3].stringValue);
                        info.durationSeconds = [result descriptorAtIndex:4].int32Value / 1000.0;
                        info.positionSeconds = [result descriptorAtIndex:5].int32Value / 1000.0;
                    } else error = "Spotify: " + utf8(details[NSAppleScriptErrorMessage]);
                }
            }
            juce::MessageManager::callAsync([state, completion, ok, info, error] {
                state->busy.store(false);
                if (state->active.load()) completion(ok, info, error);
            });
        }
    });
}
bool SpotifyDesktopController::openSearch(const juce::String& query, juce::String& error)
{
    if (query.trim().isEmpty()) { error = "Type a Spotify search first."; return false; }
    NSString* text = [NSString stringWithUTF8String:query.trim().toRawUTF8()];
    NSCharacterSet* allowed = [NSCharacterSet characterSetWithCharactersInString:@"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-._~"];
    NSString* escaped = [text stringByAddingPercentEncodingWithAllowedCharacters:allowed];
    NSURL* url = [NSURL URLWithString:[@"spotify:search:" stringByAppendingString:escaped]];
    if (!url || ![NSWorkspace.sharedWorkspace openURL:url]) { error = "Could not open Spotify."; return false; }
    error.clear(); return true;
}
void SpotifyDesktopController::openAutomationSettings()
{
    [NSWorkspace.sharedWorkspace openURL:[NSURL URLWithString:@"x-apple.systempreferences:com.apple.preference.security?Privacy_Automation"]];
}
