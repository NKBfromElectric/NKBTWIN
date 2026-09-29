#include "PluginEditor.h"

namespace
{
const std::array<const char*, 4> ampParameterIds{ "volume", "treble", "middle", "bass" };
const std::array<const char*, 4> ampControlLabels{ "VOLUME", "TREBLE", "MIDDLE", "BASS" };
const std::array<const char*, 3> pedalParameterIds{ "pedalLevel", "pedalDrive", "pedalTone" };
const std::array<const char*, 3> pedalControlLabels{ "LEVEL", "GAIN", "TONE" };
const std::array<const char*, 3> od3ParameterIds{ "od3Level", "od3Drive", "od3Tone" };
const std::array<const char*, 3> od3ControlLabels{ "LEVEL", "DRIVE", "TONE" };
const std::array<const char*, 3> pageNames{ "EFFECTS", "AMP", "CAB" };

juce::Colour background() { return juce::Colour::fromRGB(15, 18, 24); }
juce::Colour surface() { return juce::Colour::fromRGB(27, 32, 41); }
juce::Colour surfaceLight() { return juce::Colour::fromRGB(42, 49, 60); }
juce::Colour lettering() { return juce::Colour::fromRGB(229, 233, 239); }
juce::Colour muted() { return juce::Colour::fromRGB(143, 153, 167); }
juce::Colour accent() { return juce::Colour::fromRGB(88, 185, 197); }
juce::Colour amber() { return juce::Colour::fromRGB(215, 160, 88); }
juce::Colour warmWhite() { return juce::Colour::fromRGB(249, 239, 210); }

void drawSpeaker(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    g.setColour(juce::Colour::fromRGB(26, 28, 31));
    g.fillEllipse(bounds.expanded(7.0f));
    g.setColour(juce::Colour::fromRGB(169, 172, 170));
    g.drawEllipse(bounds.expanded(5.0f), 2.0f);
    g.setGradientFill(juce::ColourGradient(juce::Colour::fromRGB(100, 106, 108),
                                           bounds.getX(), bounds.getY(),
                                           juce::Colour::fromRGB(30, 34, 39),
                                           bounds.getRight(), bounds.getBottom(), false));
    g.fillEllipse(bounds);
    juce::Path speakerMask;
    speakerMask.addEllipse(bounds);
    g.saveState();
    g.reduceClipRegion(speakerMask);
    g.setColour(juce::Colour::fromRGB(180, 181, 172).withAlpha(0.22f));
    for (auto offset = -bounds.getHeight(); offset < bounds.getWidth(); offset += 7.0f)
        g.drawLine(bounds.getX() + offset, bounds.getY(),
                   bounds.getX() + offset + bounds.getHeight(), bounds.getBottom(), 0.7f);
    g.restoreState();
    g.setColour(juce::Colour::fromRGB(20, 22, 26));
    g.fillEllipse(bounds.reduced(bounds.getWidth() * 0.34f));
    g.setColour(juce::Colour::fromRGB(171, 174, 170));
    g.drawEllipse(bounds.reduced(bounds.getWidth() * 0.34f), 1.5f);
    g.setColour(juce::Colour::fromRGB(72, 78, 82));
    g.fillEllipse(bounds.withSizeKeepingCentre(bounds.getWidth() * 0.22f, bounds.getHeight() * 0.22f));
}
}

NkbTwinAudioProcessorEditor::NkbTwinAudioProcessorEditor(NkbTwinAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setLookAndFeel(&ampLookAndFeel);
    setResizable(false, false);
    setSize(900, 560);

    for (size_t index = 0; index < ampKnobs.size(); ++index)
    {
        configureKnob(ampKnobs[index], ampLabels[index], ampControlLabels[index]);
        ampKnobs[index].setName("ampKnob");
        ampAttachments[index] = std::make_unique<NkbTwinAudioProcessor::APVTS::SliderAttachment>(
            processor.parameters, ampParameterIds[index], ampKnobs[index]);
    }

    for (size_t index = 0; index < pedalKnobs.size(); ++index)
    {
        configureKnob(pedalKnobs[index], pedalLabels[index], pedalControlLabels[index]);
        pedalKnobs[index].setName("pedalKnob");
        pedalAttachments[index] = std::make_unique<NkbTwinAudioProcessor::APVTS::SliderAttachment>(
            processor.parameters, pedalParameterIds[index], pedalKnobs[index]);
    }

    for (size_t index = 0; index < od3Knobs.size(); ++index)
    {
        configureKnob(od3Knobs[index], od3Labels[index], od3ControlLabels[index]);
        od3Knobs[index].setName("pedalKnob");
        od3Attachments[index] = std::make_unique<NkbTwinAudioProcessor::APVTS::SliderAttachment>(
            processor.parameters, od3ParameterIds[index], od3Knobs[index]);
    }

    for (size_t index = 0; index < pageButtons.size(); ++index)
    {
        pageButtons[index].setButtonText(pageNames[index]);
        pageButtons[index].onClick = [this, index]
        {
            setPage(index == 0 ? Page::effects : index == 1 ? Page::amp : Page::cabinet);
        };
        addAndMakeVisible(pageButtons[index]);
    }

    brightButton.setButtonText({});
    brightButton.setName("brightSwitch");
    brightButton.setClickingTogglesState(true);
    brightButton.setLookAndFeel(&ampLookAndFeel);
    addAndMakeVisible(brightButton);
    brightAttachment = std::make_unique<NkbTwinAudioProcessor::APVTS::ButtonAttachment>(
        processor.parameters, "bright", brightButton);

    pedalEnableButton.setButtonText({});
    pedalEnableButton.setName("pedalFootswitch");
    pedalEnableButton.setTooltip("Toggle the Blues Driver effect");
    pedalEnableButton.setClickingTogglesState(true);
    pedalEnableButton.setLookAndFeel(&ampLookAndFeel);
    addAndMakeVisible(pedalEnableButton);
    pedalEnableAttachment = std::make_unique<NkbTwinAudioProcessor::APVTS::ButtonAttachment>(
        processor.parameters, "pedalEnabled", pedalEnableButton);

    od3EnableButton.setButtonText({});
    od3EnableButton.setName("od3Footswitch");
    od3EnableButton.setTooltip("Toggle the OD-3-style overdrive");
    od3EnableButton.setClickingTogglesState(true);
    od3EnableButton.setLookAndFeel(&ampLookAndFeel);
    addAndMakeVisible(od3EnableButton);
    od3EnableAttachment = std::make_unique<NkbTwinAudioProcessor::APVTS::ButtonAttachment>(
        processor.parameters, "od3Enabled", od3EnableButton);

    cabinetEnableButton.setButtonText("USE LOADED IR");
    cabinetEnableButton.setClickingTogglesState(true);
    cabinetEnableButton.setLookAndFeel(&ampLookAndFeel);
    addAndMakeVisible(cabinetEnableButton);
    cabinetEnableAttachment = std::make_unique<NkbTwinAudioProcessor::APVTS::ButtonAttachment>(
        processor.parameters, "irEnabled", cabinetEnableButton);

    testToneButton.setButtonText("TEST TONE");
    testToneButton.setClickingTogglesState(true);
    testToneButton.setLookAndFeel(&ampLookAndFeel);
    testToneButton.onClick = [this]
    {
        processor.setTestToneEnabled(testToneButton.getToggleState());
    };
    addAndMakeVisible(testToneButton);

    loadIRButton.setButtonText("LOAD IR");
    loadIRButton.onClick = [this] { beginImpulseResponseLoad(); };
    addAndMakeVisible(loadIRButton);

    clearIRButton.setButtonText("CLEAR");
    clearIRButton.onClick = [this] { clearImpulseResponse(); };
    addAndMakeVisible(clearIRButton);

    resetButton.setButtonText("RESET");
    resetButton.onClick = [this] { resetDefaults(); };
    addAndMakeVisible(resetButton);

    irStatusLabel.setJustificationType(juce::Justification::centredLeft);
    irStatusLabel.setColour(juce::Label::textColourId, lettering());
    irStatusLabel.setFont(juce::FontOptions(12.0f));
    irStatusLabel.setMinimumHorizontalScale(0.75f);
    addAndMakeVisible(irStatusLabel);

    setPage(currentPage);
    updateIRStatus();
    startTimerHz(30);
}

