#include "PluginEditor.h"
#include <cmath>
#include <initializer_list>

namespace
{
constexpr auto bg0 = 0xff05070c;
constexpr auto bg1 = 0xff0a0f18;
constexpr auto panel = 0xff0e1520;
constexpr auto panelHi = 0xff121c2b;
constexpr auto border = 0xff243247;
constexpr auto text = 0xfff5f7fb;
constexpr auto muted = 0xff8090a7;
constexpr auto cyan = 0xff55e1f0;
constexpr auto blue = 0xff4d7cff;
constexpr auto violet = 0xff9f73ff;
constexpr auto green = 0xff67e6ad;
constexpr auto amber = 0xffffb65e;

void setupLabel(juce::Label& l, const juce::String& s, float size, juce::Colour colour, bool bold = false,
                juce::Justification justification = juce::Justification::centredLeft)
{
    l.setText(s, juce::dontSendNotification);
    l.setColour(juce::Label::textColourId, colour);
    l.setFont(juce::Font(juce::FontOptions(size, bold ? juce::Font::bold : juce::Font::plain)));
    l.setJustificationType(justification);
    l.setInterceptsMouseClicks(false, false);
}
}

RefMatchLookAndFeel::RefMatchLookAndFeel()
{
    setColour(juce::TextButton::buttonColourId, juce::Colour(0xff182337));
    setColour(juce::TextButton::buttonOnColourId, juce::Colour(cyan));
    setColour(juce::TextButton::textColourOffId, juce::Colour(text));
    setColour(juce::TextButton::textColourOnId, juce::Colour(bg0));
    setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff090f18));
    setColour(juce::TextEditor::outlineColourId, juce::Colour(border));
    setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(cyan));
    setColour(juce::TextEditor::textColourId, juce::Colour(text));
    setColour(juce::TextEditor::highlightColourId, juce::Colour(blue).withAlpha(0.45f));
    setColour(juce::Slider::textBoxTextColourId, juce::Colour(text));
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
}

void RefMatchLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour& base,
                                                bool hover, bool down)
{
    auto r = button.getLocalBounds().toFloat().reduced(0.5f);
    auto c = base;
    if (hover) c = c.brighter(0.12f);
    if (down) c = c.darker(0.15f);
    g.setColour(c);
    g.fillRoundedRectangle(r, 10.0f);
    g.setColour(juce::Colour(border).withAlpha(0.9f));
    g.drawRoundedRectangle(r, 10.0f, 1.0f);
}

void RefMatchLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& b, bool, bool)
{
    auto r = b.getLocalBounds().toFloat();
    g.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
    g.setColour(juce::Colour(text));
    g.drawText(b.getButtonText(), r.removeFromLeft(r.getWidth() - 48.0f), juce::Justification::centredLeft);
    auto sw = r.withSizeKeepingCentre(42.0f, 22.0f);
    g.setColour(b.getToggleState() ? juce::Colour(cyan) : juce::Colour(0xff2b3749));
    g.fillRoundedRectangle(sw, 11.0f);
    g.setColour(juce::Colours::white);
    const float x = b.getToggleState() ? sw.getRight() - 19.0f : sw.getX() + 3.0f;
    g.fillEllipse(x, sw.getY() + 3.0f, 16.0f, 16.0f);
}

void RefMatchLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int w, int h, float pos,
                                            float, float, juce::Slider::SliderStyle, juce::Slider&)
{
    auto track = juce::Rectangle<float>((float)x, (float)y + h * 0.5f - 2.0f, (float)w, 4.0f);
    g.setColour(juce::Colour(0xff253247));
    g.fillRoundedRectangle(track, 2.0f);
    auto active = track.withWidth(juce::jlimit(0.0f, track.getWidth(), pos - track.getX()));
    juce::ColourGradient grad(juce::Colour(blue), active.getX(), active.getY(), juce::Colour(cyan), active.getRight(), active.getY(), false);
    g.setGradientFill(grad);
    g.fillRoundedRectangle(active, 2.0f);
    g.setColour(juce::Colour(text));
    g.fillEllipse(pos - 6.0f, track.getCentreY() - 6.0f, 12.0f, 12.0f);
}

