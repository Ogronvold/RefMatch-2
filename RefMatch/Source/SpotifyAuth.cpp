#include "SpotifyAuth.h"

SpotifyAuth::SpotifyAuth()
    : juce::Thread("Spotify Auth Callback")
{
}

SpotifyAuth::~SpotifyAuth()
{
    signalThreadShouldExit();

    if (listener != nullptr)
        listener->close();

    stopThread(2000);
}

juce::String SpotifyAuth::createRandomString(int length)
{
    static constexpr char chars[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789";

    juce::Random random;
    juce::String result;

    for (int i = 0; i < length; ++i)
        result += chars[random.nextInt((int) std::strlen(chars))];

    return result;
}

juce::String SpotifyAuth::createCodeVerifier()
{
    static constexpr char chars[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789-._~";

    juce::Random random;
    juce::String result;

    for (int i = 0; i < 64; ++i)
        result += chars[random.nextInt((int) std::strlen(chars))];

    return result;
}

juce::String SpotifyAuth::createCodeChallenge(
    const juce::String& verifier)
{
    juce::SHA256 hash(
        verifier.toRawUTF8(),
        (size_t) verifier.getNumBytesAsUTF8());

    const auto raw = hash.getRawData();

    juce::MemoryOutputStream stream;

    juce::Base64::convertToBase64(
        stream,
        raw.getData(),
        raw.getSize());

    return stream.toString()
        .replace("+", "-")
        .replace("/", "_")
        .trimCharactersAtEnd("=");
}

void SpotifyAuth::startLogin()
{
    if (isThreadRunning())
    {
        signalThreadShouldExit();

        if (listener != nullptr)
            listener->close();

        stopThread(1000);
    }

    codeVerifier = createCodeVerifier();
    state = createRandomString(32);

    listener = std::make_unique<juce::StreamingSocket>();

    if (!listener->createListener(8080, "127.0.0.1"))
    {
        notifyResult(
            false,
            "Could not start Spotify callback on port 8080");

        return;
    }

    startThread();

    const auto challenge =
        createCodeChallenge(codeVerifier);

    const juce::String scopes =
        "streaming "
        "user-read-private "
        "user-read-email "
        "user-read-playback-state "
        "user-modify-playback-state "
        "user-read-currently-playing";

    juce::String authUrl =
        "https://accounts.spotify.com/authorize"
        "?response_type=code"
        "&client_id=" + juce::String(clientId)
        + "&redirect_uri="
        + juce::URL::addEscapeChars(redirectUri, true)
        + "&scope="
        + juce::URL::addEscapeChars(scopes, true)
        + "&state="
        + juce::URL::addEscapeChars(state, true)
        + "&code_challenge_method=S256"
        + "&code_challenge="
        + challenge;

    juce::URL(authUrl).launchInDefaultBrowser();
}

void SpotifyAuth::run()
{
    if (listener == nullptr)
        return;

    std::unique_ptr<juce::StreamingSocket> connection(
        listener->waitForNextConnection());

    if (connection == nullptr || threadShouldExit())
        return;

    char buffer[8192] {};
    int totalRead = 0;

    while (totalRead < (int) sizeof(buffer) - 1)
    {
        if (connection->waitUntilReady(true, 2000) <= 0)
            break;

        const int bytesRead =
            connection->read(
                buffer + totalRead,
                (int) sizeof(buffer) - 1 - totalRead,
                false);

        if (bytesRead <= 0)
            break;

        totalRead += bytesRead;

        juce::String current =
            juce::String::fromUTF8(buffer, totalRead);

        if (current.contains("\r\n\r\n"))
            break;
    }

    const juce::String request =
        juce::String::fromUTF8(buffer, totalRead);

    const juce::String firstLine =
        request.upToFirstOccurrenceOf(
            "\r\n",
            false,
            false);

    const juce::String requestTarget =
        firstLine.fromFirstOccurrenceOf(
            "GET ",
            false,
            false)
        .upToFirstOccurrenceOf(
            " HTTP/",
            false,
            false);

    const int questionMark =
        requestTarget.indexOfChar('?');

    if (questionMark < 0)
    {
        sendBrowserResponse(*connection, false);
        notifyResult(false, "Spotify returned no authorization code");
        return;
    }

    const auto query =
        requestTarget.substring(questionMark + 1);

    juce::String code;
    juce::String returnedState;
    juce::String error;

    juce::StringArray parameters;
    parameters.addTokens(query, "&", "");

    for (const auto& parameter : parameters)
    {
        const auto key =
            parameter.upToFirstOccurrenceOf(
                "=",
                false,
                false);

        const auto value =
            juce::URL::removeEscapeChars(
                parameter.fromFirstOccurrenceOf(
                    "=",
                    false,
                    false));

        if (key == "code")
            code = value;
        else if (key == "state")
            returnedState = value;
        else if (key == "error")
            error = value;
    }

    if (error.isNotEmpty())
    {
        sendBrowserResponse(*connection, false);
        notifyResult(false, "Spotify login was cancelled: " + error);
        return;
    }

    if (returnedState != state)
    {
        sendBrowserResponse(*connection, false);
        notifyResult(false, "Spotify login state did not match");
        return;
    }

    if (code.isEmpty())
    {
        sendBrowserResponse(*connection, false);
        notifyResult(false, "Spotify returned no authorization code");
        return;
    }

    const bool success =
        exchangeCodeForToken(code);

    sendBrowserResponse(
        *connection,
        success);

    if (success)
        notifyResult(true, "CONNECTED");
}

bool SpotifyAuth::exchangeCodeForToken(
    const juce::String& code)
{
    juce::String body;

    body
        << "client_id="
        << juce::URL::addEscapeChars(clientId, true)
        << "&grant_type=authorization_code"
        << "&code="
        << juce::URL::addEscapeChars(code, true)
        << "&redirect_uri="
        << juce::URL::addEscapeChars(redirectUri, true)
        << "&code_verifier="
        << juce::URL::addEscapeChars(codeVerifier, true);

    juce::URL tokenUrl(
        "https://accounts.spotify.com/api/token");

    tokenUrl = tokenUrl.withPOSTData(body);

    int statusCode = 0;

    juce::StringPairArray responseHeaders;

    auto options =
        juce::URL::InputStreamOptions(
            juce::URL::ParameterHandling::inAddress)
            .withHttpRequestCmd("POST")
            .withExtraHeaders(
                "Content-Type: application/x-www-form-urlencoded\r\n")
            .withConnectionTimeoutMs(10000)
            .withStatusCode(&statusCode)
            .withResponseHeaders(&responseHeaders);

    auto stream =
        tokenUrl.createInputStream(options);

    if (stream == nullptr)
    {
        notifyResult(
            false,
            "Could not contact Spotify token server");

        return false;
    }

    const auto response =
        stream->readEntireStreamAsString();

    if (statusCode < 200 || statusCode >= 300)
    {
        notifyResult(
            false,
            "Spotify token request failed: "
                + juce::String(statusCode));

        return false;
    }

    const auto json =
        juce::JSON::parse(response);

    const auto* object =
        json.getDynamicObject();

    if (object == nullptr)
    {
        notifyResult(
            false,
            "Spotify returned an invalid token response");

        return false;
    }

    const auto newAccessToken =
        object->getProperty(
            "access_token").toString();

    const auto newRefreshToken =
        object->getProperty(
            "refresh_token").toString();

    if (newAccessToken.isEmpty())
    {
        notifyResult(
            false,
            "Spotify returned no access token");

        return false;
    }

    {
        const juce::ScopedLock lock(tokenLock);

        accessToken = newAccessToken;
        refreshToken = newRefreshToken;
    }

    return true;
}

void SpotifyAuth::sendBrowserResponse(
    juce::StreamingSocket& socket,
    bool success)
{
    const juce::String body =
        success
            ? "<html><body style=\"background:#0a0d12;color:white;"
              "font-family:sans-serif;text-align:center;padding-top:80px;\">"
              "<h1>RefMatch connected to Spotify ✓</h1>"
              "<p>You can close this window and return to Logic.</p>"
              "</body></html>"
            : "<html><body style=\"background:#0a0d12;color:white;"
              "font-family:sans-serif;text-align:center;padding-top:80px;\">"
              "<h1>Spotify connection failed</h1>"
              "<p>Return to RefMatch and try again.</p>"
              "</body></html>";

    juce::String response;

    response
        << "HTTP/1.1 200 OK\r\n"
        << "Content-Type: text/html; charset=utf-8\r\n"
        << "Content-Length: "
        << body.getNumBytesAsUTF8()
        << "\r\n"
        << "Connection: close\r\n\r\n"
        << body;

    socket.write(
        response.toRawUTF8(),
        (int) response.getNumBytesAsUTF8());
}

void SpotifyAuth::notifyResult(
    bool success,
    const juce::String& message)
{
    if (!onAuthResult)
        return;

    auto callback = onAuthResult;

    juce::MessageManager::callAsync(
        [callback, success, message]
        {
            callback(success, message);
        });
}

bool SpotifyAuth::isConnected() const
{
    const juce::ScopedLock lock(
        const_cast<juce::CriticalSection&>(
            tokenLock));

    return accessToken.isNotEmpty();
}

juce::String SpotifyAuth::getAccessToken() const
{
    const juce::ScopedLock lock(
        const_cast<juce::CriticalSection&>(
            tokenLock));

    return accessToken;
}
