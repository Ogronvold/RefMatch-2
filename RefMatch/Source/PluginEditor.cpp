#include "PluginEditor.h"

namespace
{
    const auto textPrimary = juce::Colour::fromRGB(232, 237, 245);
    const auto textMuted = juce::Colour::fromRGB(126, 140, 159);
    const auto panel = juce::Colour::fromRGB(18, 23, 31);
    const auto panel2 = juce::Colour::fromRGB(22, 28, 37);
    const auto accent = juce::Colour::fromRGB(77, 205, 216);
}

AuroraLookAndFeel::AuroraLookAndFeel()
{
    setColour(juce::TextButton::textColourOffId, textPrimary);
    setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    setColour(juce::ToggleButton::textColourId, textPrimary);
    setColour(juce::Label::textColourId, textPrimary);
    setColour(juce::TextEditor::backgroundColourId, juce::Colour::fromRGB(12, 16, 22));
    setColour(juce::TextEditor::textColourId, textPrimary);
    setColour(juce::TextEditor::outlineColourId, juce::Colour::fromRGB(48, 59, 74));
    setColour(juce::TextEditor::focusedOutlineColourId, accent);
    setColour(juce::ComboBox::backgroundColourId, juce::Colour::fromRGB(12, 16, 22));
    setColour(juce::ComboBox::textColourId, textPrimary);
    setColour(juce::ComboBox::outlineColourId, juce::Colour::fromRGB(48, 59, 74));
    setColour(juce::Slider::thumbColourId, accent);
    setColour(juce::Slider::trackColourId, juce::Colour::fromRGB(50, 64, 80));
    setColour(juce::Slider::backgroundColourId, juce::Colour::fromRGB(31, 39, 50));
}

void AuroraLookAndFeel::drawButtonBackground(juce::Graphics& g,
                                              juce::Button& button,
                                              const juce::Colour&,
                                              bool highlighted,
                                              bool down)
{
    auto bounds = button.getLocalBounds().toFloat();
    auto colour = juce::Colour::fromRGB(35, 43, 55);
    if (highlighted) colour = juce::Colour::fromRGB(45, 56, 71);
    if (down) colour = juce::Colour::fromRGB(56, 70, 89);

    g.setColour(colour);
    g.fillRoundedRectangle(bounds, 9.0f);
    g.setColour(juce::Colour::fromRGBA(255, 255, 255, 20));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 9.0f, 1.0f);
}

void AuroraLookAndFeel::drawToggleButton(juce::Graphics& g,
                                          juce::ToggleButton& button,
                                          bool,
                                          bool)
{
    auto bounds = button.getLocalBounds().toFloat();
    constexpr float toggleWidth = 42.0f;
    constexpr float toggleHeight = 22.0f;

    auto toggleArea = juce::Rectangle<float>(bounds.getRight() - toggleWidth,
                                              bounds.getCentreY() - toggleHeight * 0.5f,
                                              toggleWidth, toggleHeight);

    g.setColour(button.getToggleState() ? accent : juce::Colour::fromRGB(49, 57, 69));
    g.fillRoundedRectangle(toggleArea, toggleHeight * 0.5f);

    constexpr float dot = 16.0f;
    const float x = button.getToggleState()
        ? toggleArea.getRight() - dot - 3.0f
        : toggleArea.getX() + 3.0f;

    g.setColour(juce::Colours::white);
    g.fillEllipse(x, toggleArea.getY() + 3.0f, dot, dot);
    g.setColour(textPrimary);
    g.setFont(12.0f);
    g.drawText(button.getButtonText(),
               bounds.withTrimmedRight(toggleWidth + 12.0f),
               juce::Justification::centredLeft);
}

