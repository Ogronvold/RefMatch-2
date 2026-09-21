#pragma once

#include <JuceHeader.h>
#include <functional>

class SpotifyAuth : private juce::Thread
{
public:
    SpotifyAuth();
    ~SpotifyAuth() override;

    void startLogin();

    bool isConnected() const;
    juce::String getAccessToken() const;

    std::function<void(bool success, const juce::String& message)> onAuthResult;

private:
    void run() override;

    juce::String createCodeVerifier();
    juce::String createCodeChallenge(const juce::String& verifier);
    juce::String createRandomString(int length);

    bool exchangeCodeForToken(const juce::String& code);
    void notifyResult(bool success, const juce::String& message);
    void sendBrowserResponse(juce::StreamingSocket& socket, bool success);

    juce::String codeVerifier;
    juce::String state;
    juce::String accessToken;
    juce::String refreshToken;

    std::unique_ptr<juce::StreamingSocket> listener;

    juce::CriticalSection tokenLock;

    static constexpr const char* clientId =
        "7bd6eaf4a0524357aaeb78de45a643f2";

    static constexpr const char* redirectUri =
        "http://127.0.0.1:8080/callback";
};
