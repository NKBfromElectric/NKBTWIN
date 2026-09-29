#pragma once

#include "PluginProcessor.h"

class NkbTwinAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                          private juce::Timer
{
public:
    explicit NkbTwinAudioProcessorEditor(NkbTwinAudioProcessor&);
    ~NkbTwinAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    enum class Page { effects, amp, cabinet };

    class AmpLookAndFeel final : public juce::LookAndFeel_V4
    {
    public:
        void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height,
                              float sliderPosProportional, float rotaryStartAngle,
                              float rotaryEndAngle, juce::Slider&) override;
        void drawToggleButton(juce::Graphics&, juce::ToggleButton&,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override;
        void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&,
                                  bool shouldDrawButtonAsHighlighted,
                                  bool shouldDrawButtonAsDown) override;
    };

    class StartupSplash final : public juce::Component
    {
    public:
        void setLogo(std::unique_ptr<juce::Drawable> image) { logo = std::move(image); }
        void paint(juce::Graphics&) override;

    private:
        std::unique_ptr<juce::Drawable> logo;
    };

    void timerCallback() override;
    void setPage(Page);
    void beginImpulseResponseLoad();
    void clearImpulseResponse();
    void resetDefaults();
    void updateIRStatus();
    void drawCommonFrame(juce::Graphics&);
    void drawAmpPage(juce::Graphics&);
    void drawEffectsPage(juce::Graphics&);
    void drawCabinetPage(juce::Graphics&);
    void drawMeter(juce::Graphics&, juce::Rectangle<float>, float, const juce::String&);
    void configureKnob(juce::Slider&, juce::Label&, const juce::String&);

    NkbTwinAudioProcessor& processor;
    AmpLookAndFeel ampLookAndFeel;
    StartupSplash startupSplash;
    Page currentPage = Page::amp;
    int startupSplashFramesRemaining = 0;

    std::array<juce::Slider, 4> ampKnobs;
    std::array<juce::Label, 4> ampLabels;
    std::array<std::unique_ptr<NkbTwinAudioProcessor::APVTS::SliderAttachment>, 4> ampAttachments;
    std::array<juce::Slider, 3> pedalKnobs;
    std::array<juce::Label, 3> pedalLabels;
    std::array<std::unique_ptr<NkbTwinAudioProcessor::APVTS::SliderAttachment>, 3> pedalAttachments;
    std::array<juce::Slider, 3> od3Knobs;
    std::array<juce::Label, 3> od3Labels;
    std::array<std::unique_ptr<NkbTwinAudioProcessor::APVTS::SliderAttachment>, 3> od3Attachments;

    std::array<juce::TextButton, 3> pageButtons;
    juce::ToggleButton brightButton;
    juce::ToggleButton pedalEnableButton;
    juce::ToggleButton od3EnableButton;
    juce::ToggleButton cabinetEnableButton;
    juce::ToggleButton testToneButton;
    juce::TextButton loadIRButton;
    juce::TextButton clearIRButton;
    juce::TextButton resetButton;
    juce::Label irStatusLabel;

    std::unique_ptr<NkbTwinAudioProcessor::APVTS::ButtonAttachment> brightAttachment;
    std::unique_ptr<NkbTwinAudioProcessor::APVTS::ButtonAttachment> pedalEnableAttachment;
    std::unique_ptr<NkbTwinAudioProcessor::APVTS::ButtonAttachment> od3EnableAttachment;
    std::unique_ptr<NkbTwinAudioProcessor::APVTS::ButtonAttachment> cabinetEnableAttachment;
    std::unique_ptr<juce::FileChooser> fileChooser;

    juce::Rectangle<int> inputMeterBounds;
    juce::Rectangle<int> outputLeftMeterBounds;
    juce::Rectangle<int> outputRightMeterBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NkbTwinAudioProcessorEditor)
};
