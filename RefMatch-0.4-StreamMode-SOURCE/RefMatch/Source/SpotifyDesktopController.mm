// Keep Apple's global Point/Component declarations ahead of JUCE headers.
#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>

#include "SpotifyDesktopController.h"

namespace
{
static juce::String nsToJuce(NSString* s)
{
    return s != nil ? juce::String([s UTF8String]) : juce::String();
}
}

bool SpotifyDesktopController::runCommand(const juce::String& body, juce::String* result, juce::String& errorMessage)
{
    @autoreleasepool
    {
        NSString* source = [NSString stringWithUTF8String:body.toRawUTF8()];
        NSAppleScript* script = [[NSAppleScript alloc] initWithSource:source];
        NSDictionary* errorInfo = nil;
        NSAppleEventDescriptor* descriptor = [script executeAndReturnError:&errorInfo];
        if (descriptor == nil)
        {
            NSString* message = errorInfo[NSAppleScriptErrorMessage];
            errorMessage = message != nil ? nsToJuce(message) : "Spotify control failed";
            return false;
        }

        if (result != nullptr)
            *result = nsToJuce(descriptor.stringValue);
        errorMessage.clear();
        return true;
    }
}

bool SpotifyDesktopController::requestControlPermission(juce::String& errorMessage)
{
    juce::String ignored;
    return runCommand("tell application \"Spotify\" to get player state as string", &ignored, errorMessage);
}

bool SpotifyDesktopController::play(juce::String& errorMessage)
{
    return runCommand("tell application \"Spotify\" to play", nullptr, errorMessage);
}

bool SpotifyDesktopController::pause(juce::String& errorMessage)
{
    return runCommand("tell application \"Spotify\" to pause", nullptr, errorMessage);
}

bool SpotifyDesktopController::playPause(juce::String& errorMessage)
{
    return runCommand("tell application \"Spotify\" to playpause", nullptr, errorMessage);
}

bool SpotifyDesktopController::next(juce::String& errorMessage)
{
    return runCommand("tell application \"Spotify\" to next track", nullptr, errorMessage);
}

bool SpotifyDesktopController::previous(juce::String& errorMessage)
{
    return runCommand("tell application \"Spotify\" to previous track", nullptr, errorMessage);
}

SpotifyDesktopInfo SpotifyDesktopController::getInfo(juce::String& errorMessage)
{
    SpotifyDesktopInfo info;
    @autoreleasepool
    {
        NSArray<NSRunningApplication*>* apps = [NSRunningApplication runningApplicationsWithBundleIdentifier:@"com.spotify.client"];
        info.installed = [[NSWorkspace sharedWorkspace] URLForApplicationWithBundleIdentifier:@"com.spotify.client"] != nil;
        info.running = apps.count > 0;
    }

    if (!info.running)
    {
        errorMessage.clear();
        return info;
    }

    juce::String result;
    const auto script = juce::String(
        "tell application \"Spotify\"\n"
        "set s to player state as string\n"
        "if s is \"stopped\" then return s & \"||\"\n"
        "set t to current track\n"
        "set n to name of t\n"
        "set a to artist of t\n"
        "set d to duration of t\n"
        "set p to player position\n"
        "return s & \"||\" & n & \"||\" & a & \"||\" & (d as string) & \"||\" & (p as string)\n"
        "end tell");

    if (!runCommand(script, &result, errorMessage))
        return info;

    juce::StringArray parts;
    parts.addTokens(result, "||", "");
    info.playing = parts.size() > 0 && parts[0].trim().equalsIgnoreCase("playing");
    if (parts.size() > 1) info.track = parts[1].trim();
    if (parts.size() > 2) info.artist = parts[2].trim();
    if (parts.size() > 3) info.durationSeconds = parts[3].getDoubleValue() / 1000.0;
    if (parts.size() > 4) info.positionSeconds = parts[4].getDoubleValue();
    return info;
}

bool SpotifyDesktopController::openSearch(const juce::String& query, juce::String& errorMessage)
{
    if (query.trim().isEmpty())
    {
        errorMessage = "Type a Spotify search first";
        return false;
    }

    @autoreleasepool
    {
        const auto escaped = juce::URL::addEscapeChars(query.trim(), true);
        const auto uri = "spotify:search:" + escaped;
        NSURL* url = [NSURL URLWithString:[NSString stringWithUTF8String:uri.toRawUTF8()]];
        if (url == nil || ![[NSWorkspace sharedWorkspace] openURL:url])
        {
            errorMessage = "Could not open Spotify search";
            return false;
        }
    }

    errorMessage.clear();
    return true;
}