RefMatchAudioProcessorEditor::RefMatchAudioProcessorEditor(RefMatchAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), spotify(p.getSpotifyController())
{
    setLookAndFeel(&lookAndFeel);
    setSize(1180, 780);
    setResizable(true, true);
    setResizeLimits(1100, 780, 1600, 1050);

    setupLabel(logoLabel, "RefMatch", 31.0f, juce::Colour(text), true);
    setupLabel(versionLabel, "0.4.3  |  STREAM MODE", 10.5f, juce::Colour(cyan), true, juce::Justification::centredRight);
    setupLabel(straplineLabel, "LISTEN. COMPARE. MATCH.", 10.5f, juce::Colour(muted));
    setupLabel(trackLabel, "Nothing playing", 19.0f, juce::Colour(text), true);
    setupLabel(artistLabel, "Spotify desktop", 12.0f, juce::Colour(muted));
    setupLabel(spotifyStateLabel, "SPOTIFY OPTIONAL", 10.5f, juce::Colour(amber), true);
    setupLabel(captureStateLabel, "SYSTEM AUDIO OFF", 10.5f, juce::Colour(muted), true);
    setupLabel(listeningLabel, "NOW: YOUR MIX", 12.0f, juce::Colour(cyan), true, juce::Justification::centred);
    setupLabel(gainValueLabel, "0.0 dB", 11.0f, juce::Colour(muted), true, juce::Justification::centredRight);
    setupLabel(matchTitleLabel, "MATCH EQ", 11.0f, juce::Colour(muted), true);
    setupLabel(sourceGainLabel, "MIX GAIN", 10.0f, juce::Colour(muted), true);
    setupLabel(matchAmountLabel, "AMOUNT", 10.0f, juce::Colour(muted), true);
    setupLabel(maxCorrectionLabel, "MAX +/- dB", 10.0f, juce::Colour(muted), true);

    for (auto* c : std::initializer_list<juce::Component*> { &logoLabel, &versionLabel, &straplineLabel,
                      &trackLabel, &artistLabel, &spotifyStateLabel, &captureStateLabel,
                      &enableSpotifyButton, &permissionButton, &playPauseButton, &prevButton, &nextButton,
                      &searchBox, &searchButton, &switchButton, &listeningLabel,
                      &autoGainButton, &gainValueLabel, &matchTitleLabel, &learnButton,
                      &resetButton, &matchToggle, &bypassToggle, &sourceGainSlider,
                      &matchAmountSlider, &maxCorrectionSlider, &sourceGainLabel,
                      &matchAmountLabel, &maxCorrectionLabel })
        addAndMakeVisible(c);

    switchButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff173b46));
    switchButton.setColour(juce::TextButton::textColourOffId, juce::Colour(cyan));
    autoGainButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff25213d));

    searchBox.setTextToShowWhenEmpty("Search Spotify...", juce::Colour(muted));
    searchBox.setReturnKeyStartsNewLine(false);
    searchBox.onReturnKey = [this] { openSpotifySearch(); };

    for (auto* slider : { &sourceGainSlider, &matchAmountSlider, &maxCorrectionSlider })
    {
        slider->setSliderStyle(juce::Slider::LinearHorizontal);
        slider->setTextBoxStyle(juce::Slider::TextBoxRight, false, 64, 24);
    }

    sourceGainSlider.setTextValueSuffix(" dB");
    matchAmountSlider.setTextValueSuffix(" %");
    maxCorrectionSlider.setTextValueSuffix(" dB");

    sourceGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "sourcegain", sourceGainSlider);
    matchEnabledAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.apvts, "matchenabled", matchToggle);
    matchAmountAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "matchamount", matchAmountSlider);
    maxCorrectionAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "maxcorrection", maxCorrectionSlider);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.apvts, "bypass", bypassToggle);

    enableSpotifyButton.onClick = [this] { enableSpotifyControl(); };
    searchButton.onClick = [this] { openSpotifySearch(); };
    switchButton.onClick = [this] { switchSource(); };
    autoGainButton.onClick = [this]
    {
        audioProcessor.autoGainMatch();
        gainValueLabel.setText(juce::String(audioProcessor.getSourceGainDb(), 1) + " dB", juce::dontSendNotification);
    };
    learnButton.onClick = [this] { audioProcessor.learnMatch(); };
    resetButton.onClick = [this] { audioProcessor.clearMatch(); };

    permissionButton.onClick = [this] { spotify.openAutomationSettings(); };
    playPauseButton.onClick = [this] { runSpotifyCommand(SpotifyDesktopController::Command::playPause); };
    prevButton.onClick = [this] { runSpotifyCommand(SpotifyDesktopController::Command::previous); };
    nextButton.onClick = [this] { runSpotifyCommand(SpotifyDesktopController::Command::next); };
    enableSpotifyButton.setTooltip("Optional Spotify control using your host’s Automation permission.");
    permissionButton.setTooltip("Open macOS Automation settings if Spotify control was denied.");
    switchButton.setTooltip("Fade MIX out, then play Spotify. Return pauses Spotify before restoring MIX.");
    captureStateLabel.setTooltip("System audio is used for measurement only. The external source plays directly to your audio device.");

    updateSourceState();
    startTimerHz(20);
} 