NkbTwinAudioProcessorEditor::~NkbTwinAudioProcessorEditor()
{
    for (auto& knob : ampKnobs)
        knob.setLookAndFeel(nullptr);
    for (auto& knob : pedalKnobs)
        knob.setLookAndFeel(nullptr);
    for (auto& knob : od3Knobs)
        knob.setLookAndFeel(nullptr);

    processor.setTestToneEnabled(false);
    brightButton.setLookAndFeel(nullptr);
    pedalEnableButton.setLookAndFeel(nullptr);
    od3EnableButton.setLookAndFeel(nullptr);
    cabinetEnableButton.setLookAndFeel(nullptr);
    testToneButton.setLookAndFeel(nullptr);
    setLookAndFeel(nullptr);
}

void NkbTwinAudioProcessorEditor::configureKnob(juce::Slider& knob, juce::Label& label,
                                                 const juce::String& text)
{
    knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    knob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    knob.setRotaryParameters(-2.35f, 2.35f, true);
    knob.setLookAndFeel(&ampLookAndFeel);
    knob.setPopupDisplayEnabled(true, false, this);
    addAndMakeVisible(knob);

    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, lettering());
    label.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    addAndMakeVisible(label);
}

void NkbTwinAudioProcessorEditor::AmpLookAndFeel::drawRotarySlider(
    juce::Graphics& g, int x, int y, int width, int height, float position,
    float startAngle, float endAngle, juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
                                                static_cast<float>(width), static_cast<float>(height)).reduced(7.0f);
    const auto diameter = juce::jmin(bounds.getWidth(), bounds.getHeight());
    const auto knobBounds = bounds.withSizeKeepingCentre(diameter, diameter);
    const auto centre = knobBounds.getCentre();
    const auto radius = diameter * 0.5f;
    const auto isPedalKnob = slider.getName() == "pedalKnob";

    for (int tick = 0; tick <= 10; ++tick)
    {
        const auto angle = startAngle + static_cast<float>(tick) * (endAngle - startAngle) / 10.0f;
        const auto major = (tick % 5) == 0;
        const auto inner = radius + 3.0f;
        const auto outer = radius + (major ? 10.0f : 7.0f);
        const auto a = juce::Point<float>(centre.x + std::sin(angle) * inner,
                                          centre.y - std::cos(angle) * inner);
        const auto b = juce::Point<float>(centre.x + std::sin(angle) * outer,
                                          centre.y - std::cos(angle) * outer);
        g.setColour(isPedalKnob
                        ? (major ? juce::Colour::fromRGB(250, 194, 59)
                                 : juce::Colour::fromRGB(235, 172, 42).withAlpha(0.7f))
                        : (major ? lettering() : muted()));
        g.drawLine(a.x, a.y, b.x, b.y, major ? 1.4f : 0.8f);
    }

    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.fillEllipse(knobBounds.translated(0.0f, 3.0f).expanded(2.0f));
    g.setColour(isPedalKnob ? juce::Colour::fromRGB(12, 18, 25)
                            : juce::Colour::fromRGB(126, 137, 151));
    g.fillEllipse(knobBounds.expanded(1.5f));
    g.setColour(isPedalKnob ? juce::Colour::fromRGB(30, 38, 47)
                            : juce::Colour::fromRGB(15, 18, 23));
    g.fillEllipse(knobBounds);
    if (isPedalKnob)
    {
        // A dark, fluted edge with a brushed silver face, like a compact stompbox knob.
        for (int ridge = 0; ridge < 24; ++ridge)
        {
            const auto angle = juce::MathConstants<float>::twoPi * static_cast<float>(ridge) / 24.0f;
            const auto outer = radius - 1.0f;
            const auto inner = radius - 5.0f;
            g.setColour((ridge % 2) == 0 ? juce::Colour::fromRGB(93, 103, 113)
                                         : juce::Colour::fromRGB(17, 24, 32));
            g.drawLine(centre.x + std::sin(angle) * inner,
                       centre.y - std::cos(angle) * inner,
                       centre.x + std::sin(angle) * outer,
                       centre.y - std::cos(angle) * outer, 1.0f);
        }
        const auto face = knobBounds.reduced(7.0f);
        g.setGradientFill(juce::ColourGradient(juce::Colour::fromRGB(250, 250, 241),
                                               face.getX(), face.getY(),
                                               juce::Colour::fromRGB(145, 153, 158),
                                               face.getRight(), face.getBottom(), false));
        g.fillEllipse(face);
        g.setColour(juce::Colour::fromRGB(255, 255, 255).withAlpha(0.7f));
        g.drawEllipse(face.reduced(1.0f), 0.8f);
    }
    else
    {
        g.setGradientFill(juce::ColourGradient(juce::Colour::fromRGB(80, 91, 107),
                                               knobBounds.getX(), knobBounds.getY(),
                                               juce::Colour::fromRGB(35, 41, 51),
                                               knobBounds.getRight(), knobBounds.getBottom(), false));
        g.fillEllipse(knobBounds.reduced(3.0f));
        g.setColour(juce::Colour::fromRGB(182, 191, 202).withAlpha(0.55f));
        g.drawEllipse(knobBounds.reduced(3.0f), 0.8f);
    }

    const auto angle = startAngle + position * (endAngle - startAngle);
    const auto pointerEnd = juce::Point<float>(centre.x + std::sin(angle) * radius * 0.67f,
                                                centre.y - std::cos(angle) * radius * 0.67f);
    g.setColour(juce::Colour::fromRGB(8, 10, 13).withAlpha(0.55f));
    g.drawLine(centre.x + 1.0f, centre.y + 1.5f, pointerEnd.x + 1.0f, pointerEnd.y + 1.5f, 4.0f);
    g.setColour(isPedalKnob ? juce::Colour::fromRGB(255, 193, 54)
                            : juce::Colour::fromRGB(249, 239, 210));
    g.drawLine(centre.x, centre.y, pointerEnd.x, pointerEnd.y, 2.7f);
    g.setColour(isPedalKnob ? juce::Colour::fromRGB(93, 99, 102)
                            : juce::Colour::fromRGB(196, 204, 210));
    g.fillEllipse(centre.x - 3.3f, centre.y - 3.3f, 6.6f, 6.6f);
    g.setColour(juce::Colour::fromRGB(70, 79, 89));
    g.fillEllipse(centre.x - 1.4f, centre.y - 1.4f, 2.8f, 2.8f);
}

