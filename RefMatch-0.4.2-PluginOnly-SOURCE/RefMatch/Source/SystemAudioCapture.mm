// Import Apple headers before JuceHeader.h can introduce JUCE namespace names.
// MacTypes/CoreServices also declare Point and Component.
#import <Foundation/Foundation.h>
#import <ScreenCaptureKit/ScreenCaptureKit.h>
#import <CoreMedia/CoreMedia.h>
#import <CoreAudio/CoreAudio.h>

#include "SystemAudioCapture.h"
#include <array>
#include <atomic>
#include <cmath>
#include <mutex>
#include <vector>

struct SystemAudioCapture::Impl
{
    static constexpr int channels = 2;
    static constexpr int sourceRate = 48000;
    static constexpr int capacity = sourceRate * 12;

    std::array<std::vector<float>, channels> ring { std::vector<float>(capacity, 0.0f),
                                                    std::vector<float>(capacity, 0.0f) };
    std::mutex mutex;
    int readPos = 0;
    int writePos = 0;
    int available = 0;
    double readFraction = 0.0;

    std::atomic<unsigned> generation { 0 };
    std::atomic<bool> running { false };
    std::atomic<bool> starting { false };
    juce::String status { "System audio capture stopped" };
    juce::String error;
    std::mutex textMutex;

    id delegateObj = nil;
    SCStream* stream = nil;
    dispatch_queue_t queue = dispatch_queue_create("com.refmatch.system.capture", DISPATCH_QUEUE_SERIAL);

    void setStatus(const juce::String& s)
    {
        std::lock_guard<std::mutex> g(textMutex);
        status = s;
        error.clear();
    }

    void setError(const juce::String& s)
    {
        std::lock_guard<std::mutex> g(textMutex);
        error = s;
        status = s;
    }

    void clearAudio()
    {
        std::lock_guard<std::mutex> g(mutex);
        readPos = writePos = available = 0;
        readFraction = 0.0;
    }

    void push(const AudioBufferList* abl, const AudioStreamBasicDescription& asbd, int frames)
    {
        if (abl == nullptr || frames <= 0) return;
        if (asbd.mFormatID != kAudioFormatLinearPCM) return;
        if ((asbd.mFormatFlags & kAudioFormatFlagIsFloat) == 0 || asbd.mBitsPerChannel != 32) return;

        const bool nonInterleaved = (asbd.mFormatFlags & kAudioFormatFlagIsNonInterleaved) != 0;
        const int srcChannels = (int) asbd.mChannelsPerFrame;
        if (srcChannels < 1) return;

        std::lock_guard<std::mutex> g(mutex);
        for (int i = 0; i < frames; ++i)
        {
            float l = 0.0f, r = 0.0f;
            if (nonInterleaved)
            {
                const float* p0 = static_cast<const float*>(abl->mBuffers[0].mData);
                if (p0 == nullptr) continue;
                l = p0[i];
                if (srcChannels > 1 && abl->mNumberBuffers > 1)
                {
                    const float* p1 = static_cast<const float*>(abl->mBuffers[1].mData);
                    r = p1 != nullptr ? p1[i] : l;
                }
                else r = l;
            }
            else
            {
                const float* p = static_cast<const float*>(abl->mBuffers[0].mData);
                if (p == nullptr) continue;
                l = p[i * srcChannels];
                r = srcChannels > 1 ? p[i * srcChannels + 1] : l;
            }

            ring[0][writePos] = l;
            ring[1][writePos] = r;
            writePos = (writePos + 1) % capacity;
            if (available < capacity) ++available;
            else readPos = (readPos + 1) % capacity;
        }
    }

    bool pull(juce::AudioBuffer<float>& dest, double rate)
    {
        // Never block the DAW waiting for the capture callback, and never
        // allocate temporary vectors on the audio thread.
        std::unique_lock<std::mutex> lock(mutex, std::try_to_lock);
        if (!lock.owns_lock()) return false;
        if (available > sourceRate / 5) {
            available = sourceRate / 10;
            readPos = (writePos - available + capacity) % capacity;
            readFraction = 0.0;
        }
        const double ratio = sourceRate / rate;
        bool received = false;
        for (int i = 0; i < dest.getNumSamples(); ++i) {
            const int advance = static_cast<int>(readFraction + ratio);
            if (available < std::max(2, advance)) break;
            for (int ch = 0; ch < dest.getNumChannels(); ++ch) {
                const auto& data = ring[ch % channels];
                const float first = data[readPos];
                const float second = data[(readPos + 1) % capacity];
                dest.setSample(ch, i, first + static_cast<float>(readFraction) * (second - first));
            }
            readFraction += ratio;
            const int consumed = static_cast<int>(readFraction);
            readFraction -= consumed;
            readPos = (readPos + consumed) % capacity;
            available -= consumed;
            received = true;
        }
        return received;
    }

};