RefMatchAudioProcessorEditor::~RefMatchAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void RefMatchAudioProcessorEditor::enableSpotifyControl()
{
    if (spotify.isBusy() || audioProcessor.isTransportPending()) return;
    spotifyError = false;
    spotifyStateLabel.setText("WAITING FOR SPOTIFY PERMISSION...", juce::dontSendNotification);
    runSpotifyCommand(SpotifyDesktopController::Command::authorise);
}

void RefMatchAudioProcessorEditor::showSpotifyError(const juce::String& error)
{
    spotifyError = true;
    spotifyStateLabel.setText(error, juce::dontSendNotification);
    spotifyStateLabel.setTooltip(error);
    spotifyStateLabel.setColour(juce::Label::textColourId, juce::Colour(amber));
}

void RefMatchAudioProcessorEditor::applySpotifyInfo(const SpotifyDesktopInfo& info)
{
    spotifyInfo = info;
    spotifyControlReady = info.authorised && !spotifyError;
    trackLabel.setText(info.track.isNotEmpty() ? info.track : "Choose a track in Spotify", juce::dontSendNotification);
    artistLabel.setText(info.artist.isNotEmpty() ? info.artist : "Spotify desktop", juce::dontSendNotification);
    trackLabel.setTooltip(info.track);
    artistLabel.setTooltip(info.artist);
    playPauseButton.setButtonText(info.playing ? "PAUSE" : "PLAY");
    enableSpotifyButton.setButtonText(spotifyControlReady ? "RECONNECT" : "CONNECT SPOTIFY");
    if (!spotifyError)
    {
        spotifyStateLabel.setText(info.authorised ? "SPOTIFY CONNECTED" : "SPOTIFY OPTIONAL", juce::dontSendNotification);
        spotifyStateLabel.setColour(juce::Label::textColourId, juce::Colour(info.authorised ? green : muted));
    }
}

void RefMatchAudioProcessorEditor::runSpotifyCommand(SpotifyDesktopController::Command command)
{
    if (spotify.isBusy() || audioProcessor.isTransportPending()) return;
    juce::Component::SafePointer<RefMatchAudioProcessorEditor> safe(this);
    spotify.request(command, [safe](bool ok, SpotifyDesktopInfo info, juce::String error)
    {
        if (!safe) return;
        safe->spotifyError = !ok;
        safe->applySpotifyInfo(info);
        if (!ok || !info.playing) safe->audioProcessor.setReferenceSelected(false);
        if (!ok) safe->showSpotifyError(error);
        safe->updateSourceState();
    });
    updateSourceState();
}

void RefMatchAudioProcessorEditor::openSpotifySearch()
{
    juce::String error;
    if (!spotify.openSearch(searchBox.getText(), error))
        showSpotifyError(error);
}

void RefMatchAudioProcessorEditor::switchSource()
{
    audioProcessor.switchWithSpotify();
    updateSourceState();
}

void RefMatchAudioProcessorEditor::updateSpotifyInfo()
{
    // No permission prompts or scripts from the timer before explicit CONNECT.
    if (!spotifyControlReady || spotifyError || spotify.isBusy() || audioProcessor.isTransportPending()) return;
    runSpotifyCommand(SpotifyDesktopController::Command::status);
}