void NkbTwinAudioProcessorEditor::AmpLookAndFeel::drawToggleButton(
    juce::Graphics& g, juce::ToggleButton& button, bool highlighted, bool down)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
    if (highlighted || down)
        bounds = bounds.translated(0.0f, 1.0f);

    if (button.getName() == "pedalFootswitch" || button.getName() == "od3Footswitch")
    {
        const auto switchBounds = bounds.withSizeKeepingCentre(
            juce::jmin(bounds.getWidth() - 5.0f, bounds.getHeight() - 10.0f),
            juce::jmin(bounds.getWidth() - 5.0f, bounds.getHeight() - 10.0f));
        const auto switchFace = switchBounds.reduced(4.0f);
        g.setColour(juce::Colours::black.withAlpha(0.5f));
        g.fillEllipse(switchBounds.translated(0.0f, 3.0f).expanded(2.0f));
        g.setGradientFill(juce::ColourGradient(juce::Colour::fromRGB(96, 108, 117),
                                               switchBounds.getX(), switchBounds.getY(),
                                               juce::Colour::fromRGB(26, 34, 42),
                                               switchBounds.getRight(), switchBounds.getBottom(), false));
        g.fillEllipse(switchBounds);
        g.setGradientFill(juce::ColourGradient(juce::Colour::fromRGB(249, 250, 246),
                                               switchFace.getX(), switchFace.getY(),
                                               juce::Colour::fromRGB(139, 151, 159),
                                               switchFace.getRight(), switchFace.getBottom(), false));
        g.fillEllipse(switchFace);
        g.setColour(button.getToggleState() ? juce::Colour::fromRGB(255, 193, 54)
                                           : juce::Colour::fromRGB(91, 102, 111));
        g.drawEllipse(switchFace, 1.2f);
        g.setColour(juce::Colour::fromRGB(255, 255, 255).withAlpha(0.65f));
        g.drawLine(switchFace.getX() + 8.0f, switchFace.getY() + 7.0f,
                   switchFace.getRight() - 8.0f, switchFace.getY() + 7.0f, 1.0f);
        return;
    }

    if (button.getName() == "brightSwitch")
    {
        const auto bezel = bounds.reduced(2.0f, 3.0f);
        g.setColour(juce::Colours::black.withAlpha(0.7f));
        g.fillRoundedRectangle(bezel.translated(0.0f, 1.0f), 2.0f);
        g.setGradientFill(juce::ColourGradient(juce::Colour::fromRGB(166, 173, 177),
                                               bezel.getX(), bezel.getY(),
                                               juce::Colour::fromRGB(55, 61, 67),
                                               bezel.getRight(), bezel.getBottom(), false));
        g.fillRoundedRectangle(bezel, 2.0f);
        const auto stemY = button.getToggleState() ? bezel.getY() + 3.0f : bezel.getBottom() - 12.0f;
        g.setColour(juce::Colour::fromRGB(21, 25, 29));
        g.fillRoundedRectangle(bezel.withSizeKeepingCentre(4.0f, 11.0f).withY(stemY), 1.0f);
        g.setColour(button.getToggleState() ? juce::Colour::fromRGB(255, 214, 124)
                                           : juce::Colour::fromRGB(199, 205, 207));
        g.fillEllipse(bezel.getCentreX() - 3.0f, stemY - 2.0f, 6.0f, 6.0f);
        return;
    }

    g.setColour(juce::Colour::fromRGB(8, 10, 13).withAlpha(0.7f));
    g.fillRoundedRectangle(bounds.translated(0.0f, 2.0f), 7.0f);
    g.setColour(juce::Colour::fromRGB(34, 40, 49));
    g.fillRoundedRectangle(bounds, 7.0f);
    g.setColour(button.getToggleState() ? accent() : juce::Colour::fromRGB(79, 88, 101));
    g.drawRoundedRectangle(bounds, 7.0f, 1.2f);

    const auto indicator = juce::Rectangle<float>(bounds.getRight() - 19.0f,
                                                   bounds.getCentreY() - 4.0f, 8.0f, 8.0f);
    g.setColour(button.getToggleState() ? accent() : juce::Colour::fromRGB(67, 74, 84));
    g.fillEllipse(indicator);
    g.setColour(button.getToggleState() ? juce::Colour::fromRGB(190, 255, 246)
                                       : juce::Colour::fromRGB(115, 124, 137));
    g.drawEllipse(indicator, 0.8f);

    g.setColour(lettering());
    g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
    g.drawFittedText(button.getButtonText(), button.getLocalBounds().reduced(9, 2),
                     juce::Justification::centred, 1);
}