RefMatchAudioProcessorEditor::RefMatchAudioProcessorEditor(RefMatchAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(980, 760);
    setResizable(true, true);
    setResizeLimits(820, 680, 1300, 950);
    setLookAndFeel(&lookAndFeel);

    auto setupLabel = [this](juce::Label& label, const juce::String& text,
                             float size, juce::Colour colour, bool bold = false)
    {
        label.setText(text, juce::dontSendNotification);
        label.setFont(juce::Font(size, bold ? juce::Font::bold : juce::Font::plain));
        label.setColour(juce::Label::textColourId, colour);
        addAndMakeVisible(label);
    };

    setupLabel(logoLabel, "RefMatch", 34.0f, juce::Colours::white, true);
    setupLabel(subtitleLabel, "REFERENCE  /  A-B  /  MATCH", 11.0f, textMuted);
    setupLabel(buildLabel, "REFMATCH 2  •  NEW BUILD", 10.5f, juce::Colour::fromRGB(77, 205, 216), true);
    setupLabel(localTitle, "LOCAL REFERENCE", 12.0f, textMuted, true);
    setupLabel(referenceName, "No reference loaded", 17.0f, textPrimary);
    setupLabel(matchTitle, "MATCH EQ", 12.0f, textMuted, true);
    setupLabel(matchAmountLabel, "AMOUNT", 11.0f, textMuted);
    setupLabel(maxCorrectionLabel, "MAX ±dB", 11.0f, textMuted);
    setupLabel(metersLabel, "MIX  — dB     REF  — dB", 11.0f, textMuted);
    setupLabel(spotifyTitle, "SPOTIFY CONTROL", 12.0f, textMuted, true);
    setupLabel(spotifyStatus, "NOT CONNECTED", 11.0f, textMuted);
    setupLabel(spotifyDevice, "Device: —", 12.0f, textMuted);
    setupLabel(spotifyNowPlaying, "Nothing playing", 15.0f, textPrimary);
    setupLabel(spotifyNote, "Spotify is control + metadata only. Local files are used for DSP / Match EQ.", 10.5f, textMuted);
    setupLabel(referenceTime, "0:00 / 0:00", 11.0f, textMuted);

    juce::Component* components[] = {
        &loadButton, &localPlayButton, &referencePosition,
        &mixButton, &referenceButton, &levelMatch,
        &learnButton, &matchEnabled, &matchAmount, &maxCorrection,
        &resetMatchButton, &bypassButton,
        &spotifyButton, &spotifyPlayPause, &spotifySearch,
        &spotifySearchButton, &spotifyResults, &spotifyPlaySelected
    };

    for (auto* c : components)
        addAndMakeVisible(c);

    referencePosition.setSliderStyle(juce::Slider::LinearHorizontal);
    referencePosition.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    referencePosition.setRange(0.0, 1.0, 0.001);
    referencePosition.onDragStart = [this] { draggingReferencePosition = true; };
    referencePosition.onDragEnd = [this]
    {
        draggingReferencePosition = false;
        audioProcessor.setReferencePositionSeconds(referencePosition.getValue());
    };

    matchAmount.setSliderStyle(juce::Slider::LinearHorizontal);
    matchAmount.setTextBoxStyle(juce::Slider::TextBoxRight, false, 64, 22);
    maxCorrection.setSliderStyle(juce::Slider::LinearHorizontal);
    maxCorrection.setTextBoxStyle(juce::Slider::TextBoxRight, false, 64, 22);
    maxCorrection.setNumDecimalPlacesToDisplay(1);

    spotifySearch.setTextToShowWhenEmpty("Search Spotify tracks…", textMuted);
    spotifySearch.setReturnKeyStartsNewLine(false);
    spotifySearch.onReturnKey = [this] { runSpotifySearch(); };
    spotifyResults.setTextWhenNothingSelected("Search results");

    levelMatchAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.apvts, "levelmatch", levelMatch);
    matchEnabledAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.apvts, "matchenabled", matchEnabled);
    matchAmountAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "matchamount", matchAmount);
    maxCorrectionAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "maxcorrection", maxCorrection);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.apvts, "bypass", bypassButton);

    const auto existingReference = audioProcessor.getReferenceName();
    if (existingReference.isNotEmpty())
        referenceName.setText(existingReference, juce::dontSendNotification);

    loadButton.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser>(
            "Choose reference track", juce::File {}, "*.wav;*.aif;*.aiff;*.mp3;*.m4a;*.flac");

        chooser->launchAsync(juce::FileBrowserComponent::openMode
                                 | juce::FileBrowserComponent::canSelectFiles,
            [this](const juce::FileChooser& fc)
            {
                const auto file = fc.getResult();
                if (file.existsAsFile() && audioProcessor.loadReference(file))
                {
                    referenceName.setText(file.getFileName(), juce::dontSendNotification);
                    referencePosition.setRange(0.0,
                        juce::jmax(0.001, audioProcessor.getReferenceLengthSeconds()), 0.001);
                    updateReferenceTransportText();
                }
            });
    };

    localPlayButton.onClick = [this]
    {
        const bool shouldPlay = !audioProcessor.isReferencePlaying();
        audioProcessor.setReferencePlaying(shouldPlay);
        localPlayButton.setButtonText(shouldPlay ? "PAUSE" : "PLAY");
    };

    mixButton.onClick = [this] { setABMode(0); };
    referenceButton.onClick = [this] { setABMode(1); };

    learnButton.onClick = [this]
    {
        audioProcessor.learnMatch();
        if (auto* parameter = audioProcessor.apvts.getParameter("matchenabled"))
        {
            parameter->beginChangeGesture();
            parameter->setValueNotifyingHost(1.0f);
            parameter->endChangeGesture();
        }
        learnButton.setButtonText("LEARNED");
        juce::Timer::callAfterDelay(1200, [safe = juce::Component::SafePointer<RefMatchAudioProcessorEditor>(this)]
        {
            if (safe != nullptr)
                safe->learnButton.setButtonText("LEARN");
        });
    };

    resetMatchButton.onClick = [this] { audioProcessor.clearMatch(); };

    spotifyAuth.onAuthResult = [this](bool success, const juce::String& message)
    {
        spotifyStatus.setText(success ? "CONNECTED" : message, juce::dontSendNotification);
        spotifyStatus.setColour(juce::Label::textColourId,
            success ? juce::Colour::fromRGB(75, 205, 142)
                    : juce::Colour::fromRGB(232, 105, 116));

        if (success)
        {
            spotifyClient.setAccessToken(spotifyAuth.getAccessToken());
            refreshSpotifyPlayback();
        }
    };

    spotifyButton.onClick = [this] { spotifyAuth.startLogin(); };
    spotifySearchButton.onClick = [this] { runSpotifySearch(); };

    spotifyPlayPause.onClick = [this]
    {
        if (!spotifyAuth.isConnected())
            return;

        auto done = [this](bool ok)
        {
            if (ok)
                juce::Timer::callAfterDelay(250, [safe = juce::Component::SafePointer<RefMatchAudioProcessorEditor>(this)]
                {
                    if (safe != nullptr) safe->refreshSpotifyPlayback();
                });
        };

        if (spotifyIsPlaying) spotifyClient.pause(done);
        else spotifyClient.resume(done);
    };

    spotifyPlaySelected.onClick = [this]
    {
        const int index = spotifyResults.getSelectedItemIndex();
        if (index < 0 || index >= (int) spotifyTracks.size())
            return;

        spotifyClient.playTrack(spotifyTracks[(size_t) index].uri,
            [this](bool ok)
            {
                if (ok)
                    juce::Timer::callAfterDelay(300, [safe = juce::Component::SafePointer<RefMatchAudioProcessorEditor>(this)]
                    {
                        if (safe != nullptr) safe->refreshSpotifyPlayback();
                    });
            });
    };

    currentAB = audioProcessor.apvts.getRawParameterValue("ab")->load() > 0.5f ? 1 : 0;
    startTimerHz(5);
}