void RefMatchAudioProcessorEditor::updateSourceState()
{
    const bool ref = audioProcessor.isReferenceSelected();
    const bool busy = spotify.isBusy() || audioProcessor.isTransportPending();
    listeningLabel.setText(audioProcessor.hasReferenceFailure() ? "MIX RESTORED - SELECT MIX TO RESET"
                              : ref ? "REF SELECTED" : "MIX SELECTED", juce::dontSendNotification);
    listeningLabel.setColour(juce::Label::textColourId, juce::Colour(ref ? violet : cyan));
    switchButton.setButtonText(audioProcessor.isTransportPending() ? "RETURN TO MIX" : ref ? "SWITCH TO MIX" : audioProcessor.isReferenceCaptureRunning() ? "SWITCH TO REF" : "ENABLE SYSTEM AUDIO");
    switchButton.setEnabled(true);
    enableSpotifyButton.setEnabled(!busy);
    playPauseButton.setEnabled(!busy && spotifyControlReady);
    prevButton.setEnabled(!busy && spotifyControlReady);
    nextButton.setEnabled(!busy && spotifyControlReady);
    autoGainButton.setEnabled(audioProcessor.hasReferenceAudio());
    learnButton.setEnabled(audioProcessor.hasReferenceAudio());

}

void RefMatchAudioProcessorEditor::timerCallback()
{
    ++timerTicks;
    const auto transportError = audioProcessor.getTransportError();
    if (transportError.isNotEmpty()) showSpotifyError(transportError);
    if (timerTicks % 20 == 0)
        updateSpotifyInfo();

    const auto captureStatus = audioProcessor.getReferenceCaptureStatus();
    captureStateLabel.setText(audioProcessor.hasReferenceFailure() ? "MIX RESTORED - CHECK REFERENCE"
                              : audioProcessor.isReferenceCaptureRunning() ? "SYSTEM AUDIO READY"
                              : audioProcessor.isReferenceCaptureStarting() ? "STARTING AUDIO CAPTURE..."
                              : "SYSTEM AUDIO OFF - SWITCH TO ENABLE", juce::dontSendNotification);
    captureStateLabel.setTooltip(captureStatus);

    captureStateLabel.setColour(juce::Label::textColourId,
                                audioProcessor.isReferenceCaptureRunning() ? juce::Colour(green) : juce::Colour(muted));
    gainValueLabel.setText(juce::String(audioProcessor.getSourceGainDb(), 1) + " dB", juce::dontSendNotification);
    updateSourceState();
    repaint();
}

void RefMatchAudioProcessorEditor::drawPill(juce::Graphics& g, juce::Rectangle<float> r, const juce::String& s,
                                             juce::Colour c, bool filled)
{
    g.setColour(filled ? c.withAlpha(0.15f) : juce::Colour(0xff0a1019));
    g.fillRoundedRectangle(r, r.getHeight() * 0.5f);
    g.setColour(c.withAlpha(0.7f));
    g.drawRoundedRectangle(r, r.getHeight() * 0.5f, 1.0f);
    g.setColour(c);
    g.setFont(juce::Font(juce::FontOptions(9.5f, juce::Font::bold)));
    g.drawText(s, r, juce::Justification::centred);
}

void RefMatchAudioProcessorEditor::drawPeakMeter(juce::Graphics& g, juce::Rectangle<float> r, float db,
                                                  const juce::String& label, juce::Colour colour)
{
    g.setColour(juce::Colour(0xff090f18));
    g.fillRoundedRectangle(r, 7.0f);
    const float norm = juce::jlimit(0.0f, 1.0f, juce::jmap(db, -60.0f, 0.0f, 0.0f, 1.0f));
    auto fill = r.reduced(3.0f);
    fill.setWidth(fill.getWidth() * norm);
    g.setColour(db > -2.0f ? juce::Colour(amber) : colour);
    g.fillRoundedRectangle(fill, 4.0f);
    g.setFont(juce::Font(juce::FontOptions(10.5f, juce::Font::bold)));
    g.setColour(juce::Colour(text));
    g.drawText(label + "  " + juce::String(db, 1) + " dB", r.reduced(8.0f), juce::Justification::centredLeft);
}

