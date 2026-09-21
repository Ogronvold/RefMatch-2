#include "SpotifyClient.h"

void SpotifyClient::setAccessToken(const juce::String& token)
{
    accessToken = token;
}

juce::String SpotifyClient::makeAuthHeader() const
{
    return "Authorization: Bearer " + accessToken + "\r\n";
}

std::unique_ptr<juce::InputStream> SpotifyClient::createRequest(
    const juce::String& endpoint,
    const juce::String& method,
    const juce::String& body)
{
    juce::URL url("https://api.spotify.com/v1" + endpoint);

    if (method == "GET")
    {
        auto options =
            juce::URL::InputStreamOptions(
                juce::URL::ParameterHandling::inAddress)
                .withExtraHeaders(makeAuthHeader())
                .withConnectionTimeoutMs(10000);

        return url.createInputStream(options);
    }

    if (body.isNotEmpty())
        url = url.withPOSTData(body);

    juce::String headers = makeAuthHeader();

    if (body.isNotEmpty())
        headers += "Content-Type: application/json\r\n";

    auto options =
        juce::URL::InputStreamOptions(
            juce::URL::ParameterHandling::inAddress)
            .withHttpRequestCmd(method)
            .withExtraHeaders(headers)
            .withConnectionTimeoutMs(10000);

    return url.createInputStream(options);
}

void SpotifyClient::getPlaybackState(
    std::function<void(bool, SpotifyPlaybackInfo)> callback)
{
    juce::Thread::launch(
        [this, callback]
        {
            SpotifyPlaybackInfo info;

            auto stream =
                createRequest("/me/player");

            if (stream == nullptr)
            {
                juce::MessageManager::callAsync(
                    [callback, info]
                    {
                        callback(false, info);
                    });

                return;
            }

            const auto response =
                stream->readEntireStreamAsString();

            const auto json =
                juce::JSON::parse(response);

            const auto* object =
                json.getDynamicObject();

            if (object == nullptr)
            {
                juce::MessageManager::callAsync(
                    [callback, info]
                    {
                        callback(false, info);
                    });

                return;
            }

            info.isPlaying =
                (bool) object->getProperty("is_playing");

            info.progressMs =
                (int) object->getProperty("progress_ms");

            if (const auto* device =
                    object->getProperty("device")
                        .getDynamicObject())
            {
                info.deviceName =
                    device->getProperty("name").toString();
            }

            if (const auto* item =
                    object->getProperty("item")
                        .getDynamicObject())
            {
                info.trackName =
                    item->getProperty("name").toString();

                info.durationMs =
                    (int) item->getProperty("duration_ms");

                const auto artists =
                    item->getProperty("artists");

                if (artists.isArray()
                    && artists.getArray()->size() > 0)
                {
                    if (const auto* artist =
                            artists.getArray()
                                ->getReference(0)
                                .getDynamicObject())
                    {
                        info.artistName =
                            artist->getProperty("name")
                                .toString();
                    }
                }
            }

            juce::MessageManager::callAsync(
                [callback, info]
                {
                    callback(true, info);
                });
        });
}

void SpotifyClient::pause(
    std::function<void(bool)> callback)
{
    juce::Thread::launch(
        [this, callback]
        {
            auto stream =
                createRequest(
                    "/me/player/pause",
                    "PUT");

            const bool success =
                stream != nullptr;

            juce::MessageManager::callAsync(
                [callback, success]
                {
                    callback(success);
                });
        });
}

void SpotifyClient::resume(
    std::function<void(bool)> callback)
{
    juce::Thread::launch(
        [this, callback]
        {
            auto stream =
                createRequest(
                    "/me/player/play",
                    "PUT");

            const bool success =
                stream != nullptr;

            juce::MessageManager::callAsync(
                [callback, success]
                {
                    callback(success);
                });
        });
}

void SpotifyClient::playTrack(
    const juce::String& trackUri,
    std::function<void(bool)> callback)
{
    juce::Thread::launch(
        [this, trackUri, callback]
        {
            const juce::String body =
                "{\"uris\":[\""
                + trackUri
                + "\"]}";

            auto stream =
                createRequest(
                    "/me/player/play",
                    "PUT",
                    body);

            const bool success =
                stream != nullptr;

            juce::MessageManager::callAsync(
                [callback, success]
                {
                    callback(success);
                });
        });
}

void SpotifyClient::searchTracks(
    const juce::String& query,
    std::function<void(
        bool,
        std::vector<SpotifyTrackInfo>)> callback)
{
    juce::Thread::launch(
        [this, query, callback]
        {
            std::vector<SpotifyTrackInfo> results;

            const auto encodedQuery =
                juce::URL::addEscapeChars(
                    query,
                    true);

            auto stream =
                createRequest(
                    "/search?q="
                    + encodedQuery
                    + "&type=track&limit=10");

            if (stream == nullptr)
            {
                juce::MessageManager::callAsync(
                    [callback, results]
                    {
                        callback(false, results);
                    });

                return;
            }

            const auto response =
                stream->readEntireStreamAsString();

            const auto json =
                juce::JSON::parse(response);

            const auto* root =
                json.getDynamicObject();

            if (root == nullptr)
            {
                juce::MessageManager::callAsync(
                    [callback, results]
                    {
                        callback(false, results);
                    });

                return;
            }

            const auto tracksVar =
                root->getProperty("tracks");

            const auto* tracksObject =
                tracksVar.getDynamicObject();

            if (tracksObject == nullptr)
            {
                juce::MessageManager::callAsync(
                    [callback, results]
                    {
                        callback(false, results);
                    });

                return;
            }

            const auto items =
                tracksObject->getProperty("items");

            if (items.isArray())
            {
                for (const auto& itemVar
                     : *items.getArray())
                {
                    const auto* item =
                        itemVar.getDynamicObject();

                    if (item == nullptr)
                        continue;

                    SpotifyTrackInfo info;

                    info.name =
                        item->getProperty("name")
                            .toString();

                    info.uri =
                        item->getProperty("uri")
                            .toString();

                    const auto artists =
                        item->getProperty("artists");

                    if (artists.isArray()
                        && artists.getArray()->size() > 0)
                    {
                        if (const auto* artist =
                                artists.getArray()
                                    ->getReference(0)
                                    .getDynamicObject())
                        {
                            info.artist =
                                artist->getProperty("name")
                                    .toString();
                        }
                    }

                    if (const auto* album =
                            item->getProperty("album")
                                .getDynamicObject())
                    {
                        info.album =
                            album->getProperty("name")
                                .toString();

                        const auto images =
                            album->getProperty("images");

                        if (images.isArray()
                            && images.getArray()->size() > 0)
                        {
                            if (const auto* image =
                                    images.getArray()
                                        ->getReference(0)
                                        .getDynamicObject())
                            {
                                info.imageUrl =
                                    image->getProperty("url")
                                        .toString();
                            }
                        }
                    }

                    results.push_back(info);
                }
            }

            juce::MessageManager::callAsync(
                [callback, results]
                {
                    callback(true, results);
                });
        });
}