void NkbTwinAudioProcessorEditor::AmpLookAndFeel::drawButtonBackground(
    juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour,
    bool highlighted, bool down)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
    if (down)
        bounds = bounds.translated(0.0f, 1.0f);
    auto fill = backgroundColour;
    if (highlighted)
        fill = fill.brighter(0.12f);
    g.setColour(juce::Colours::black.withAlpha(0.35f));
    g.fillRoundedRectangle(bounds.translated(0.0f, 2.0f), 6.0f);
    g.setColour(fill);
    g.fillRoundedRectangle(bounds, 6.0f);
    g.setColour(juce::Colour::fromRGB(82, 94, 107));
    g.drawRoundedRectangle(bounds, 6.0f, 1.0f);
}

void NkbTwinAudioProcessorEditor::setPage(Page page)
{
    currentPage = page;
    const auto showAmp = page == Page::amp;
    const auto showEffects = page == Page::effects;
    const auto showCabinet = page == Page::cabinet;

    for (auto& knob : ampKnobs) knob.setVisible(showAmp);
    for (auto& label : ampLabels) label.setVisible(showAmp);
    for (auto& knob : pedalKnobs) knob.setVisible(showEffects);
    for (auto& label : pedalLabels) label.setVisible(showEffects);
    for (auto& knob : od3Knobs) knob.setVisible(showEffects);
    for (auto& label : od3Labels) label.setVisible(showEffects);
    brightButton.setVisible(showAmp);
    pedalEnableButton.setVisible(showEffects);
    od3EnableButton.setVisible(showEffects);
    cabinetEnableButton.setVisible(showCabinet);
    loadIRButton.setVisible(showCabinet);
    clearIRButton.setVisible(showCabinet);
    irStatusLabel.setVisible(showCabinet);

    for (size_t index = 0; index < pageButtons.size(); ++index)
    {
        const auto active = (page == Page::effects && index == 0)
                         || (page == Page::amp && index == 1)
                         || (page == Page::cabinet && index == 2);
        pageButtons[index].setColour(juce::TextButton::buttonColourId,
                                     active ? juce::Colour::fromRGB(57, 116, 128) : surface());
        pageButtons[index].setColour(juce::TextButton::textColourOffId, lettering());
    }

    resized();
    repaint();
}

void NkbTwinAudioProcessorEditor::beginImpulseResponseLoad()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Load Cabinet Impulse Response", juce::File{}, "*.wav;*.aif;*.aiff");
    fileChooser->launchAsync(juce::FileBrowserComponent::openMode
                               | juce::FileBrowserComponent::canSelectFiles,
                             [this](const juce::FileChooser& chooser)
                             {
                                 const auto file = chooser.getResult();
                                 if (file == juce::File{})
                                     return;

                                 if (!processor.loadCabinetImpulseResponse(file))
                                 {
                                     irStatusLabel.setText("Could not open that audio file as an IR.",
                                                           juce::dontSendNotification);
                                     return;
                                 }
                                 updateIRStatus();
                             });
}

void NkbTwinAudioProcessorEditor::clearImpulseResponse()
{
    processor.clearCabinetImpulseResponse();
    updateIRStatus();
}

void NkbTwinAudioProcessorEditor::resetDefaults()
{
    for (const auto* id : { "volume", "treble", "middle", "bass", "bright",
                            "pedalDrive", "pedalTone", "pedalLevel", "pedalEnabled",
                            "od3Drive", "od3Tone", "od3Level", "od3Enabled", "irEnabled" })
        processor.setParameterToDefault(id);
    processor.clearCabinetImpulseResponse();
    testToneButton.setToggleState(false, juce::dontSendNotification);
    processor.setTestToneEnabled(false);
    updateIRStatus();
    repaint();
}

void NkbTwinAudioProcessorEditor::updateIRStatus()
{
    const auto fileName = processor.getCabinetImpulseResponseName();
    const auto loaded = processor.hasCabinetImpulseResponse();
    cabinetEnableButton.setEnabled(loaded);
    clearIRButton.setEnabled(loaded);
    if (fileName.isNotEmpty())
        irStatusLabel.setText("Loaded: " + fileName, juce::dontSendNotification);
    else
        irStatusLabel.setText("No custom IR loaded  |  built-in Twin 2x12 voicing active",
                              juce::dontSendNotification);
}

void NkbTwinAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(background());
    drawCommonFrame(g);
    if (currentPage == Page::effects)
        drawEffectsPage(g);
    else if (currentPage == Page::amp)
        drawAmpPage(g);
    else
        drawCabinetPage(g);

    drawMeter(g, inputMeterBounds.toFloat(), processor.getInputMeter(), "IN");
    drawMeter(g, outputLeftMeterBounds.toFloat(), processor.getOutputLeftMeter(), "OUT L");
    drawMeter(g, outputRightMeterBounds.toFloat(), processor.getOutputRightMeter(), "OUT R");
}

void NkbTwinAudioProcessorEditor::drawCommonFrame(juce::Graphics& g)
{
    g.setColour(surface());
    g.fillRect(0, 0, getWidth(), 68);
    g.setColour(juce::Colour::fromRGB(47, 56, 68));
    g.drawHorizontalLine(67, 0.0f, static_cast<float>(getWidth()));

    g.setColour(lettering());
    g.setFont(juce::FontOptions(17.0f, juce::Font::bold));
    g.drawText("NKB TWIN", 26, 17, 154, 25, juce::Justification::centredLeft);
    g.setColour(muted());
    g.setFont(juce::FontOptions(8.5f, juce::Font::bold));
    g.drawText("GUITAR AMP SUITE", 27, 40, 150, 13, juce::Justification::centredLeft);

    g.setColour(juce::Colour::fromRGB(54, 61, 72));
    g.fillRect(0, 510, getWidth(), 50);
    g.setColour(juce::Colour::fromRGB(57, 66, 79));
    g.drawHorizontalLine(510, 0.0f, static_cast<float>(getWidth()));
    g.setColour(muted());
    g.setFont(juce::FontOptions(8.0f, juce::Font::bold));
}