RefMatchAudioProcessorEditor::~RefMatchAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void RefMatchAudioProcessorEditor::setABMode(int mode)
{
    if (auto* parameter = audioProcessor.apvts.getParameter("ab"))
    {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost(mode == 0 ? 0.0f : 1.0f);
        parameter->endChangeGesture();
    }
    currentAB = mode;
    repaint();
}

void RefMatchAudioProcessorEditor::runSpotifySearch()
{
    const auto query = spotifySearch.getText().trim();
    if (query.isEmpty()) return;

    if (!spotifyAuth.isConnected())
    {
        spotifyStatus.setText("CONNECT FIRST", juce::dontSendNotification);
        return;
    }

    spotifyResults.clear();
    spotifyTracks.clear();
    spotifyResults.setTextWhenNothingSelected("Searching…");

    spotifyClient.searchTracks(query,
        [this](bool ok, std::vector<SpotifyTrackInfo> tracks)
        {
            spotifyResults.setTextWhenNothingSelected(ok ? "Search results" : "Search failed");
            if (!ok) return;

            spotifyTracks = std::move(tracks);
            for (size_t i = 0; i < spotifyTracks.size(); ++i)
            {
                const auto& t = spotifyTracks[i];
                spotifyResults.addItem(t.name + " — " + t.artist, (int) i + 1);
            }

            if (!spotifyTracks.empty())
                spotifyResults.setSelectedItemIndex(0, juce::dontSendNotification);
        });
}

