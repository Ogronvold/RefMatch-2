#import <AppKit/AppKit.h>
#import <Carbon/Carbon.h>

static NSString* const requestName = @"com.refmatch.control.request.v1";
static NSString* const replyName = @"com.refmatch.control.reply.v1";
static NSString* const spotifyID = @"com.spotify.client";

static NSString* permissionMessage(OSStatus status)
{
    if (status == errAEEventNotPermitted || status == -10004)
        return @"Allow RefMatch Control to control Spotify in System Settings > Privacy & Security > Automation, then click CONNECT again.";
    if (status == errAEEventWouldRequireUserConsent)
        return @"Click CONNECT SPOTIFY to grant Automation permission.";
    return [NSString stringWithFormat:@"Spotify control failed (%d). Open Spotify and retry CONNECT.", (int)status];
}

static OSStatus permission(BOOL ask)
{
    AEAddressDesc target = { typeNull, nullptr };
    OSStatus status = AECreateDesc(typeApplicationBundleID, spotifyID.UTF8String,
                                  (Size)[spotifyID lengthOfBytesUsingEncoding:NSUTF8StringEncoding], &target);
    if (status == noErr)
    {
        status = AEDeterminePermissionToAutomateTarget(&target, typeWildCard, typeWildCard, ask);
        AEDisposeDesc(&target);
    }
    return status;
}

static NSAppleEventDescriptor* execute(NSString* body, NSString** error, NSInteger* code)
{
    // No incoming script text is executed. All bodies below are fixed commands.
    NSString* source = [NSString stringWithFormat:
        @"with timeout of 5 seconds\n tell application id \"com.spotify.client\"\n%@\nend tell\nend timeout", body];
    NSAppleScript* script = [[NSAppleScript alloc] initWithSource:source];
    NSDictionary* details = nil;
    NSAppleEventDescriptor* result = [script executeAndReturnError:&details];
    if (!result)
    {
        *code = [details[NSAppleScriptErrorNumber] integerValue];
        *error = (*code == -1743 || *code == -10004) ? permissionMessage((OSStatus)*code)
            : [NSString stringWithFormat:@"%@ (%ld)", details[NSAppleScriptErrorMessage] ?: @"Spotify command failed", (long)*code];
    }
    return result;
}

@interface RMControl : NSObject <NSApplicationDelegate>
@property(strong) NSStatusItem* statusItem;
@property(strong) NSMutableDictionary<NSString*, NSDictionary*>* replies;
@property(strong) NSMutableArray<NSString*>* replyOrder;
- (void)receive:(NSNotification*)note;
@end