void NkbTwinAudioProcessorEditor::drawAmpPage(juce::Graphics& g)
{
    const auto cabinet = juce::Rectangle<float>(22.0f, 84.0f, 856.0f, 416.0f);
    const auto panel = juce::Rectangle<float>(42.0f, 104.0f, 816.0f, 108.0f);
    const auto grille = juce::Rectangle<float>(42.0f, 222.0f, 816.0f, 260.0f);

    // Black textured cabinet shell and nickel corner caps.
    g.setColour(juce::Colours::black.withAlpha(0.75f));
    g.fillRoundedRectangle(cabinet.translated(0.0f, 5.0f), 18.0f);
    g.setGradientFill(juce::ColourGradient(juce::Colour::fromRGB(40, 42, 44),
                                           cabinet.getX(), cabinet.getY(),
                                           juce::Colour::fromRGB(10, 12, 14),
                                           cabinet.getRight(), cabinet.getBottom(), false));
    g.fillRoundedRectangle(cabinet, 18.0f);
    g.saveState();
    juce::Path cabinetMask;
    cabinetMask.addRoundedRectangle(cabinet.reduced(2.0f), 16.0f);
    g.reduceClipRegion(cabinetMask);
    g.setColour(juce::Colour::fromRGB(158, 163, 164).withAlpha(0.06f));
    for (int x = 27; x < 874; x += 4)
        g.drawVerticalLine(x, 85.0f, 500.0f);
    g.restoreState();
    g.setColour(juce::Colour::fromRGB(118, 123, 126));
    g.drawRoundedRectangle(cabinet.reduced(2.0f), 16.0f, 1.0f);

    for (const auto corner : { juce::Rectangle<float>(27, 88, 22, 26),
                               juce::Rectangle<float>(851, 88, 22, 26),
                               juce::Rectangle<float>(27, 470, 22, 26),
                               juce::Rectangle<float>(851, 470, 22, 26) })
    {
        g.setColour(juce::Colour::fromRGB(168, 172, 171));
        g.fillRoundedRectangle(corner, 4.0f);
        g.setColour(juce::Colour::fromRGB(67, 72, 75));
        g.drawLine(corner.getX() + 5.0f, corner.getY() + 5.0f,
                   corner.getRight() - 5.0f, corner.getBottom() - 5.0f, 1.0f);
    }

    // Top strap handle, above the classic blackface control strip.
    g.setColour(juce::Colour::fromRGB(164, 169, 169));
    g.fillRoundedRectangle(382.0f, 78.0f, 136.0f, 16.0f, 6.0f);
    g.setColour(juce::Colour::fromRGB(14, 17, 19));
    g.fillRoundedRectangle(397.0f, 76.0f, 106.0f, 13.0f, 5.0f);

    // Blackface panel with nickel trim.
    g.setGradientFill(juce::ColourGradient(juce::Colour::fromRGB(38, 40, 42),
                                           panel.getX(), panel.getY(),
                                           juce::Colour::fromRGB(13, 15, 17),
                                           panel.getX(), panel.getBottom(), false));
    g.fillRoundedRectangle(panel, 4.0f);
    g.setColour(juce::Colour::fromRGB(166, 170, 169));
    g.drawRoundedRectangle(panel, 4.0f, 1.1f);
    g.setColour(juce::Colour::fromRGB(191, 196, 194).withAlpha(0.36f));
    g.drawHorizontalLine(108, panel.getX() + 3.0f, panel.getRight() - 3.0f);

    // Two Normal-channel inputs and the Bright switch.
    g.setColour(warmWhite());
    g.setFont(juce::FontOptions(8.0f, juce::Font::bold));
    g.drawText("NORMAL", 53, 111, 84, 13, juce::Justification::centred);
    g.drawText("1", 66, 126, 18, 12, juce::Justification::centred);
    g.drawText("2", 102, 126, 18, 12, juce::Justification::centred);
    for (const auto x : { 65.0f, 101.0f })
    {
        g.setColour(juce::Colour::fromRGB(4, 6, 8));
        g.fillEllipse(x, 138.0f, 20.0f, 20.0f);
        g.setColour(juce::Colour::fromRGB(171, 177, 177));
        g.drawEllipse(x, 138.0f, 20.0f, 20.0f, 1.8f);
        g.setColour(juce::Colour::fromRGB(9, 11, 13));
        g.fillEllipse(x + 5.0f, 143.0f, 10.0f, 10.0f);
        g.setColour(juce::Colour::fromRGB(100, 107, 111));
        g.drawEllipse(x + 5.0f, 143.0f, 10.0f, 10.0f, 0.8f);
    }
    g.setColour(warmWhite());
    g.drawText("BRIGHT", 137, 116, 50, 13, juce::Justification::centred);
    g.setColour(juce::Colour::fromRGB(105, 110, 112));
    g.drawVerticalLine(196, 117.0f, 197.0f);
    g.setColour(warmWhite());
    g.setFont(juce::FontOptions(17.0f, juce::Font::bold | juce::Font::italic));
    g.drawFittedText("NKB TWIN", 651, 116, 145, 23, juce::Justification::centred, 1);
    g.setColour(juce::Colour::fromRGB(203, 207, 203));
    g.setFont(juce::FontOptions(7.0f, juce::Font::bold));
    g.drawText("REVERB AMP  /  CLEAN 2 x 12", 652, 143, 145, 11,
               juce::Justification::centred);

    // Twin-style silver grille cloth with woven horizontal and vertical threads.
    g.setGradientFill(juce::ColourGradient(juce::Colour::fromRGB(160, 164, 158),
                                           grille.getX(), grille.getY(),
                                           juce::Colour::fromRGB(85, 91, 94),
                                           grille.getRight(), grille.getBottom(), false));
    g.fillRoundedRectangle(grille, 4.0f);
    g.saveState();
    juce::Path grilleMask;
    grilleMask.addRoundedRectangle(grille.reduced(1.0f), 3.0f);
    g.reduceClipRegion(grilleMask);
    for (int y = 223; y < 482; y += 4)
    {
        g.setColour(juce::Colour::fromRGB(231, 231, 220).withAlpha(0.30f));
        g.drawHorizontalLine(y, 42.0f, 858.0f);
        g.setColour(juce::Colour::fromRGB(29, 32, 35).withAlpha(0.34f));
        g.drawHorizontalLine(y + 1, 42.0f, 858.0f);
    }
    for (int x = 43; x < 858; x += 7)
    {
        g.setColour(juce::Colour::fromRGB(236, 234, 219).withAlpha(0.18f));
        g.drawVerticalLine(x, 222.0f, 482.0f);
        g.setColour(juce::Colour::fromRGB(22, 25, 28).withAlpha(0.20f));
        g.drawVerticalLine(x + 2, 222.0f, 482.0f);
    }
    g.restoreState();
    g.setColour(juce::Colour::fromRGB(204, 208, 202));
    g.drawRoundedRectangle(grille, 4.0f, 1.0f);

    // Small model badge on the grille, plus the red pilot jewel at the right of the panel.
    g.setColour(juce::Colour::fromRGB(26, 29, 31).withAlpha(0.78f));
    g.fillRoundedRectangle(75.0f, 307.0f, 168.0f, 47.0f, 7.0f);
    g.setColour(juce::Colour::fromRGB(222, 224, 216));
    g.drawRoundedRectangle(75.0f, 307.0f, 168.0f, 47.0f, 7.0f, 1.0f);
    g.setColour(juce::Colour::fromRGB(240, 239, 226));
    g.setFont(juce::FontOptions(22.0f, juce::Font::bold | juce::Font::italic));
    g.drawText("NKB Twin", 84, 311, 151, 26, juce::Justification::centred);
    g.setColour(juce::Colour::fromRGB(193, 198, 197));
    g.setFont(juce::FontOptions(7.5f, juce::Font::bold));
    g.drawText("CLEAN AMPLIFIER  ·  2 x 12", 84, 338, 151, 11, juce::Justification::centred);

    g.setColour(juce::Colour::fromRGB(5, 6, 8));
    g.fillEllipse(813.0f, 130.0f, 34.0f, 34.0f);
    g.setColour(juce::Colour::fromRGB(93, 98, 99));
    g.drawEllipse(813.0f, 130.0f, 34.0f, 34.0f, 1.0f);
    g.setGradientFill(juce::ColourGradient(juce::Colour::fromRGB(255, 118, 108),
                                           820.0f, 136.0f,
                                           juce::Colour::fromRGB(122, 20, 31),
                                           842.0f, 157.0f, false));
    g.fillEllipse(819.0f, 136.0f, 22.0f, 22.0f);
    g.setColour(juce::Colour::fromRGB(255, 213, 198).withAlpha(0.85f));
    g.fillEllipse(824.0f, 139.0f, 7.0f, 5.0f);
    g.setColour(warmWhite());
    g.setFont(juce::FontOptions(7.0f, juce::Font::bold));
    g.drawText("PILOT", 812, 166, 37, 10, juce::Justification::centred);
}