void RefMatchAudioProcessorEditor::drawSpectrum(juce::Graphics& g, juce::Rectangle<float> r)
{
    const auto mix = audioProcessor.getSourceSpectrum();
    const auto ref = audioProcessor.getReferenceSpectrum();
    const auto diff = audioProcessor.getDifferenceSpectrum();
    const float sr = (float)juce::jmax(22050.0, audioProcessor.getSampleRateForDisplay());

    g.setColour(juce::Colour(0xff07101a));
    g.fillRoundedRectangle(r, 14.0f);

    for (int i = 1; i < 6; ++i)
    {
        const float y = r.getY() + r.getHeight() * i / 6.0f;
        g.setColour(juce::Colour(0xff172335));
        g.drawHorizontalLine((int)y, r.getX() + 12.0f, r.getRight() - 12.0f);
    }

    auto xForHz = [r](float hz)
    {
        const float lo = std::log10(20.0f), hi = std::log10(20000.0f);
        return juce::jmap(std::log10(juce::jlimit(20.0f, 20000.0f, hz)), lo, hi, r.getX() + 12.0f, r.getRight() - 12.0f);
    };
    auto yForDb = [r](float db)
    {
        return juce::jmap(juce::jlimit(-90.0f, 0.0f, db), -90.0f, 0.0f, r.getBottom() - 24.0f, r.getY() + 22.0f);
    };

    auto makePath = [&](const auto& data)
    {
        juce::Path p;
        bool started = false;
        for (int i = 2; i < SpectrumAnalyser::bins; i += 2)
        {
            const float hz = (float)i * sr / (float)SpectrumAnalyser::fftSize;
            if (hz < 20.0f || hz > 20000.0f) continue;
            const float x = xForHz(hz), y = yForDb(data[(size_t)i]);
            if (!started) { p.startNewSubPath(x, y); started = true; }
            else p.lineTo(x, y);
        }
        return p;
    };

    auto mixPath = makePath(mix);
    auto refPath = makePath(ref);
    g.setColour(juce::Colour(cyan).withAlpha(0.22f));
    g.strokePath(mixPath, juce::PathStrokeType(7.0f));
    g.setColour(juce::Colour(cyan));
    g.strokePath(mixPath, juce::PathStrokeType(1.9f));
    g.setColour(juce::Colour(violet).withAlpha(0.18f));
    g.strokePath(refPath, juce::PathStrokeType(7.0f));
    g.setColour(juce::Colour(violet));
    g.strokePath(refPath, juce::PathStrokeType(1.9f));

    juce::Path diffPath;
    bool ds = false;
    for (int i = 2; i < SpectrumAnalyser::bins; i += 2)
    {
        const float hz = (float)i * sr / (float)SpectrumAnalyser::fftSize;
        if (hz < 20.0f || hz > 20000.0f) continue;
        const float x = xForHz(hz);
        const float y = juce::jmap(juce::jlimit(-18.0f, 18.0f, diff[(size_t)i]), -18.0f, 18.0f, r.getCentreY() + 58.0f, r.getCentreY() - 58.0f);
        if (!ds) { diffPath.startNewSubPath(x, y); ds = true; }
        else diffPath.lineTo(x, y);
    }
    g.setColour(juce::Colour(green).withAlpha(0.82f));
    g.strokePath(diffPath, juce::PathStrokeType(1.35f));

    drawPill(g, { r.getX() + 14.0f, r.getY() + 12.0f, 58.0f, 22.0f }, "MIX", juce::Colour(cyan), true);
    drawPill(g, { r.getX() + 78.0f, r.getY() + 12.0f, 58.0f, 22.0f }, "REF", juce::Colour(violet), true);
    drawPill(g, { r.getX() + 142.0f, r.getY() + 12.0f, 58.0f, 22.0f }, "DIFF", juce::Colour(green), true);

    g.setFont(juce::Font(juce::FontOptions(9.5f)));
    g.setColour(juce::Colour(muted));
    for (auto hz : { 20, 50, 100, 500, 1000, 5000, 10000, 20000 })
    {
        auto label = hz >= 1000 ? juce::String(hz / 1000) + "k" : juce::String(hz);
        g.drawText(label, (int)xForHz((float)hz) - 18, (int)r.getBottom() - 18, 36, 13, juce::Justification::centred);
    }
}

void RefMatchAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(bg0));

    juce::ColourGradient header(juce::Colour(0xff182343), 0, 0,
                                juce::Colour(0xff082b32), (float)getWidth(), 0, false);
    header.addColour(0.50, juce::Colour(0xff24183f));
    g.setGradientFill(header);
    g.fillRect(0, 0, getWidth(), 128);

    g.setColour(juce::Colour(0x22111111));
    g.fillEllipse((float)getWidth() - 240.0f, -80.0f, 340.0f, 240.0f);

    const float margin = 30.0f;
    const float sidebarW = 326.0f;
    const float gap = 18.0f;
    const float leftW = (float)getWidth() - margin * 2.0f - sidebarW - gap;

    auto spectrumCard = juce::Rectangle<float>(margin, 154.0f, leftW, 386.0f);
    g.setColour(juce::Colour(panel));
    g.fillRoundedRectangle(spectrumCard, 16.0f);
    g.setColour(juce::Colour(border));
    g.drawRoundedRectangle(spectrumCard, 16.0f, 1.0f);
    g.setColour(juce::Colour(text));
    g.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
    g.drawText("TONAL COMPARISON", spectrumCard.getX() + 18.0f, spectrumCard.getY() + 14.0f, 190.0f, 20.0f, juce::Justification::left);
    g.setColour(juce::Colour(muted));
    g.setFont(juce::Font(juce::FontOptions(10.0f)));
    g.drawText("Live mix vs. system reference", spectrumCard.getX() + 18.0f, spectrumCard.getY() + 33.0f, 220.0f, 18.0f, juce::Justification::left);

    auto graph = spectrumCard.reduced(16.0f);
    graph.removeFromTop(50.0f);
    graph.removeFromBottom(50.0f);
    drawSpectrum(g, graph);

    auto meters = spectrumCard.reduced(16.0f);
    meters.removeFromTop(spectrumCard.getHeight() - 46.0f);
    const float half = (meters.getWidth() - 10.0f) * 0.5f;
    drawPeakMeter(g, meters.removeFromLeft(half), audioProcessor.getSourcePeakDb(), "MIX PEAK", juce::Colour(cyan));
    meters.removeFromLeft(10.0f);
    drawPeakMeter(g, meters, audioProcessor.getReferencePeakDb(), "REF PEAK", juce::Colour(violet));

    auto matchCard = juce::Rectangle<float>(margin, 558.0f, leftW, (float)getHeight() - 588.0f);
    g.setColour(juce::Colour(panel));
    g.fillRoundedRectangle(matchCard, 16.0f);
    g.setColour(juce::Colour(border));
    g.drawRoundedRectangle(matchCard, 16.0f, 1.0f);

    auto side = juce::Rectangle<float>(margin + leftW + gap, 154.0f, sidebarW, (float)getHeight() - 184.0f);
    g.setColour(juce::Colour(panel));
    g.fillRoundedRectangle(side, 16.0f);
    g.setColour(juce::Colour(border));
    g.drawRoundedRectangle(side, 16.0f, 1.0f);

    g.setColour(juce::Colour(panelHi));
    g.fillRoundedRectangle(side.withHeight(148.0f), 16.0f);

    drawPill(g, { side.getX() + 18.0f, side.getY() + 16.0f, 104.0f, 24.0f }, "SPOTIFY DESKTOP", juce::Colour(green), true);

    // Scrub/progress display (read-only for the first local-control build).
    auto progress = juce::Rectangle<float>(side.getX() + 18.0f, side.getY() + 113.0f, side.getWidth() - 36.0f, 4.0f);
    g.setColour(juce::Colour(0xff263247));
    g.fillRoundedRectangle(progress, 2.0f);
    const float ratio = spotifyInfo.durationSeconds > 0.0 ? (float)juce::jlimit(0.0, 1.0, spotifyInfo.positionSeconds / spotifyInfo.durationSeconds) : 0.0f;
    g.setColour(juce::Colour(violet));
    g.fillRoundedRectangle(progress.withWidth(progress.getWidth() * ratio), 2.0f);

    g.setColour(juce::Colour(muted));
    g.setFont(juce::Font(juce::FontOptions(10.0f)));
    g.drawText("SWITCH controls Spotify play / pause", side.getX() + 18.0f, 686.0f,
               side.getWidth() - 36.0f, 20.0f, juce::Justification::centred);

    // Large A/B state indicator.
    auto ab = juce::Rectangle<float>(side.getX() + 18.0f, side.getY() + 365.0f, side.getWidth() - 36.0f, 96.0f);
    g.setColour(audioProcessor.isReferenceSelected() ? juce::Colour(violet).withAlpha(0.10f) : juce::Colour(cyan).withAlpha(0.10f));
    g.fillRoundedRectangle(ab, 13.0f);
    g.setColour(audioProcessor.isReferenceSelected() ? juce::Colour(violet).withAlpha(0.5f) : juce::Colour(cyan).withAlpha(0.5f));
    g.drawRoundedRectangle(ab, 13.0f, 1.0f);
}

