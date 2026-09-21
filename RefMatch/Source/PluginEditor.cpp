#include "PluginEditor.h"

AuroraLookAndFeel::AuroraLookAndFeel()
{
    setColour(
        juce::TextButton::textColourOffId,
        juce::Colour::fromRGB(220, 228, 238));

    setColour(
        juce::TextButton::textColourOnId,
        juce::Colours::white);

    setColour(
        juce::ToggleButton::textColourId,
        juce::Colour::fromRGB(205, 214, 226));
}

void AuroraLookAndFeel::drawButtonBackground(
    juce::Graphics& g,
    juce::Button& button,
    const juce::Colour&,
    bool highlighted,
    bool down)
{
    auto bounds =
        button.getLocalBounds().toFloat();

    auto colour =
        juce::Colour::fromRGB(35, 42, 53);

    if (highlighted)
        colour = juce::Colour::fromRGB(45, 55, 70);

    if (down)
        colour = juce::Colour::fromRGB(55, 67, 86);

    g.setColour(colour);
    g.fillRoundedRectangle(bounds, 10.0f);

    g.setColour(
        juce::Colour::fromRGBA(255, 255, 255, 22));

    g.drawRoundedRectangle(
        bounds.reduced(0.5f),
        10.0f,
        1.0f);
}

void AuroraLookAndFeel::drawToggleButton(
    juce::Graphics& g,
    juce::ToggleButton& button,
    bool,
    bool)
{
    auto bounds =
        button.getLocalBounds().toFloat();

    const float toggleWidth = 46.0f;
    const float toggleHeight = 24.0f;

    auto toggleArea =
        juce::Rectangle<float>(
            bounds.getRight() - toggleWidth,
            bounds.getCentreY() - toggleHeight * 0.5f,
            toggleWidth,
            toggleHeight);

    g.setColour(
        button.getToggleState()
            ? juce::Colour::fromRGB(72, 198, 217)
            : juce::Colour::fromRGB(48, 55, 66));

    g.fillRoundedRectangle(
        toggleArea,
        toggleHeight * 0.5f);

    const float dotSize = 18.0f;

    const float dotX =
        button.getToggleState()
            ? toggleArea.getRight() - dotSize - 3.0f
            : toggleArea.getX() + 3.0f;

    g.setColour(juce::Colours::white);

    g.fillEllipse(
        dotX,
        toggleArea.getY() + 3.0f,
        dotSize,
        dotSize);

    g.setColour(
        juce::Colour::fromRGB(215, 222, 232));

    g.setFont(13.0f);

    g.drawText(
        button.getButtonText(),
        bounds.withTrimmedRight(toggleWidth + 14.0f),
        juce::Justification::centredLeft);
}