void NkbTwinAudioProcessorEditor::drawEffectsPage(juce::Graphics& g)
{
    const auto drawPedal = [&g](float xOffset, bool isOD3,
                                const std::array<juce::Slider, 3>& knobs,
                                const juce::ToggleButton& enable)
    {
        g.saveState();
        g.addTransform(juce::AffineTransform::translation(xOffset, 0.0f));
        const auto pedal = juce::Rectangle<float>(324.0f, 78.0f, 252.0f, 424.0f);
        const auto edge = isOD3 ? juce::Colour::fromRGB(126, 91, 0)
                                : juce::Colour::fromRGB(108, 190, 229);
        const auto main = isOD3 ? juce::Colour::fromRGB(255, 194, 0)
                                : juce::Colour::fromRGB(48, 143, 205);
        const auto shadow = isOD3 ? juce::Colour::fromRGB(190, 127, 0)
                                  : juce::Colour::fromRGB(14, 64, 113);
        const auto letteringColour = isOD3 ? juce::Colour::fromRGB(49, 42, 20)
                                            : juce::Colour::fromRGB(255, 194, 56);

        g.setColour(juce::Colours::black.withAlpha(0.55f));
        g.fillRoundedRectangle(pedal.translated(0.0f, 7.0f), 22.0f);
        g.setGradientFill(juce::ColourGradient(main.brighter(isOD3 ? 0.12f : 0.0f),
                                               pedal.getX(), pedal.getY(),
                                               shadow, pedal.getRight(), pedal.getBottom(), false));
        g.fillRoundedRectangle(pedal, 22.0f);
        g.setColour(edge.withAlpha(0.85f));
        g.drawRoundedRectangle(pedal.reduced(3.0f), 19.0f, 1.4f);

        g.saveState();
        juce::Path bodyMask;
        bodyMask.addRoundedRectangle(pedal.reduced(4.0f), 17.0f);
        g.reduceClipRegion(bodyMask);
        g.setColour((isOD3 ? juce::Colour::fromRGB(255, 246, 183)
                           : juce::Colour::fromRGB(190, 225, 241)).withAlpha(0.14f));
        for (int y = 84; y < 496; y += 5)
            g.drawHorizontalLine(y, pedal.getX() + 9.0f, pedal.getRight() - 9.0f);
        g.restoreState();

        for (const auto screw : { juce::Point<float>{ 337.0f, 92.0f },
                                  juce::Point<float>{ 563.0f, 92.0f },
                                  juce::Point<float>{ 337.0f, 488.0f },
                                  juce::Point<float>{ 563.0f, 488.0f } })
        {
            g.setColour((isOD3 ? juce::Colour::fromRGB(116, 85, 0)
                               : juce::Colour::fromRGB(9, 44, 77)).withAlpha(0.58f));
            g.fillEllipse(screw.x - 4.0f, screw.y - 4.0f, 8.0f, 8.0f);
            g.setColour(isOD3 ? juce::Colour::fromRGB(218, 184, 86)
                              : juce::Colour::fromRGB(179, 207, 220));
            g.drawLine(screw.x - 2.0f, screw.y + 2.0f, screw.x + 2.0f, screw.y - 2.0f, 0.8f);
        }

        g.setColour(letteringColour);
        g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
        g.drawText("CHECK", 391, 94, 82, 18, juce::Justification::centred);
        const auto ledCentre = juce::Point<float>(450.0f, 119.0f);
        if (enable.getToggleState())
        {
            g.setColour(juce::Colour::fromRGB(255, 50, 35).withAlpha(0.25f));
            g.fillEllipse(ledCentre.x - 11.0f, ledCentre.y - 11.0f, 22.0f, 22.0f);
        }
        g.setColour(enable.getToggleState() ? juce::Colour::fromRGB(255, 67, 46)
                                           : juce::Colour::fromRGB(87, 31, 37));
        g.fillEllipse(ledCentre.x - 6.0f, ledCentre.y - 6.0f, 12.0f, 12.0f);
        g.setColour(juce::Colour::fromRGB(255, 196, 175).withAlpha(0.8f));
        g.drawEllipse(ledCentre.x - 6.0f, ledCentre.y - 6.0f, 12.0f, 12.0f, 1.0f);

        g.setColour(letteringColour);
        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        g.drawText("LEVEL", 337, 218, 96, 16, juce::Justification::centred);
        g.drawText(isOD3 ? "DRIVE" : "GAIN", 467, 218, 96, 16, juce::Justification::centred);
        g.drawText("TONE", 402, 251, 96, 16, juce::Justification::centred);
        g.setColour(isOD3 ? juce::Colour::fromRGB(62, 48, 6)
                          : juce::Colour::fromRGB(242, 246, 248));
        g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
        g.drawText(juce::String(knobs[0].getValue(), 1), 337, 234, 96, 14,
                   juce::Justification::centred);
        g.drawText(juce::String(knobs[1].getValue(), 1), 467, 234, 96, 14,
                   juce::Justification::centred);
        g.drawText(juce::String(knobs[2].getValue(), 1), 402, 343, 96, 14,
                   juce::Justification::centred);

        g.setColour(letteringColour);
        g.setFont(juce::FontOptions(19.0f, juce::Font::bold));
        g.drawText("NKB", 371, 371, 158, 24, juce::Justification::centred);
        g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
        g.drawText(isOD3 ? "OVERDRIVE" : "BLUES DRIVE", 363, 394, 174, 18,
                   juce::Justification::centred);
        g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
        g.drawText(isOD3 ? "OD-3 STYLE" : "BD-2 STYLE", 390, 411, 120, 14,
                   juce::Justification::centred);

        for (const auto x : { 324.0f, 568.0f })
        {
            g.setColour(juce::Colour::fromRGB(17, 27, 38));
            g.fillEllipse(x - 5.0f, 303.0f, 10.0f, 10.0f);
            g.setColour(juce::Colour::fromRGB(155, 170, 179));
            g.drawEllipse(x - 5.0f, 303.0f, 10.0f, 10.0f, 1.0f);
        }
        g.restoreState();
    };

    drawPedal(-148.0f, false, pedalKnobs, pedalEnableButton);
    drawPedal(148.0f, true, od3Knobs, od3EnableButton);
}