@interface RMSystemCaptureDelegate : NSObject <SCStreamOutput, SCStreamDelegate>
{
@public
    std::weak_ptr<SystemAudioCapture::Impl> owner;
    unsigned captureGeneration;
}
@end

@implementation RMSystemCaptureDelegate
- (void)stream:(SCStream*)stream didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer ofType:(SCStreamOutputType)type
{
    (void)stream;
    auto p = owner.lock();
    if (type != SCStreamOutputTypeAudio || !p || p->generation.load() != captureGeneration || !p->running.load() || !CMSampleBufferDataIsReady(sampleBuffer)) return;

    CMAudioFormatDescriptionRef fmt = (CMAudioFormatDescriptionRef)CMSampleBufferGetFormatDescription(sampleBuffer);
    if (fmt == nullptr) return;
    const AudioStreamBasicDescription* asbd = CMAudioFormatDescriptionGetStreamBasicDescription(fmt);
    if (asbd == nullptr) return;

    size_t needed = 0;
    CMBlockBufferRef block = nullptr;
    OSStatus s = CMSampleBufferGetAudioBufferListWithRetainedBlockBuffer(sampleBuffer,
                                                                          &needed,
                                                                          nullptr,
                                                                          0,
                                                                          kCFAllocatorDefault,
                                                                          kCFAllocatorDefault,
                                                                          kCMSampleBufferFlag_AudioBufferList_Assure16ByteAlignment,
                                                                          &block);
    if (s != noErr || needed == 0) { if (block) CFRelease(block); return; }

    if (block) { CFRelease(block); block = nullptr; }
    std::vector<uint8_t> storage(needed);
    auto* abl = reinterpret_cast<AudioBufferList*>(storage.data());
    s = CMSampleBufferGetAudioBufferListWithRetainedBlockBuffer(sampleBuffer,
                                                                 &needed,
                                                                 abl,
                                                                 needed,
                                                                 kCFAllocatorDefault,
                                                                 kCFAllocatorDefault,
                                                                 kCMSampleBufferFlag_AudioBufferList_Assure16ByteAlignment,
                                                                 &block);
    if (s == noErr)
        p->push(abl, *asbd, (int)CMSampleBufferGetNumSamples(sampleBuffer));
    if (block) CFRelease(block);
}

- (void)stream:(SCStream*)stream didStopWithError:(NSError*)error
{
    (void)stream;
    if (auto p = owner.lock())
    {
        if (p->generation.load() != captureGeneration) return;
        p->running.store(false);
        p->starting.store(false);
        p->setError("System audio capture stopped: " + juce::String::fromUTF8([[error localizedDescription] UTF8String]));
    }
}
@end

SystemAudioCapture::SystemAudioCapture() : impl(std::make_shared<Impl>()) {}
SystemAudioCapture::~SystemAudioCapture() { stop(); }

