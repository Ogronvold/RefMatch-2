#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#import <Carbon/Carbon.h>
#include "SpotifyDesktopController.h"

namespace {
dispatch_queue_t controlQueue()
{
    static dispatch_queue_t queue = dispatch_queue_create("com.refmatch.spotify.transport", DISPATCH_QUEUE_SERIAL);
    return queue;
}
// Codes from Spotify.app/Contents/Resources/Spotify.sdef. No script compilation,
// dictionary fetch, or com.spotify.library metadata request is needed.
NSAppleEventDescriptor* playerProperty()
{
    auto* record = [NSAppleEventDescriptor recordDescriptor];
    [record setDescriptor:[NSAppleEventDescriptor descriptorWithTypeCode:cProperty] forKeyword:keyAEDesiredClass];
    [record setDescriptor:[NSAppleEventDescriptor descriptorWithEnumCode:formPropertyID] forKeyword:keyAEKeyForm];
    [record setDescriptor:[NSAppleEventDescriptor descriptorWithTypeCode:'pPlS'] forKeyword:keyAEKeyData];
    [record setDescriptor:[NSAppleEventDescriptor nullDescriptor] forKeyword:keyAEContainer];
    return [record coerceToDescriptorType:typeObjectSpecifier];
}
OSStatus send(AEEventClass eventClass, AEEventID eventID, bool ask,
              NSAppleEventDescriptor* direct, NSAppleEventDescriptor** result)
{
    auto* target = [NSAppleEventDescriptor descriptorWithBundleIdentifier:@"com.spotify.client"];
    if (!target) return paramErr;
    OSStatus status = AEDeterminePermissionToAutomateTarget(target.aeDesc, eventClass, eventID, ask);
    if (status != noErr) return status;
    auto* event = [NSAppleEventDescriptor appleEventWithEventClass:eventClass eventID:eventID
        targetDescriptor:target returnID:kAutoGenerateReturnID transactionID:kAnyTransactionID];
    if (direct) [event setParamDescriptor:direct forKeyword:keyDirectObject];
    AppleEvent reply = { typeNull, nullptr };
    status = AESendMessage(event.aeDesc, &reply, kAEWaitReply | kAENeverInteract | kAEDontRecord, 180);
    if (status != noErr) { AEDisposeDesc(&reply); return status; }
    auto* response = [[NSAppleEventDescriptor alloc] initWithAEDescNoCopy:&reply];
    const auto replyError = [response paramDescriptorForKeyword:keyErrorNumber];
    if (replyError && replyError.int32Value != 0) return replyError.int32Value;
    if (result) *result = [response paramDescriptorForKeyword:keyDirectObject];
    return noErr;
}
juce::String failure(OSStatus status)
{
    return "Spotify transport failed (" + juce::String((int)status)
        + "). Check host Automation permission. MIX restored.";
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
    const bool canPrompt = [[NSBundle.mainBundle objectForInfoDictionaryKey:@"NSAppleEventsUsageDescription"] length] > 0;
    dispatch_async(controlQueue(), ^{
        @autoreleasepool {
            if (!state->active.load()) { state->busy.store(false); return; }
            SpotifyDesktopInfo info;
            info.installed = [NSWorkspace.sharedWorkspace URLForApplicationWithBundleIdentifier:@"com.spotify.client"] != nil;
            info.running = [NSRunningApplication runningApplicationsWithBundleIdentifier:@"com.spotify.client"].count > 0;
            OSStatus status = noErr;
            juce::String error;
            if (!info.running) { status = procNotFound; error = "Open Spotify and choose a track first."; }
            else {
                const AEEventID operations[] = { 0, 0, 'Play', 'Paus', 'PlPs', 'Next', 'Prev' };
                const auto operation = operations[static_cast<int>(command)];
                if (operation) status = send('spfy', operation, false, nil, nullptr);
                // Confirm the actual playback state; tags are never a prerequisite.
                NSAppleEventDescriptor* reply = nil;
                if (status == noErr) status = send(kAECoreSuite, kAEGetData,
                    command == Command::authorise && canPrompt, playerProperty(), &reply);
                if (status == noErr) {
                    const auto value = reply.enumCodeValue;
                    if (value != 'kPSP' && value != 'kPSp' && value != 'kPSS') status = errAECoercionFail;
                    else {
                        info.playing = value == 'kPSP';
                        info.authorised = true;
                        info.track = "Spotify transport connected";
                        info.artist = "Choose your reference in Spotify";
                        if ((command == Command::play && !info.playing)
                            || (command == Command::pause && info.playing)) {
                            status = errAEEventFailed;
                            error = "Spotify did not confirm the requested playback state. MIX restored.";
                        }
                    }
                }
                // PLAY may have happened before verification failed: attempt PAUSE
                // before reporting failure. Never claim rollback succeeded if denied.
                if (status != noErr && command == Command::play) {
                    if (send('spfy', 'Paus', false, nil, nullptr) != noErr)
                        error = "Spotify state uncertain. Pause Spotify manually; MIX restored.";
                }
            }
            if (status != noErr && error.isEmpty()) error = failure(status);
            if (status != noErr) info.authorised = false;
            juce::MessageManager::callAsync([state, completion, status, info, error] {
                state->busy.store(false);
                if (state->active.load()) completion(status == noErr, info, error);
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
