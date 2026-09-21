#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#include <dlfcn.h>
#include "SpotifyDesktopController.h"

namespace
{
constexpr auto requestName = "com.refmatch.control.request.v1";
constexpr auto replyName = "com.refmatch.control.reply.v1";
juce::String utf8(NSString* s) { return s ? juce::String::fromUTF8(s.UTF8String) : juce::String(); }
void moduleAnchor() {}
NSURL* helperURL()
{
    // Resolve this plugin binary, not NSBundle.mainBundle (which is Logic).
    Dl_info image {};
    if (dladdr(reinterpret_cast<const void*>(&moduleAnchor), &image) && image.dli_fname)
    {
        NSString* binary = [NSString stringWithUTF8String:image.dli_fname];
        NSString* contents = [[binary stringByDeletingLastPathComponent] stringByDeletingLastPathComponent];
        NSString* path = [contents stringByAppendingPathComponent:@"Resources/RefMatch Control.app"];
        if ([[NSFileManager defaultManager] fileExistsAtPath:path]) return [NSURL fileURLWithPath:path];
    }
    return nil;
}
}

struct SpotifyDesktopController::Impl
{
    id observer = nil;
    NSString* requestID = nil;
    Completion completion;
    bool active = true;
    ~Impl() { clearObserver(); }
    void clearObserver()
    {
        if (observer) [[NSDistributedNotificationCenter defaultCenter] removeObserver:observer];
        observer = nil;
    }
    void finish(bool ok, SpotifyDesktopInfo info, const juce::String& error)
    {
        clearObserver();
        requestID = nil;
        auto cb = std::move(completion);
        completion = {};
        if (active && cb) cb(ok, std::move(info), error);
    }
};

SpotifyDesktopController::SpotifyDesktopController() : impl(std::make_shared<Impl>()) {}
SpotifyDesktopController::~SpotifyDesktopController()
{
    impl->active = false;
    impl->completion = {};
    impl->clearObserver();
}
bool SpotifyDesktopController::isBusy() const { return impl->completion != nullptr; }

void SpotifyDesktopController::request(Command command, Completion completion)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    if (isBusy()) { completion(false, {}, "Spotify is busy. Try again shortly."); return; }
    NSURL* url = helperURL();
    if (!url) { completion(false, {}, "RefMatch Control is missing. Reinstall the complete plugin bundle."); return; }
    const char* commands[] = { "authorise", "status", "play", "pause", "playPause", "next", "previous" };
    NSString* operation = [NSString stringWithUTF8String:commands[static_cast<int>(command)]];
    auto p = impl;
    p->completion = std::move(completion);
    NSString* identifier = NSUUID.UUID.UUIDString;
    p->requestID = identifier;
    std::weak_ptr<Impl> weak = p;
    p->observer = [[NSDistributedNotificationCenter defaultCenter]
        addObserverForName:[NSString stringWithUTF8String:replyName] object:identifier
        queue:NSOperationQueue.mainQueue usingBlock:^(NSNotification* note)
    {
        auto state = weak.lock();
        if (!state || !state->active || ![state->requestID isEqualToString:identifier]) return;
        NSDictionary* data = note.userInfo;
        SpotifyDesktopInfo info;
        info.installed = [data[@"installed"] boolValue];
        info.running = [data[@"running"] boolValue];
        info.playing = [data[@"playing"] boolValue];
        info.authorised = [data[@"authorised"] boolValue];
        info.track = utf8(data[@"track"]);
        info.artist = utf8(data[@"artist"]);
        info.durationSeconds = [data[@"duration"] doubleValue];
        info.positionSeconds = [data[@"position"] doubleValue];
        state->finish([data[@"ok"] boolValue], info, utf8(data[@"error"]));
    }];
    auto* config = [NSWorkspaceOpenConfiguration configuration];
    config.activates = NO;
    [[NSWorkspace sharedWorkspace] openApplicationAtURL:url configuration:config
        completionHandler:^(NSRunningApplication* app, NSError* error)
    {
        dispatch_async(dispatch_get_main_queue(), ^{
            auto state = weak.lock();
            if (!state || !state->active || ![state->requestID isEqualToString:identifier]) return;
            if (!app || error)
            {
                state->finish(false, {}, "Could not open RefMatch Control: " + utf8(error.localizedDescription));
                return;
            }
            // A PID target prevents different installed copies executing twice.
            NSDictionary* data = @{ @"command": operation, @"pid": @(app.processIdentifier) };
            [[NSDistributedNotificationCenter defaultCenter]
                postNotificationName:[NSString stringWithUTF8String:requestName]
                object:identifier userInfo:data deliverImmediately:YES];
        });
    }];
    const int seconds = command == Command::authorise ? 90 : 12;
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)seconds * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
        auto state = weak.lock();
        if (state && state->active && [state->requestID isEqualToString:identifier])
            state->finish(false, {}, "RefMatch Control did not respond. Check the macOS permission dialog, then retry.");
    });
}

bool SpotifyDesktopController::openSearch(const juce::String& query, juce::String& error)
{
    if (query.trim().isEmpty()) { error = "Type a Spotify search first."; return false; }
    NSString* text = [NSString stringWithUTF8String:query.trim().toRawUTF8()];
    NSCharacterSet* allowed = [NSCharacterSet characterSetWithCharactersInString:@"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-._~"];
    NSString* escaped = [text stringByAddingPercentEncodingWithAllowedCharacters:allowed];
    NSURL* url = [NSURL URLWithString:[@"spotify:search:" stringByAppendingString:escaped]];
    if (!url || ![[NSWorkspace sharedWorkspace] openURL:url])
    { error = "Could not open Spotify. Install and open Spotify desktop first."; return false; }
    error.clear();
    return true;
}

void SpotifyDesktopController::openAutomationSettings()
{
    [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"x-apple.systempreferences:com.apple.preference.security?Privacy_Automation"]];
}