void RefMatchAudioProcessorEditor::resized()
{
    const int w = getWidth();
    const int h = getHeight();
    const int margin = 30;
    const int sidebarW = 326;
    const int gap = 18;
    const int leftW = w - margin * 2 - sidebarW - gap;
    const int sideX = margin + leftW + gap;

    logoLabel.setBounds(margin, 22, 250, 40);
    straplineLabel.setBounds(margin + 2, 66, 390, 22);
    versionLabel.setBounds(w - 260, 33, 225, 20);

    // Sidebar: now playing
    trackLabel.setBounds(sideX + 18, 199, sidebarW - 36, 28);
    artistLabel.setBounds(sideX + 18, 227, sidebarW - 36, 20);
    spotifyStateLabel.setBounds(sideX + 18, 286, sidebarW - 36, 48);
    spotifyStateLabel.setMinimumHorizontalScale(1.0f);
    spotifyStateLabel.setJustificationType(juce::Justification::topLeft);

    captureStateLabel.setBounds(sideX + 18, 656, sidebarW - 36, 24);

    enableSpotifyButton.setBounds(sideX + 18, 342, sidebarW - 150, 32);
    permissionButton.setBounds(sideX + sidebarW - 122, 342, 104, 32);

    prevButton.setBounds(sideX + 18, 386, 44, 34);
    playPauseButton.setBounds(sideX + 70, 386, sidebarW - 140, 34);
    nextButton.setBounds(sideX + sidebarW - 62, 386, 44, 34);

    searchBox.setBounds(sideX + 18, 434, sidebarW - 36, 34);
    searchButton.setBounds(sideX + 18, 475, sidebarW - 36, 34);

    switchButton.setBounds(sideX + 18, 528, sidebarW - 36, 54);
    listeningLabel.setBounds(sideX + 18, 588, sidebarW - 36, 24);
    autoGainButton.setBounds(sideX + 18, 616, 154, 36);
    gainValueLabel.setBounds(sideX + sidebarW - 116, 622, 98, 24);

    // Match panel
    const int matchY = 576;
    matchTitleLabel.setBounds(margin + 18, matchY, 100, 22);
    learnButton.setBounds(margin + 18, matchY + 32, 92, 34);
    resetButton.setBounds(margin + 118, matchY + 32, 92, 34);
    matchToggle.setBounds(margin + 226, matchY + 34, 128, 30);
    bypassToggle.setBounds(margin + 370, matchY + 34, 110, 30);

    const int sliderX = margin + 18;
    const int sliderW = juce::jmax(260, leftW - 36);
    sourceGainLabel.setBounds(sliderX, matchY + 78, 90, 18);
    sourceGainSlider.setBounds(sliderX + 82, matchY + 73, sliderW - 82, 28);
    matchAmountLabel.setBounds(sliderX, matchY + 111, 90, 18);
    matchAmountSlider.setBounds(sliderX + 82, matchY + 106, sliderW - 82, 28);
    maxCorrectionLabel.setBounds(sliderX, matchY + 144, 90, 18);
    maxCorrectionSlider.setBounds(sliderX + 82, matchY + 139, sliderW - 82, 28);

    if (h < 760)
    {
        // On the minimum-height layout, keep the last slider visible.
        maxCorrectionSlider.setBounds(sliderX + 82, h - 37, sliderW - 82, 28);
        maxCorrectionLabel.setBounds(sliderX, h - 32, 90, 18);
    }
}