RefMatchAudioProcessorEditor::RefMatchAudioProcessorEditor(
    RefMatchAudioProcessor& p)
    : AudioProcessorEditor(&p),
      audioProcessor(p)
{
    setSize(900, 600);

    setLookAndFeel(&lookAndFeel);

    logoLabel.setText(
        "RefMatch",
        juce::dontSendNotification);

    logoLabel.setFont(
        juce::Font(34.0f, juce::Font::bold));

    logoLabel.setColour(
        juce::Label::textColourId,
        juce::Colours::white);

    subtitleLabel.setText(
        "REFERENCE  /  A-B  /  MATCH",
        juce::dontSendNotification);

    subtitleLabel.setFont(11.0f);

    subtitleLabel.setColour(
        juce::Label::textColourId,
        juce::Colour::fromRGB(116, 130, 149));

    referenceTitle.setText(
        "REFERENCE",
        juce::dontSendNotification);

    referenceTitle.setFont(
        juce::Font(12.0f, juce::Font::bold));

    referenceTitle.setColour(
        juce::Label::textColourId,
        juce::Colour::fromRGB(122, 137, 157));

    referenceName.setText(
        "No reference loaded",
        juce::dontSendNotification);

    referenceName.setFont(18.0f);

    referenceName.setColour(
        juce::Label::textColourId,
        juce::Colour::fromRGB(231, 236, 244));

    spotifyStatus.setText(
    "NOT CONNECTED",
        juce::dontSendNotification);

    spotifyStatus.setFont(11.0f);

    spotifyStatus.setColour(
        juce::Label::textColourId,
        juce::Colour::fromRGB(116, 130, 149));

    juce::Component* components[] =
    {
        &logoLabel,
        &subtitleLabel,
        &referenceTitle,
        &referenceName,
        &spotifyButton,
        &spotifyStatus,
        &spotifyDevice,
        &spotifyNowPlaying,
        &spotifyPlayPause,
        &loadButton,
        &playButton,
        &mixButton,
        &referenceButton,
        &levelMatch
    };

    for (auto* component : components)
        addAndMakeVisible(component);

    spotifyDevice.setText(
    "Device: —",
    juce::dontSendNotification);

spotifyNowPlaying.setText(
    "Nothing playing",
    juce::dontSendNotification);

    mixButton.setClickingTogglesState(false);
    referenceButton.setClickingTogglesState(false);

  spotifyAuth.onAuthResult =
    [this](bool success, const juce::String& message)
{
    spotifyStatus.setText(
        success ? "CONNECTED" : message,
        juce::dontSendNotification);

    spotifyStatus.setColour(
        juce::Label::textColourId,
        success
            ? juce::Colour::fromRGB(72, 198, 140)
            : juce::Colour::fromRGB(230, 100, 110));

    if (success)
    {
        spotifyClient.setAccessToken(
            spotifyAuth.getAccessToken());

        refreshSpotifyPlayback();
    }
};

    spotifyButton.onClick = [this]
{
    spotifyAuth.startLogin();
};

    spotifyPlayPause.onClick = [this]
{
    spotifyClient.getPlaybackState(
        [this](bool ok, SpotifyPlaybackInfo info)
        {
            if (!ok)
                return;

            if (info.isPlaying)
            {
                spotifyClient.pause(
                    [this](bool)
                    {
                        refreshSpotifyPlayback();
                    });
            }
            else
            {
                spotifyClient.resume(
                    [this](bool)
                    {
                        refreshSpotifyPlayback();
                    });
            }
        });
};

    loadButton.onClick = [this]
    {
        chooser =
            std::make_unique<juce::FileChooser>(
                "Choose reference track",
                juce::File {},
                "*.wav;*.aiff;*.aif;*.mp3;*.flac;*.m4a");

        chooser->launchAsync(
            juce::FileBrowserComponent::openMode
                | juce::FileBrowserComponent::canSelectFiles,
            [this](const juce::FileChooser& fc)
            {
                const auto file = fc.getResult();

                if (file.existsAsFile()
                    && audioProcessor.loadReference(file))
                {
                    referenceName.setText(
                        file.getFileName(),
                        juce::dontSendNotification);
                }
            });
    };

    playButton.onClick = [this]
    {
        referencePlaying = !referencePlaying;

        audioProcessor.setReferencePlaying(
            referencePlaying);

        playButton.setButtonText(
            referencePlaying
                ? "PAUSE"
                : "PLAY");
    };

    mixButton.onClick =
        [this] { setABMode(0); };

    referenceButton.onClick =
        [this] { setABMode(1); };

    levelMatchAttachment =
        std::make_unique<
            juce::AudioProcessorValueTreeState::ButtonAttachment>(
                audioProcessor.apvts,
                "levelmatch",
                levelMatch);

    currentAB =
        audioProcessor.apvts
            .getRawParameterValue("ab")
            ->load() > 0.5f ? 1 : 0;

    startTimerHz(20);
}

RefMatchAudioProcessorEditor::~RefMatchAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void RefMatchAudioProcessorEditor::setABMode(int mode)
{
    currentAB = mode;

    if (auto* parameter =
        audioProcessor.apvts.getParameter("ab"))
    {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost(
            mode == 0 ? 0.0f : 1.0f);
        parameter->endChangeGesture();
    }

    repaint();
}

void RefMatchAudioProcessorEditor::timerCallback()
{
    const int parameterMode =
        audioProcessor.apvts
            .getRawParameterValue("ab")
            ->load() > 0.5f ? 1 : 0;

    if (parameterMode != currentAB)
    {
        currentAB = parameterMode;
        repaint();
    }
}