void NkbTwinAudioProcessorEditor::drawCabinetPage(juce::Graphics& g)
{
    auto card = juce::Rectangle<float>(34.0f, 84.0f, 832.0f, 410.0f);
    g.setColour(surface());
    g.fillRoundedRectangle(card, 13.0f);
    g.setColour(juce::Colour::fromRGB(62, 74, 88));
    g.drawRoundedRectangle(card, 13.0f, 1.0f);

    auto cab = juce::Rectangle<float>(76.0f, 112.0f, 315.0f, 352.0f);
    g.setGradientFill(juce::ColourGradient(juce::Colour::fromRGB(91, 59, 40),
                                           cab.getX(), cab.getY(),
                                           juce::Colour::fromRGB(42, 33, 29),
                                           cab.getRight(), cab.getBottom(), false));
    g.fillRoundedRectangle(cab, 14.0f);
    g.setColour(juce::Colour::fromRGB(169, 130, 83));
    g.drawRoundedRectangle(cab.reduced(3.0f), 11.0f, 1.2f);
    drawSpeaker(g, { 111.0f, 149.0f, 112.0f, 112.0f });
    drawSpeaker(g, { 244.0f, 149.0f, 112.0f, 112.0f });
    drawSpeaker(g, { 111.0f, 290.0f, 112.0f, 112.0f });
    drawSpeaker(g, { 244.0f, 290.0f, 112.0f, 112.0f });
    g.setColour(lettering());
    g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    g.drawText("TWIN 2x12", 120, 429, 225, 17, juce::Justification::centred);

    g.setColour(lettering());
    g.setFont(juce::FontOptions(22.0f, juce::Font::bold));
    g.drawText("CABINET", 445, 116, 350, 31, juce::Justification::centredLeft);
    g.setColour(muted());
    g.setFont(juce::FontOptions(10.0f));
    g.drawText("Twin 2x12 speaker voicing is active by default.", 447, 151, 350, 20,
               juce::Justification::centredLeft);
    g.setColour(juce::Colour::fromRGB(66, 78, 91));
    g.drawHorizontalLine(185, 447.0f, 821.0f);

    g.setColour(accent());
    g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    g.drawText("CUSTOM IMPULSE RESPONSE", 447, 204, 350, 17, juce::Justification::centredLeft);
    g.setColour(muted());
    g.setFont(juce::FontOptions(9.0f));
    g.drawFittedText("Load a mono or stereo cabinet IR in WAV or AIFF format.", 447, 224, 350, 35,
                     juce::Justification::topLeft, 2);
    g.drawFittedText("When enabled, it replaces the built-in speaker voicing.", 447, 259, 350, 30,
                     juce::Justification::topLeft, 2);
}

void NkbTwinAudioProcessorEditor::drawMeter(juce::Graphics& g, juce::Rectangle<float> bounds,
                                             float level, const juce::String& label)
{
    g.setColour(lettering());
    g.setFont(juce::FontOptions(8.0f, juce::Font::bold));
    g.drawText(label, bounds.removeFromLeft(43.0f).toNearestInt(), juce::Justification::centredLeft);

    g.setColour(juce::Colour::fromRGB(9, 12, 16));
    g.fillRoundedRectangle(bounds, 3.0f);
    const auto usable = bounds.reduced(2.0f, 2.0f);
    const auto normalized = juce::jlimit(0.0f, 1.0f, level * 3.5f);
    const auto barWidth = usable.getWidth() * normalized;
    if (barWidth > 0.5f)
    {
        const auto colour = normalized > 0.92f ? juce::Colour::fromRGB(213, 73, 63)
                           : normalized > 0.72f ? amber() : juce::Colour::fromRGB(91, 184, 131);
        g.setColour(colour);
        g.fillRoundedRectangle(usable.withWidth(barWidth), 2.0f);
    }
}

void NkbTwinAudioProcessorEditor::resized()
{
    pageButtons[0].setBounds(290, 16, 105, 34);
    pageButtons[1].setBounds(401, 16, 105, 34);
    pageButtons[2].setBounds(512, 16, 105, 34);
    testToneButton.setBounds(700, 18, 94, 30);
    resetButton.setBounds(808, 18, 64, 30);

    const std::array<int, 4> ampXs{ 205, 291, 377, 463 };
    for (size_t index = 0; index < ampKnobs.size(); ++index)
    {
        ampLabels[index].setBounds(ampXs[index] - 5, 186, 90, 14);
        ampKnobs[index].setBounds(ampXs[index], 112, 80, 76);
    }
    brightButton.setBounds(151, 133, 22, 28);

    pedalKnobs[0].setBounds(191, 126, 92, 86);
    pedalKnobs[1].setBounds(321, 126, 92, 86);
    pedalKnobs[2].setBounds(261, 261, 82, 78);
    od3Knobs[0].setBounds(487, 126, 92, 86);
    od3Knobs[1].setBounds(617, 126, 92, 86);
    od3Knobs[2].setBounds(557, 261, 82, 78);
    for (auto& label : pedalLabels)
        label.setVisible(false); // The compact enclosure carries its own silkscreen labels.
    for (auto& label : od3Labels)
        label.setVisible(false);
    pedalEnableButton.setBounds(266, 439, 72, 54);
    od3EnableButton.setBounds(562, 439, 72, 54);

    loadIRButton.setBounds(447, 314, 137, 36);
    clearIRButton.setBounds(594, 314, 86, 36);
    cabinetEnableButton.setBounds(447, 365, 233, 34);
    irStatusLabel.setBounds(447, 414, 365, 30);

    inputMeterBounds = { 30, 531, 260, 15 };
    outputLeftMeterBounds = { 306, 531, 260, 15 };
    outputRightMeterBounds = { 610, 531, 260, 15 };
}

void NkbTwinAudioProcessorEditor::timerCallback()
{
    repaint(inputMeterBounds);
    repaint(outputLeftMeterBounds);
    repaint(outputRightMeterBounds);
    if (currentPage == Page::effects)
        repaint({ 165, 78, 570, 424 });
}