void SystemAudioCapture::start()
{
    if (impl->running.load() || impl->starting.exchange(true)) return;
    impl->clearAudio();
    impl->setStatus("Requesting Screen & System Audio permission...");

    if (@available(macOS 13.0, *))
    {
        auto p = impl;
        const auto generation = ++p->generation;
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 20 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
            if (p->generation.load() == generation && p->starting.load()) {
                ++p->generation;
                p->starting.store(false);
                p->running.store(false);
                if (p->stream) [p->stream stopCaptureWithCompletionHandler:^(NSError*) {}];
                p->stream = nil;
                p->delegateObj = nil;
                p->setError("Capture permission/start timed out. Allow your host in macOS settings and retry.");
            }
        });
        [SCShareableContent getExcludingDesktopWindows:NO onScreenWindowsOnly:NO completionHandler:^(SCShareableContent* content, NSError* err)
        {
            dispatch_async(dispatch_get_main_queue(), ^{
            if (p->generation.load() != generation) return;
            if (err != nil || content == nil || content.displays.count == 0)
            {
                p->starting.store(false);
                p->setError("Allow Screen & System Audio Recording for your DAW in System Settings, then reopen the DAW.");
                return;
            }

            SCDisplay* display = content.displays.firstObject;
            NSMutableArray<SCRunningApplication*>* excluded = [NSMutableArray array];
            for (SCRunningApplication* app in content.applications) {
                NSString* bundle = app.bundleIdentifier;
                if (app.processID == NSProcessInfo.processInfo.processIdentifier
                    || [bundle isEqualToString:@"com.apple.logic10"]
                    || [bundle hasPrefix:@"com.apple.audio.AUHostingService"])
                    [excluded addObject:app];
            }
            SCContentFilter* filter = [[SCContentFilter alloc] initWithDisplay:display
                                                       excludingApplications:excluded
                                                            exceptingWindows:@[]];

            SCStreamConfiguration* config = [SCStreamConfiguration new];
            config.capturesAudio = YES;
            config.sampleRate = Impl::sourceRate;
            config.channelCount = 2;
            // Exclude this process plus Logic explicitly: AUHostingService may
            // execute the plugin separately from the DAW audio process.
            config.excludesCurrentProcessAudio = YES;
            config.width = 2;
            config.height = 2;
            config.minimumFrameInterval = CMTimeMake(1, 2);
            config.showsCursor = NO;
            config.queueDepth = 3;

            RMSystemCaptureDelegate* delegate = [RMSystemCaptureDelegate new];
            delegate->owner = p;
            delegate->captureGeneration = generation;
            if (p->stream) [p->stream stopCaptureWithCompletionHandler:^(NSError*) {}];
            p->delegateObj = delegate;
            p->stream = [[SCStream alloc] initWithFilter:filter configuration:config delegate:delegate];

            NSError* addError = nil;
            BOOL ok = [p->stream addStreamOutput:delegate type:SCStreamOutputTypeAudio sampleHandlerQueue:p->queue error:&addError];
            if (!ok)
            {
                p->starting.store(false);
                p->setError("Could not attach system audio stream: " + juce::String::fromUTF8([[addError localizedDescription] UTF8String]));
                return;
            }

            [p->stream startCaptureWithCompletionHandler:^(NSError* startError)
            {
                dispatch_async(dispatch_get_main_queue(), ^{
                if (p->generation.load() != generation) return;
                p->starting.store(false);
                if (startError != nil)
                {
                    p->running.store(false);
                    p->setError("Could not start system audio capture: " + juce::String::fromUTF8([[startError localizedDescription] UTF8String]));
                }
                else
                {
                    p->running.store(true);
                    p->setStatus("SYSTEM AUDIO READY");
                }
                });
            }];
            });
        }];
    }
    else
    {
        impl->starting.store(false);
        impl->setError("Reference metering requires macOS 13 or newer.");
    }
}

void SystemAudioCapture::stop()
{
    auto p = impl;
    if (!p) return;
    const auto stoppedGeneration = ++p->generation;
    p->starting.store(false);
    p->running.store(false);
    p->clearAudio();
    p->setStatus("System audio capture stopped");
    dispatch_async(dispatch_get_main_queue(), ^{
        if (p->generation.load() != stoppedGeneration) return;
        if (p->stream != nil)
        {
            [p->stream stopCaptureWithCompletionHandler:^(NSError*) {}];
            p->stream = nil;
        }
        p->delegateObj = nil;
    });
}

bool SystemAudioCapture::isRunning() const { return impl->running.load(); }
bool SystemAudioCapture::isStarting() const { return impl->starting.load(); }

juce::String SystemAudioCapture::getStatusText() const
{
    std::lock_guard<std::mutex> g(impl->textMutex);
    return impl->status;
}

juce::String SystemAudioCapture::getLastError() const
{
    std::lock_guard<std::mutex> g(impl->textMutex);
    return impl->error;
}

bool SystemAudioCapture::pullAudio(juce::AudioBuffer<float>& dest, double hostSampleRate)
{
    dest.clear();
    if (!impl->running.load() || hostSampleRate <= 0.0) return false;

    return impl->pull(dest, hostSampleRate);
}
