#include "SystemAudioCapture.h"

#import <Foundation/Foundation.h>
#import <ScreenCaptureKit/ScreenCaptureKit.h>
#import <CoreMedia/CoreMedia.h>
#import <CoreAudio/CoreAudio.h>

#include <atomic>
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

    int pop(std::array<std::vector<float>, channels>& out, int frames)
    {
        std::lock_guard<std::mutex> g(mutex);
        const int n = juce::jmin(frames, available);
        for (int ch = 0; ch < channels; ++ch) out[ch].resize((size_t) n);
        for (int i = 0; i < n; ++i)
        {
            out[0][(size_t)i] = ring[0][readPos];
            out[1][(size_t)i] = ring[1][readPos];
            readPos = (readPos + 1) % capacity;
        }
        available -= n;
        return n;
    }
};

@interface RMSystemCaptureDelegate : NSObject <SCStreamOutput, SCStreamDelegate>
@property(nonatomic, assign) SystemAudioCapture::Impl* owner;
@end

@implementation RMSystemCaptureDelegate
- (void)stream:(SCStream*)stream didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer ofType:(SCStreamOutputType)type
{
    (void)stream;
    if (type != SCStreamOutputTypeAudio || self.owner == nullptr || !CMSampleBufferDataIsReady(sampleBuffer)) return;

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
        self.owner->push(abl, *asbd, (int)CMSampleBufferGetNumSamples(sampleBuffer));
    if (block) CFRelease(block);
}

- (void)stream:(SCStream*)stream didStopWithError:(NSError*)error
{
    (void)stream;
    if (self.owner)
    {
        self.owner->running.store(false);
        self.owner->starting.store(false);
        self.owner->setError("System audio capture stopped: " + juce::String([[error localizedDescription] UTF8String]));
    }
}
@end

SystemAudioCapture::SystemAudioCapture() : impl(std::make_unique<Impl>()) {}
SystemAudioCapture::~SystemAudioCapture() { stop(); }

void SystemAudioCapture::start()
{
    if (impl->running.load() || impl->starting.exchange(true)) return;
    impl->clearAudio();
    impl->setStatus("Requesting Screen & System Audio permission…");

    if (@available(macOS 13.0, *))
    {
        auto* p = impl.get();
        [SCShareableContent getExcludingDesktopWindows:NO onScreenWindowsOnly:NO completionHandler:^(SCShareableContent* content, NSError* err)
        {
            if (err != nil || content == nil || content.displays.count == 0)
            {
                p->starting.store(false);
                p->setError("Allow Screen & System Audio Recording for your DAW in System Settings, then reopen the DAW.");
                return;
            }

            SCDisplay* display = content.displays.firstObject;
            SCContentFilter* filter = [[SCContentFilter alloc] initWithDisplay:display
                                                       excludingApplications:@[]
                                                            exceptingWindows:@[]];

            SCStreamConfiguration* config = [SCStreamConfiguration new];
            config.capturesAudio = YES;
            config.sampleRate = Impl::sourceRate;
            config.channelCount = 2;
            // RefMatch runs inside the DAW process. Excluding the current process
            // prevents the user's mix from leaking into the captured reference.
            config.excludesCurrentProcessAudio = YES;
            config.width = 2;
            config.height = 2;
            config.minimumFrameInterval = CMTimeMake(1, 2);
            config.showsCursor = NO;
            config.queueDepth = 3;

            RMSystemCaptureDelegate* delegate = [RMSystemCaptureDelegate new];
            delegate.owner = p;
            p->delegateObj = delegate;
            p->stream = [[SCStream alloc] initWithFilter:filter configuration:config delegate:delegate];

            NSError* addError = nil;
            BOOL ok = [p->stream addStreamOutput:delegate type:SCStreamOutputTypeAudio sampleHandlerQueue:p->queue error:&addError];
            if (!ok)
            {
                p->starting.store(false);
                p->setError("Could not attach system audio stream: " + juce::String([[addError localizedDescription] UTF8String]));
                return;
            }

            [p->stream startCaptureWithCompletionHandler:^(NSError* startError)
            {
                p->starting.store(false);
                if (startError != nil)
                {
                    p->running.store(false);
                    p->setError("Could not start system audio capture: " + juce::String([[startError localizedDescription] UTF8String]));
                }
                else
                {
                    p->running.store(true);
                    p->setStatus("SYSTEM AUDIO READY");
                }
            }];
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
    if (!impl) return;
    auto* p = impl.get();
    p->starting.store(false);
    if (p->stream != nil)
    {
        [p->stream stopCaptureWithCompletionHandler:^(NSError*) {}];
        p->stream = nil;
    }
    p->delegateObj = nil;
    p->running.store(false);
    p->clearAudio();
    p->setStatus("System audio capture stopped");
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

    const int outN = dest.getNumSamples();
    const double ratio = (double)Impl::sourceRate / hostSampleRate;
    const int inNeeded = juce::jmax(2, (int)std::ceil(outN * ratio) + 2);
    std::array<std::vector<float>, Impl::channels> input;
    const int got = impl->pop(input, inNeeded);
    if (got < 2) return false;

    for (int ch = 0; ch < juce::jmin(2, dest.getNumChannels()); ++ch)
    {
        auto* d = dest.getWritePointer(ch);
        for (int i = 0; i < outN; ++i)
        {
            const double pos = i * ratio;
            const int i0 = juce::jlimit(0, got - 1, (int)pos);
            const int i1 = juce::jmin(got - 1, i0 + 1);
            const float frac = (float)(pos - i0);
            d[i] = input[ch][(size_t)i0] + frac * (input[ch][(size_t)i1] - input[ch][(size_t)i0]);
        }
    }

    if (dest.getNumChannels() > 2)
        for (int ch = 2; ch < dest.getNumChannels(); ++ch)
            dest.copyFrom(ch, 0, dest, ch % 2, 0, outN);

    return true;
}