@implementation RMControl
- (instancetype)init
{
    if ((self = [super init]))
    {
        self.replies = [NSMutableDictionary dictionary];
        self.replyOrder = [NSMutableArray array];
        // Register before LaunchServices signals that app launch completed.
        [[NSDistributedNotificationCenter defaultCenter] addObserver:self selector:@selector(receive:)
            name:requestName object:nil suspensionBehavior:NSNotificationSuspensionBehaviorDeliverImmediately];
    }
    return self;
}
- (void)applicationDidFinishLaunching:(NSNotification*)notification
{
    (void)notification;
    self.statusItem = [[NSStatusBar systemStatusBar] statusItemWithLength:NSVariableStatusItemLength];
    self.statusItem.button.title = @"RM";
    self.statusItem.button.toolTip = @"RefMatch Spotify Control";
    NSMenu* menu = [NSMenu new];
    NSMenuItem* settings = [[NSMenuItem alloc] initWithTitle:@"Open Automation Settings"
        action:@selector(openSettings:) keyEquivalent:@""];
    settings.target = self;
    [menu addItem:settings];
    [menu addItem:NSMenuItem.separatorItem];
    [menu addItemWithTitle:@"Quit RefMatch Control" action:@selector(terminate:) keyEquivalent:@""];
    self.statusItem.menu = menu;
}
- (void)openSettings:(id)sender
{
    (void)sender;
    [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:
        @"x-apple.systempreferences:com.apple.preference.security?Privacy_Automation"]];
}
- (void)reply:(NSString*)identifier data:(NSDictionary*)data
{
    self.replies[identifier] = data;
    [self.replyOrder addObject:identifier];
    if (self.replyOrder.count > 64)
    {
        [self.replies removeObjectForKey:self.replyOrder.firstObject];
        [self.replyOrder removeObjectAtIndex:0];
    }
    [[NSDistributedNotificationCenter defaultCenter] postNotificationName:replyName
        object:identifier userInfo:data deliverImmediately:YES];
}
- (void)receive:(NSNotification*)note
{
    if ([note.userInfo[@"pid"] intValue] != NSProcessInfo.processInfo.processIdentifier) return;
    NSString* identifier = [note.object isKindOfClass:NSString.class] ? note.object : nil;
    NSString* command = note.userInfo[@"command"];
    if (!identifier || identifier.length > 64 || ![command isKindOfClass:NSString.class]) return;
    if (self.replies[identifier])
    {
        [[NSDistributedNotificationCenter defaultCenter] postNotificationName:replyName
            object:identifier userInfo:self.replies[identifier] deliverImmediately:YES];
        return;
    }
    NSSet* allowed = [NSSet setWithArray:@[@"authorise", @"status", @"play", @"pause", @"playPause", @"next", @"previous"]];
    if (![allowed containsObject:command]) return;

    NSMutableDictionary* result = [@{@"ok": @NO, @"authorised": @NO, @"playing": @NO,
        @"track": @"", @"artist": @"", @"duration": @0, @"position": @0} mutableCopy];
    NSURL* spotify = [[NSWorkspace sharedWorkspace] URLForApplicationWithBundleIdentifier:spotifyID];
    result[@"installed"] = @(spotify != nil);
    BOOL running = [NSRunningApplication runningApplicationsWithBundleIdentifier:spotifyID].count > 0;
    result[@"running"] = @(running);
    if (!spotify || !running)
    {
        result[@"error"] = spotify ? @"Open Spotify desktop, choose a track, then click CONNECT."
                                   : @"Install Spotify desktop, then click CONNECT.";
        [self reply:identifier data:result];
        return;
    }
    // Only an explicit CONNECT can prompt. Background status never prompts.
    OSStatus status = permission([command isEqualToString:@"authorise"]);
    if (status != noErr)
    {
        result[@"error"] = permissionMessage(status);
        result[@"code"] = @(status);
        [self reply:identifier data:result];
        return;
    }
    result[@"authorised"] = @YES;
    NSString* error = nil;
    NSInteger code = 0;
    NSDictionary* commands = @{@"play": @"play\nrepeat 10 times\nif player state is playing then exit repeat\ndelay 0.1\nend repeat", @"pause": @"pause",
        @"playPause": @"playpause", @"next": @"next track", @"previous": @"previous track"};
    if (commands[command] && !execute(commands[command], &error, &code))
    {
        result[@"error"] = error;
        result[@"code"] = @(code);
        if (code == -1743 || code == -10004) result[@"authorised"] = @NO;
        [self reply:identifier data:result];
        return;
    }

    // An AppleScript list preserves Unicode, literal "|" and locale-independent
    // integer durations. No delimiter splitting or locale-sensitive doubles.
    NSAppleEventDescriptor* info = execute(
        @"set s to player state as string\n"
         "if s is \"stopped\" then return {s, \"\", \"\", 0, 0}\n"
         "set t to current track\n"
         "return {s, name of t, artist of t, duration of t, (player position * 1000) as integer}",
        &error, &code);
    if (!info || info.numberOfItems != 5)

    {
        // If PLAY happened but verification failed, undo playback before telling
        // the plugin to restore its mix. PAUSE success does not depend on tags.
        if ([command isEqualToString:@"play"])
        {
            NSString* ignored = nil;
            NSInteger ignoredCode = 0;
            execute(@"pause", &ignored, &ignoredCode);
        }
        if ([command isEqualToString:@"pause"])
        {
            result[@"ok"] = @YES;
            [self reply:identifier data:result];
            return;
        }
        result[@"error"] = error ?: @"Spotify returned incomplete track information. Select a track and retry.";

        result[@"code"] = @(code);
        [self reply:identifier data:result];
        return;
    }
    result[@"playing"] = @([[[info descriptorAtIndex:1] stringValue] isEqualToString:@"playing"]);
    result[@"track"] = [info descriptorAtIndex:2].stringValue ?: @"";
    result[@"artist"] = [info descriptorAtIndex:3].stringValue ?: @"";
    result[@"duration"] = @([info descriptorAtIndex:4].int32Value / 1000.0);
    result[@"position"] = @([info descriptorAtIndex:5].int32Value / 1000.0);
    // PLAY must actually reach playing before the plugin stays on reference.
    if ([command isEqualToString:@"play"] && ![result[@"playing"] boolValue])
        result[@"error"] = @"Spotify did not start. Select a playable track, then retry.";
    else
        result[@"ok"] = @YES;
    [self reply:identifier data:result];
}
@end

int main()
{
    @autoreleasepool
    {
        NSApplication* app = NSApplication.sharedApplication;
        RMControl* delegate = [RMControl new];
        app.delegate = delegate;
        [app setActivationPolicy:NSApplicationActivationPolicyAccessory];
        [app run];
    }
    return 0;
}
