#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>

class NkbTwinAudioProcessor final : public juce::AudioProcessor
{
public:
    using APVTS = juce::AudioProcessorValueTreeState;

    NkbTwinAudioProcessor();
    ~NkbTwinAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    bool loadCabinetImpulseResponse(const juce::File& file);
    void clearCabinetImpulseResponse();
    juce::String getCabinetImpulseResponseName() const;
    juce::File getCabinetImpulseResponseFile() const { return juce::File(cabinetIRPath); }
    bool hasCabinetImpulseResponse() const noexcept
    {
        return cabinetIRLoaded.load(std::memory_order_relaxed);
    }
    void setParameterToDefault(const juce::String& parameterId);

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destinationData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    APVTS parameters;
    float getInputMeter() const noexcept { return inputMeter.load(std::memory_order_relaxed); }
    float getOutputLeftMeter() const noexcept { return outputLeftMeter.load(std::memory_order_relaxed); }
    float getOutputRightMeter() const noexcept { return outputRightMeter.load(std::memory_order_relaxed); }

private:
    struct ChannelState
    {
        float lowSplit = 0.0f;
        float highSplit = 0.0f;
        float brightLow = 0.0f;
        float rumbleLow = 0.0f;
        float speakerBassLow = 0.0f;
        float speakerLowMid = 0.0f;
        float speakerMid = 0.0f;
        float speakerPresence = 0.0f;
        float speakerAir = 0.0f;
        float pedalInputLow = 0.0f;
        float pedalStackLow = 0.0f;
        float pedalStackHigh = 0.0f;
        float pedalToneLow = 0.0f;
        float pedalBufferLow = 0.0f;
        float od3InputLow = 0.0f;
        float od3ToneLow = 0.0f;
        float od3BufferLow = 0.0f;
    };

    static APVTS::ParameterLayout createParameterLayout();
    static float dbToGain(float db) noexcept;

    std::array<ChannelState, 2> channelStates{};
    std::array<juce::dsp::IIR::Filter<float>, 5> builtInCabinetEq;
    juce::dsp::Convolution cabinetConvolution;
    juce::AudioBuffer<float> builtInCabinetBuffer;
    juce::AudioBuffer<float> cabinetBlendBuffer;
    float lowSplitCoefficient = 0.0f;
    float highSplitCoefficient = 0.0f;
    float brightCoefficient = 0.0f;
    float speakerBassCoefficient = 0.0f;
    float speakerLowMidCoefficient = 0.0f;
    float speakerMidCoefficient = 0.0f;
    float speakerPresenceCoefficient = 0.0f;
    float speakerAirCoefficient = 0.0f;
    float rumbleCoefficient = 0.0f;
    float pedalInputCoefficient = 0.0f;
    float pedalStackLowCoefficient = 0.0f;
    float pedalStackHighCoefficient = 0.0f;
    float pedalBufferCoefficient = 0.0f;
    float od3InputCoefficient = 0.0f;
    float od3BufferCoefficient = 0.0f;
    float driveToneCoefficient = 0.0f;
    float od3ToneCoefficient = 0.0f;
    juce::String cabinetIRPath;
    juce::Reverb postCabinetReverb;
    juce::AudioBuffer<float> reverbBuffer;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> ampDrive;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> ampVolume;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> bassGain;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> middleGain;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> trebleGain;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> brightAmount;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> pedalDrive;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> pedalTone;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> pedalLevel;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> pedalMix;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> od3Drive;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> od3Tone;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> od3Level;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> od3Mix;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> cabinetIRMix;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> reverbMix;
    std::atomic<float> inputMeter{ 0.0f };
    std::atomic<float> outputLeftMeter{ 0.0f };
    std::atomic<float> outputRightMeter{ 0.0f };
    std::atomic<bool> cabinetIRLoaded{ false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NkbTwinAudioProcessor)
};