void RefMatchAudioProcessorEditor::refreshSpotifyPlayback()
{
    if (!spotifyAuth.isConnected()) return;

    spotifyClient.getPlaybackState(
        [this](bool ok, SpotifyPlaybackInfo info)
        {
            if (!ok) return;

            spotifyIsPlaying = info.isPlaying;
            spotifyDevice.setText("Device: " + (info.deviceName.isEmpty() ? juce::String("—") : info.deviceName),
                                  juce::dontSendNotification);

            const auto now = info.trackName.isEmpty()
                ? juce::String("Nothing playing")
                : info.trackName + " — " + info.artistName;
            spotifyNowPlaying.setText(now, juce::dontSendNotification);
            spotifyPlayPause.setButtonText(info.isPlaying ? "PAUSE" : "PLAY");
        });
}

juce::String RefMatchAudioProcessorEditor::formatTime(double seconds)
{
    seconds = juce::jmax(0.0, seconds);
    const int total = (int) std::round(seconds);
    return juce::String(total / 60) + ":" + juce::String(total % 60).paddedLeft('0', 2);
}

void RefMatchAudioProcessorEditor::updateReferenceTransportText()
{
    const auto pos = audioProcessor.getReferencePositionSeconds();
    const auto len = audioProcessor.getReferenceLengthSeconds();
    referenceTime.setText(formatTime(pos) + " / " + formatTime(len), juce::dontSendNotification);

    if (!draggingReferencePosition && len > 0.0)
    {
        referencePosition.setRange(0.0, juce::jmax(0.001, len), 0.001);
        referencePosition.setValue(pos, juce::dontSendNotification);
    }
}

void RefMatchAudioProcessorEditor::timerCallback()
{
    const int mode = audioProcessor.apvts.getRawParameterValue("ab")->load() > 0.5f ? 1 : 0;
    if (mode != currentAB)
    {
        currentAB = mode;
        repaint();
    }

    localPlayButton.setButtonText(audioProcessor.isReferencePlaying() ? "PAUSE" : "PLAY");
    updateReferenceTransportText();

    metersLabel.setText(
        "MIX  " + juce::String(audioProcessor.getSourceLevelDb(), 1) + " dB     REF  "
        + juce::String(audioProcessor.getReferenceLevelDb(), 1) + " dB",
        juce::dontSendNotification);

    static int spotifyTick = 0;
    if (++spotifyTick >= 10)
    {
        spotifyTick = 0;
        if (spotifyAuth.isConnected()) refreshSpotifyPlayback();
    }
}

void RefMatchAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour::fromRGB(9, 12, 17));

    const float w = (float) getWidth();
    const float h = (float) getHeight();

    juce::ColourGradient violet(juce::Colour::fromRGBA(103, 76, 255, 95),
                                 w * 0.68f, 80.0f,
                                 juce::Colours::transparentBlack,
                                 w * 0.68f, h * 0.55f, true);
    g.setGradientFill(violet);
    g.fillEllipse(w * 0.35f, -150.0f, 600.0f, 380.0f);

    juce::ColourGradient teal(juce::Colour::fromRGBA(32, 220, 207, 65),
                               w * 0.92f, 160.0f,
                               juce::Colours::transparentBlack,
                               w * 0.58f, 330.0f, true);
    g.setGradientFill(teal);
    g.fillEllipse(w * 0.63f, -50.0f, 420.0f, 360.0f);

    auto card = [&g](juce::Rectangle<float> r)
    {
        g.setColour(panel.withAlpha(0.96f));
        g.fillRoundedRectangle(r, 15.0f);
        g.setColour(juce::Colour::fromRGBA(255, 255, 255, 17));
        g.drawRoundedRectangle(r.reduced(0.5f), 15.0f, 1.0f);
    };

    const auto bounds = getLocalBounds().reduced(28);
    const int top = 112;
    const int gap = 16;
    const int leftW = (bounds.getWidth() - gap) * 55 / 100;
    const int rightW = bounds.getWidth() - gap - leftW;

    card(juce::Rectangle<float>((float) bounds.getX(), (float) top, (float) leftW, 246.0f));
    card(juce::Rectangle<float>((float) bounds.getX(), 374.0f, (float) leftW, 330.0f));
    card(juce::Rectangle<float>((float) (bounds.getX() + leftW + gap), (float) top,
                                (float) rightW, 592.0f));

    auto ab = juce::Rectangle<float>((float) bounds.getX() + 18.0f, 276.0f,
                                      (float) leftW - 36.0f, 58.0f);
    const auto half = ab.getWidth() * 0.5f;
    auto active = currentAB == 0 ? ab.withWidth(half)
                                 : ab.withWidth(half).translated(half, 0.0f);

    juce::ColourGradient activeGradient(
        currentAB == 0 ? juce::Colour::fromRGBA(49, 179, 226, 100)
                       : juce::Colour::fromRGBA(116, 82, 255, 115),
        active.getX(), active.getCentreY(),
        currentAB == 0 ? juce::Colour::fromRGBA(84, 88, 255, 80)
                       : juce::Colour::fromRGBA(37, 213, 196, 85),
        active.getRight(), active.getCentreY(), false);
    g.setGradientFill(activeGradient);
    g.fillRoundedRectangle(active, 10.0f);
}