void RefMatchAudioProcessorEditor::paint(
    juce::Graphics& g)
{
    g.fillAll(
        juce::Colour::fromRGB(10, 13, 18));

    const auto width =
        (float) getWidth();

    const auto height =
        (float) getHeight();

    {
        juce::ColourGradient glow(
            juce::Colour::fromRGBA(90, 72, 255, 95),
            width * 0.70f,
            height * 0.16f,
            juce::Colours::transparentBlack,
            width * 0.70f,
            height * 0.58f,
            true);

        g.setGradientFill(glow);
        g.fillEllipse(
            width * 0.42f,
            -120.0f,
            520.0f,
            330.0f);
    }

    {
        juce::ColourGradient glow(
            juce::Colour::fromRGBA(30, 224, 213, 70),
            width * 0.91f,
            height * 0.25f,
            juce::Colours::transparentBlack,
            width * 0.58f,
            height * 0.40f,
            true);

        g.setGradientFill(glow);
        g.fillEllipse(
            width * 0.62f,
            -30.0f,
            380.0f,
            330.0f);
    }

    {
        juce::ColourGradient glow(
            juce::Colour::fromRGBA(230, 61, 115, 48),
            width * 0.26f,
            height * 0.05f,
            juce::Colours::transparentBlack,
            width * 0.42f,
            height * 0.28f,
            true);

        g.setGradientFill(glow);
        g.fillEllipse(
            40.0f,
            -50.0f,
            470.0f,
            240.0f);
    }

    auto mainPanel =
        juce::Rectangle<float>(
            28.0f,
            118.0f,
            width - 56.0f,
            height - 150.0f);

    g.setColour(
        juce::Colour::fromRGBA(
            25, 31, 40, 235));

    g.fillRoundedRectangle(
        mainPanel,
        18.0f);

    g.setColour(
        juce::Colour::fromRGBA(
            255, 255, 255, 18));

    g.drawRoundedRectangle(
        mainPanel,
        18.0f,
        1.0f);

    auto referenceCard =
        juce::Rectangle<float>(
            55.0f,
            170.0f,
            width - 110.0f,
            130.0f);

    g.setColour(
        juce::Colour::fromRGB(
            19, 24, 32));

    g.fillRoundedRectangle(
        referenceCard,
        14.0f);

    auto abArea =
        juce::Rectangle<float>(
            55.0f,
            335.0f,
            width - 110.0f,
            82.0f);

    g.setColour(
        juce::Colour::fromRGB(
            17, 21, 28));

    g.fillRoundedRectangle(
        abArea,
        14.0f);

    auto leftAB =
        juce::Rectangle<float>(
            65.0f,
            345.0f,
            (width - 140.0f) * 0.5f,
            62.0f);

    auto rightAB =
        leftAB.translated(
            leftAB.getWidth(),
            0.0f);

    if (currentAB == 0)
    {
        juce::ColourGradient active(
            juce::Colour::fromRGBA(
                36, 185, 229, 110),
            leftAB.getX(),
            leftAB.getCentreY(),
            juce::Colour::fromRGBA(
                75, 86, 255, 90),
            leftAB.getRight(),
            leftAB.getCentreY(),
            false);

        g.setGradientFill(active);
        g.fillRoundedRectangle(
            leftAB,
            11.0f);
    }
    else
    {
        juce::ColourGradient active(
            juce::Colour::fromRGBA(
                120, 76, 255, 120),
            rightAB.getX(),
            rightAB.getCentreY(),
            juce::Colour::fromRGBA(
                34, 218, 199, 90),
            rightAB.getRight(),
            rightAB.getCentreY(),
            false);

        g.setGradientFill(active);
        g.fillRoundedRectangle(
            rightAB,
            11.0f);
    }
}

void RefMatchAudioProcessorEditor::resized()
{
    logoLabel.setBounds(
        44, 28, 260, 44);

    subtitleLabel.setBounds(
        47, 72, 300, 22);

    referenceTitle.setBounds(
        74, 184, 180, 20);

    referenceName.setBounds(
        74, 207, 480, 32);

    spotifyButton.setBounds(
        74, 250, 180, 34);

    spotifyStatus.setBounds(
        270, 250, 150, 34);
    
    spotifyDevice.setBounds(
        140,
        290,
        220,
        24);

    spotifyNowPlaying.setBounds(
        140,
        318,
        420,
        28);

    spotifyPlayPause.setBounds(
        580,
        300,
        150,
        34);
    
    loadButton.setBounds(
    430, 250, 140, 34);

    playButton.setBounds(
        getWidth() - 190,
        250,
        110,
        34);

    mixButton.setBounds(
        75,
        353,
        (getWidth() - 150) / 2,
        46);

    referenceButton.setBounds(
        getWidth() / 2,
        353,
        (getWidth() - 150) / 2,
        46);

    levelMatch.setBounds(
        72,
        455,
        getWidth() - 144,
        40);
}
void RefMatchAudioProcessorEditor::refreshSpotifyPlayback()
{
    spotifyClient.getPlaybackState(
        [this](bool ok, SpotifyPlaybackInfo info)
        {
            if (!ok)
                return;

            spotifyDevice.setText(
                "Device: " + info.deviceName,
                juce::dontSendNotification);

            spotifyNowPlaying.setText(
                info.trackName + " — " + info.artistName,
                juce::dontSendNotification);

            spotifyPlayPause.setButtonText(
                info.isPlaying ? "PAUSE" : "PLAY");
        });
}