void RefMatchAudioProcessorEditor::resized()
{
    const auto bounds = getLocalBounds().reduced(28);
    logoLabel.setBounds(bounds.getX() + 14, 22, 260, 44);
    subtitleLabel.setBounds(bounds.getX() + 17, 65, 320, 22);
    buildLabel.setBounds(bounds.getRight() - 220, 34, 205, 24);

    const int top = 112;
    const int gap = 16;
    const int leftW = (bounds.getWidth() - gap) * 55 / 100;
    const int rightX = bounds.getX() + leftW + gap;
    const int rightW = bounds.getWidth() - gap - leftW;

    localTitle.setBounds(bounds.getX() + 20, top + 18, 180, 20);
    referenceName.setBounds(bounds.getX() + 20, top + 42, leftW - 40, 30);
    loadButton.setBounds(bounds.getX() + 20, top + 82, 130, 34);
    localPlayButton.setBounds(bounds.getX() + 160, top + 82, 90, 34);
    referenceTime.setBounds(bounds.getX() + 264, top + 87, leftW - 284, 24);
    referencePosition.setBounds(bounds.getX() + 20, top + 124, leftW - 40, 28);

    mixButton.setBounds(bounds.getX() + 18, 282, leftW / 2 - 18, 46);
    referenceButton.setBounds(bounds.getX() + leftW / 2, 282, leftW / 2 - 18, 46);
    levelMatch.setBounds(bounds.getX() + 22, 335, leftW - 44, 30);

    matchTitle.setBounds(bounds.getX() + 20, 392, 120, 20);
    learnButton.setBounds(bounds.getX() + 20, 424, 105, 34);
    resetMatchButton.setBounds(bounds.getX() + 136, 424, 90, 34);
    matchEnabled.setBounds(bounds.getX() + 245, 424, leftW - 267, 34);

    matchAmountLabel.setBounds(bounds.getX() + 20, 478, 100, 20);
    matchAmount.setBounds(bounds.getX() + 20, 499, leftW - 40, 34);
    maxCorrectionLabel.setBounds(bounds.getX() + 20, 545, 100, 20);
    maxCorrection.setBounds(bounds.getX() + 20, 566, leftW - 40, 34);
    bypassButton.setBounds(bounds.getX() + 20, 617, 160, 34);
    metersLabel.setBounds(bounds.getX() + 195, 622, leftW - 215, 24);

    spotifyTitle.setBounds(rightX + 20, top + 18, 180, 20);
    spotifyButton.setBounds(rightX + 20, top + 48, 165, 34);
    spotifyStatus.setBounds(rightX + 198, top + 52, rightW - 218, 26);
    spotifyDevice.setBounds(rightX + 20, top + 98, rightW - 40, 22);
    spotifyNowPlaying.setBounds(rightX + 20, top + 124, rightW - 40, 30);
    spotifyPlayPause.setBounds(rightX + 20, top + 164, 100, 34);

    spotifySearch.setBounds(rightX + 20, top + 230, rightW - 128, 34);
    spotifySearchButton.setBounds(rightX + rightW - 98, top + 230, 78, 34);
    spotifyResults.setBounds(rightX + 20, top + 276, rightW - 40, 36);
    spotifyPlaySelected.setBounds(rightX + 20, top + 324, 150, 34);
    spotifyNote.setBounds(rightX + 20, top + 378, rightW - 40, 48);
}
