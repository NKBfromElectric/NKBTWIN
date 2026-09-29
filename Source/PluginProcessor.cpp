#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>
#include <memory>

namespace
{
constexpr auto volumeId = "volume";
constexpr auto trebleId = "treble";
constexpr auto middleId = "middle";
constexpr auto bassId = "bass";
constexpr auto brightId = "bright";
constexpr auto pedalDriveId = "pedalDrive";
constexpr auto pedalToneId = "pedalTone";
constexpr auto pedalLevelId = "pedalLevel";
constexpr auto pedalEnabledId = "pedalEnabled";
constexpr auto od3DriveId = "od3Drive";
constexpr auto od3ToneId = "od3Tone";
constexpr auto od3LevelId = "od3Level";
constexpr auto od3EnabledId = "od3Enabled";
constexpr auto irEnabledId = "irEnabled";
constexpr auto cabinetIRPathProperty = "cabinetIRPath";
constexpr auto stateTag = "NKB_TWIN_STATE_V4";

float onePoleCoefficient(float frequency, double sampleRate)
{
    return 1.0f - std::exp(-juce::MathConstants<float>::twoPi * frequency
                           / static_cast<float>(sampleRate));
}

float diodePairTransfer(float input, float knee) noexcept
{
    const auto positiveKnee = knee * 0.94f;
    const auto negativeKnee = knee * 1.06f;
    return input >= 0.0f ? positiveKnee * std::tanh(input / positiveKnee)
                         : negativeKnee * std::tanh(input / negativeKnee);
}

float tubeStage(float input, float gain, float bias) noexcept
{
    const auto biasPoint = std::tanh(bias * gain);
    const auto smallSignalGain = gain * (1.0f - biasPoint * biasPoint);
    return (std::tanh((input + bias) * gain) - biasPoint) / smallSignalGain;
}
}

NkbTwinAudioProcessor::NkbTwinAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, stateTag, createParameterLayout())
{
}

NkbTwinAudioProcessor::APVTS::ParameterLayout NkbTwinAudioProcessor::createParameterLayout()
{
    APVTS::ParameterLayout layout;
    const auto knobRange = juce::NormalisableRange<float>{ 0.0f, 10.0f, 0.01f };

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ volumeId, 1 }, "Volume", knobRange, 5.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ trebleId, 1 }, "Treble", knobRange, 5.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ middleId, 1 }, "Middle", knobRange, 5.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ bassId, 1 }, "Bass", knobRange, 5.0f));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ brightId, 1 }, "Bright", false));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ pedalDriveId, 1 }, "Blues Gain", knobRange, 5.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ pedalToneId, 1 }, "Blues Tone", knobRange, 5.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ pedalLevelId, 1 }, "Blues Level", knobRange, 5.0f));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ pedalEnabledId, 1 }, "Blues Driver On", false));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ od3DriveId, 1 }, "OverDrive Drive", knobRange, 5.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ od3ToneId, 1 }, "OverDrive Tone", knobRange, 5.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ od3LevelId, 1 }, "OverDrive Level", knobRange, 5.0f));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ od3EnabledId, 1 }, "OverDrive On", false));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ irEnabledId, 1 }, "Custom IR On", false));

    return layout;
}

float NkbTwinAudioProcessor::dbToGain(float db) noexcept
{
    return juce::Decibels::decibelsToGain(db);
}

void NkbTwinAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    const auto rate = juce::jmax(1.0, sampleRate);
    const auto maxBlockSize = static_cast<juce::uint32>(juce::jmax(1, samplesPerBlock));
    testToneSampleRate = static_cast<float>(rate);
    lowSplitCoefficient = onePoleCoefficient(220.0f, rate);
    highSplitCoefficient = onePoleCoefficient(1800.0f, rate);
    brightCoefficient = onePoleCoefficient(2700.0f, rate);
    speakerBassCoefficient = onePoleCoefficient(130.0f, rate);
    speakerLowMidCoefficient = onePoleCoefficient(650.0f, rate);
    speakerMidCoefficient = onePoleCoefficient(1700.0f, rate);
    speakerPresenceCoefficient = onePoleCoefficient(3500.0f, rate);
    speakerAirCoefficient = onePoleCoefficient(5600.0f, rate);
    rumbleCoefficient = onePoleCoefficient(45.0f, rate);
    pedalInputCoefficient = onePoleCoefficient(36.0f, rate);
    pedalStackLowCoefficient = onePoleCoefficient(250.0f, rate);
    pedalStackHighCoefficient = onePoleCoefficient(2100.0f, rate);
    pedalBufferCoefficient = onePoleCoefficient(15000.0f, rate);
    od3InputCoefficient = onePoleCoefficient(85.0f, rate);
    od3BufferCoefficient = onePoleCoefficient(15000.0f, rate);
    driveToneCoefficient = onePoleCoefficient(5200.0f, rate);

    for (auto& state : channelStates)
        state = {};
    const auto rampSeconds = 0.025;
    ampDrive.reset(rate, rampSeconds);
    ampVolume.reset(rate, rampSeconds);
    bassGain.reset(rate, rampSeconds);
    middleGain.reset(rate, rampSeconds);
    trebleGain.reset(rate, rampSeconds);
    brightAmount.reset(rate, rampSeconds);
    pedalDrive.reset(rate, rampSeconds);
    pedalTone.reset(rate, rampSeconds);
    pedalLevel.reset(rate, rampSeconds);
    pedalMix.reset(rate, rampSeconds);
    od3Drive.reset(rate, rampSeconds);
    od3Tone.reset(rate, rampSeconds);
    od3Level.reset(rate, rampSeconds);
    od3Mix.reset(rate, rampSeconds);
    cabinetIRMix.reset(rate, rampSeconds);

    const auto volume = parameters.getRawParameterValue(volumeId)->load();
    const auto bass = parameters.getRawParameterValue(bassId)->load();
    const auto middle = parameters.getRawParameterValue(middleId)->load();
    const auto treble = parameters.getRawParameterValue(trebleId)->load();
    const auto bright = parameters.getRawParameterValue(brightId)->load();
    const auto pedalDriveValue = parameters.getRawParameterValue(pedalDriveId)->load();
    const auto pedalToneValue = parameters.getRawParameterValue(pedalToneId)->load();
    const auto pedalLevelValue = parameters.getRawParameterValue(pedalLevelId)->load();
    const auto pedalOn = parameters.getRawParameterValue(pedalEnabledId)->load();
    const auto od3DriveValue = parameters.getRawParameterValue(od3DriveId)->load();
    const auto od3ToneValue = parameters.getRawParameterValue(od3ToneId)->load();
    const auto od3LevelValue = parameters.getRawParameterValue(od3LevelId)->load();
    const auto od3On = parameters.getRawParameterValue(od3EnabledId)->load();
    const auto irOn = parameters.getRawParameterValue(irEnabledId)->load();

    ampDrive.setCurrentAndTargetValue(1.25f + volume * 0.18f);
    ampVolume.setCurrentAndTargetValue(dbToGain(-8.0f + volume * 1.6f));
    bassGain.setCurrentAndTargetValue(dbToGain((bass - 5.0f) * 1.35f));
    middleGain.setCurrentAndTargetValue(dbToGain(-3.5f + (middle - 5.0f) * 1.35f));
    trebleGain.setCurrentAndTargetValue(dbToGain((treble - 5.0f) * 1.3f));
    brightAmount.setCurrentAndTargetValue(bright >= 0.5f ? 1.0f : 0.0f);
    pedalDrive.setCurrentAndTargetValue(pedalDriveValue);
    pedalTone.setCurrentAndTargetValue(pedalToneValue);
    pedalLevel.setCurrentAndTargetValue(pedalLevelValue);
    pedalMix.setCurrentAndTargetValue(pedalOn >= 0.5f ? 1.0f : 0.0f);
    od3Drive.setCurrentAndTargetValue(od3DriveValue);
    od3Tone.setCurrentAndTargetValue(od3ToneValue);
    od3Level.setCurrentAndTargetValue(od3LevelValue);
    od3Mix.setCurrentAndTargetValue(od3On >= 0.5f ? 1.0f : 0.0f);
    od3ToneCoefficient = onePoleCoefficient(650.0f + od3ToneValue * 430.0f, rate);
    cabinetIRMix.setCurrentAndTargetValue(irOn >= 0.5f && cabinetIRLoaded.load() ? 1.0f : 0.0f);

    const auto outputChannels = juce::jmax(1, getTotalNumOutputChannels());
    builtInCabinetBuffer.setSize(outputChannels, juce::jmax(1, samplesPerBlock), false, false, true);
    cabinetBlendBuffer.setSize(1, juce::jmax(1, samplesPerBlock), false, false, true);
    cabinetConvolution.prepare({ rate, maxBlockSize, static_cast<juce::uint32>(outputChannels) });
    setLatencySamples(cabinetConvolution.getLatency());

    inputMeter.store(0.0f, std::memory_order_relaxed);
    outputLeftMeter.store(0.0f, std::memory_order_relaxed);
    outputRightMeter.store(0.0f, std::memory_order_relaxed);
    testTonePhase = 0.0f;
}

void NkbTwinAudioProcessor::releaseResources()
{
    cabinetConvolution.reset();
}

bool NkbTwinAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto input = layouts.getMainInputChannelSet();
    const auto output = layouts.getMainOutputChannelSet();

    if (input.isDisabled())
        return output == juce::AudioChannelSet::mono() || output == juce::AudioChannelSet::stereo();

    if (input == juce::AudioChannelSet::mono())
        return output == juce::AudioChannelSet::mono() || output == juce::AudioChannelSet::stereo();

    return input == juce::AudioChannelSet::stereo() && output == juce::AudioChannelSet::stereo();
}

void NkbTwinAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const auto inputChannels = getTotalNumInputChannels();
    const auto outputChannels = getTotalNumOutputChannels();
    const auto numSamples = buffer.getNumSamples();
    const auto irIsLoaded = cabinetIRLoaded.load(std::memory_order_relaxed);

    for (auto channel = inputChannels; channel < outputChannels; ++channel)
        buffer.clear(channel, 0, numSamples);

    const auto volume = parameters.getRawParameterValue(volumeId)->load(std::memory_order_relaxed);
    const auto bass = parameters.getRawParameterValue(bassId)->load(std::memory_order_relaxed);
    const auto middle = parameters.getRawParameterValue(middleId)->load(std::memory_order_relaxed);
    const auto treble = parameters.getRawParameterValue(trebleId)->load(std::memory_order_relaxed);
    const auto bright = parameters.getRawParameterValue(brightId)->load(std::memory_order_relaxed);
    const auto pedalDriveValue = parameters.getRawParameterValue(pedalDriveId)->load(std::memory_order_relaxed);
    const auto pedalToneValue = parameters.getRawParameterValue(pedalToneId)->load(std::memory_order_relaxed);
    const auto pedalLevelValue = parameters.getRawParameterValue(pedalLevelId)->load(std::memory_order_relaxed);
    const auto pedalOn = parameters.getRawParameterValue(pedalEnabledId)->load(std::memory_order_relaxed);
    const auto od3DriveValue = parameters.getRawParameterValue(od3DriveId)->load(std::memory_order_relaxed);
    const auto od3ToneValue = parameters.getRawParameterValue(od3ToneId)->load(std::memory_order_relaxed);
    const auto od3LevelValue = parameters.getRawParameterValue(od3LevelId)->load(std::memory_order_relaxed);
    const auto od3On = parameters.getRawParameterValue(od3EnabledId)->load(std::memory_order_relaxed);
    const auto irOn = parameters.getRawParameterValue(irEnabledId)->load(std::memory_order_relaxed);

    ampDrive.setTargetValue(1.25f + volume * 0.18f);
    ampVolume.setTargetValue(dbToGain(-8.0f + volume * 1.6f));
    bassGain.setTargetValue(dbToGain((bass - 5.0f) * 1.35f));
    middleGain.setTargetValue(dbToGain(-3.5f + (middle - 5.0f) * 1.35f));
    trebleGain.setTargetValue(dbToGain((treble - 5.0f) * 1.3f));
    brightAmount.setTargetValue(bright >= 0.5f ? 1.0f : 0.0f);
    pedalDrive.setTargetValue(pedalDriveValue);
    pedalTone.setTargetValue(pedalToneValue);
    pedalLevel.setTargetValue(pedalLevelValue);
    pedalMix.setTargetValue(pedalOn >= 0.5f ? 1.0f : 0.0f);
    od3Drive.setTargetValue(od3DriveValue);
    od3Tone.setTargetValue(od3ToneValue);
    od3Level.setTargetValue(od3LevelValue);
    od3Mix.setTargetValue(od3On >= 0.5f ? 1.0f : 0.0f);
    cabinetIRMix.setTargetValue(irIsLoaded && irOn >= 0.5f ? 1.0f : 0.0f);
    driveToneCoefficient = onePoleCoefficient(750.0f + pedalToneValue * 920.0f,
                                               getSampleRate() > 0.0 ? getSampleRate() : 44100.0);
    od3ToneCoefficient = onePoleCoefficient(650.0f + od3ToneValue * 430.0f,
                                             getSampleRate() > 0.0 ? getSampleRate() : 44100.0);

    auto sourceChannel = 0;
    if (inputChannels > 1)
    {
        std::array<float, 2> inputPeaks{};
        for (auto channel = 0; channel < juce::jmin(inputChannels, 2); ++channel)
            for (auto sample = 0; sample < numSamples; ++sample)
                inputPeaks[static_cast<size_t>(channel)] = juce::jmax(
                    inputPeaks[static_cast<size_t>(channel)], std::abs(buffer.getSample(channel, sample)));
        if (inputPeaks[1] > inputPeaks[0])
            sourceChannel = 1;
    }

    float inPeak = 0.0f;
    for (auto sample = 0; sample < numSamples; ++sample)
    {
        const auto drive = ampDrive.getNextValue();
        const auto volumeGain = ampVolume.getNextValue();
        const auto lowGain = bassGain.getNextValue();
        const auto midGain = middleGain.getNextValue();
        const auto highGain = trebleGain.getNextValue();
        const auto brightMix = brightAmount.getNextValue();
        const auto pedalDriveAmount = pedalDrive.getNextValue();
        const auto pedalToneAmount = pedalTone.getNextValue();
        const auto pedalLevelAmount = pedalLevel.getNextValue();
        const auto pedalBlend = pedalMix.getNextValue();
        const auto od3DriveAmount = od3Drive.getNextValue();
        const auto od3ToneAmount = od3Tone.getNextValue();
        const auto od3LevelAmount = od3Level.getNextValue();
        const auto od3Blend = od3Mix.getNextValue();
        const auto irBlend = cabinetIRMix.getNextValue();
        cabinetBlendBuffer.setSample(0, sample, irBlend);

        const auto useTestTone = testToneEnabled.load(std::memory_order_relaxed);
        const auto testTone = useTestTone ? 0.1f * std::sin(testTonePhase) : 0.0f;
        if (useTestTone)
        {
            testTonePhase += juce::MathConstants<float>::twoPi * 440.0f / testToneSampleRate;
            if (testTonePhase >= juce::MathConstants<float>::twoPi)
                testTonePhase -= juce::MathConstants<float>::twoPi;
        }

        const auto dry = useTestTone
                             ? testTone
                             : (inputChannels > 0 ? buffer.getSample(sourceChannel, sample) : 0.0f);
        inPeak = juce::jmax(inPeak, std::abs(dry));
        const auto ampInput = dry * 1.35f;

        auto& state = channelStates[0];

        // Schematic-informed BD-2 stages: clean JFET-like boost, a fixed
        // Fender-style contour, two diode-pair clipping stages, a gain-linked
        // post-clip boost, then the passive Tone/Level controls and buffer.
        state.pedalInputLow += pedalInputCoefficient * (dry - state.pedalInputLow);
        auto stage1 = (dry - state.pedalInputLow) * (1.5f + pedalDriveAmount * 1.3f);
        stage1 /= 1.0f + 0.055f * std::abs(stage1);

        state.pedalStackLow += pedalStackLowCoefficient * (stage1 - state.pedalStackLow);
        state.pedalStackHigh += pedalStackHighCoefficient * (stage1 - state.pedalStackHigh);
        const auto pedalLowBand = state.pedalStackLow;
        const auto pedalMidBand = state.pedalStackHigh - state.pedalStackLow;
        const auto pedalHighBand = stage1 - state.pedalStackHigh;
        auto stage2 = pedalLowBand + pedalMidBand * 0.88f + pedalHighBand * 1.06f;

        stage2 = diodePairTransfer(stage2, 0.42f);
        stage2 = diodePairTransfer(stage2, 0.55f);
        auto stage4 = stage2 * (1.0f + pedalDriveAmount * 0.72f);
        stage4 = diodePairTransfer(stage4, 0.35f);

        state.pedalToneLow += driveToneCoefficient * (stage4 - state.pedalToneLow);
        auto pedalSignal = state.pedalToneLow
                         + (stage4 - state.pedalToneLow) * (0.2f + pedalToneAmount * 0.08f);
        pedalSignal *= dbToGain((pedalLevelAmount - 5.0f) * 2.0f - 6.0f);
        state.pedalBufferLow += pedalBufferCoefficient * (pedalSignal - state.pedalBufferLow);
        pedalSignal = state.pedalBufferLow;
        auto signal = ampInput + (pedalSignal - ampInput) * pedalBlend;

        // OD-3-inspired dual-stage overdrive: input shaping, two soft-clipping
        // gain stages, a broad Tone roll-off, and a buffered Level control.
        // This is a musical circuit-inspired model, not a component-level SPICE simulation.
        state.od3InputLow += od3InputCoefficient * (signal - state.od3InputLow);
        const auto od3Input = state.od3InputLow * 0.78f + (signal - state.od3InputLow) * 1.08f;
        auto od3Stage1 = tubeStage(od3Input, 1.15f + od3DriveAmount * 0.25f, 0.055f);
        od3Stage1 *= 1.0f + od3DriveAmount * 0.52f;
        od3Stage1 = diodePairTransfer(od3Stage1, 0.92f - od3DriveAmount * 0.045f);
        auto od3Stage2 = tubeStage(od3Stage1, 1.35f + od3DriveAmount * 0.34f, -0.035f);
        od3Stage2 = diodePairTransfer(od3Stage2 * (1.0f + od3DriveAmount * 0.28f),
                                      0.82f - od3DriveAmount * 0.035f);
        state.od3ToneLow += od3ToneCoefficient * (od3Stage2 - state.od3ToneLow);
        const auto toneBlend = od3ToneAmount * 0.1f;
        auto od3Signal = state.od3ToneLow + (od3Stage2 - state.od3ToneLow) * toneBlend;
        od3Signal *= dbToGain((od3LevelAmount - 5.0f) * 1.8f - 1.0f);
        state.od3BufferLow += od3BufferCoefficient * (od3Signal - state.od3BufferLow);
        od3Signal = state.od3BufferLow;
        signal += (od3Signal - signal) * od3Blend;

        // Twin-style clean preamp with modest, asymmetric tube compression.
        signal = tubeStage(signal, drive, 0.035f);

        state.lowSplit += lowSplitCoefficient * (signal - state.lowSplit);
        state.highSplit += highSplitCoefficient * (signal - state.highSplit);
        const auto lowBand = state.lowSplit;
        const auto midBand = state.highSplit - state.lowSplit;
        const auto highBand = signal - state.highSplit;
        signal = lowBand * lowGain + midBand * midGain + highBand * highGain;

        // The Bright cap is most audible at low Volume settings, across the
        // volume control, before the following voltage-amplifier stage.
        state.brightLow += brightCoefficient * (signal - state.brightLow);
        const auto brightDepth = brightMix * (1.0f - volume * 0.075f);
        signal += (signal - state.brightLow) * (0.48f * brightDepth);
        signal = tubeStage(signal, 1.05f + volume * 0.06f, -0.025f);

        state.rumbleLow += rumbleCoefficient * (signal - state.rumbleLow);
        signal -= state.rumbleLow;

        // Broad 2x12 speaker voicing: low resonance, restrained low mids,
        // a clear upper-mid presence band, then a steep top-end roll-off.
        state.speakerBassLow += speakerBassCoefficient * (signal - state.speakerBassLow);
        state.speakerLowMid += speakerLowMidCoefficient * (signal - state.speakerLowMid);
        state.speakerMid += speakerMidCoefficient * (signal - state.speakerMid);
        state.speakerPresence += speakerPresenceCoefficient * (signal - state.speakerPresence);
        state.speakerAir += speakerAirCoefficient * (signal - state.speakerAir);
        const auto speakerBass = state.speakerBassLow;
        const auto speakerLowMid = state.speakerLowMid - state.speakerBassLow;
        const auto speakerMid = state.speakerMid - state.speakerLowMid;
        const auto speakerPresence = state.speakerPresence - state.speakerMid;
        const auto speakerUpper = state.speakerAir - state.speakerPresence;
        const auto speakerAir = signal - state.speakerAir;
        const auto speakerSignal = speakerBass * 1.15f + speakerLowMid * 0.94f
                                 + speakerMid * 0.88f + speakerPresence * 1.12f
                                 + speakerUpper * 0.72f + speakerAir * 0.05f;
        const auto builtInCabinet = speakerSignal * volumeGain;
        const auto irInput = signal * volumeGain;

        for (auto channel = 0; channel < outputChannels; ++channel)
        {
            builtInCabinetBuffer.setSample(channel, sample, builtInCabinet);
            buffer.setSample(channel, sample, irIsLoaded ? irInput : builtInCabinet);
        }
    }

    if (irIsLoaded && outputChannels > 0)
    {
        auto block = juce::dsp::AudioBlock<float>(buffer).getSubsetChannelBlock(
            0, static_cast<size_t>(outputChannels));
        cabinetConvolution.process(juce::dsp::ProcessContextReplacing<float>(block));

        for (auto channel = 0; channel < outputChannels; ++channel)
            for (auto sample = 0; sample < numSamples; ++sample)
            {
                const auto blend = cabinetBlendBuffer.getSample(0, sample);
                const auto dry = builtInCabinetBuffer.getSample(channel, sample);
                const auto wet = buffer.getSample(channel, sample);
                buffer.setSample(channel, sample, dry + (wet - dry) * blend);
            }
    }

    std::array<float, 2> outputPeaks{};
    for (auto channel = 0; channel < juce::jmin(outputChannels, 2); ++channel)
        for (auto sample = 0; sample < numSamples; ++sample)
            outputPeaks[static_cast<size_t>(channel)] = juce::jmax(
                outputPeaks[static_cast<size_t>(channel)], std::abs(buffer.getSample(channel, sample)));

    inputMeter.store(inPeak, std::memory_order_relaxed);
    outputLeftMeter.store(outputPeaks[0], std::memory_order_relaxed);
    outputRightMeter.store(outputChannels > 1 ? outputPeaks[1] : outputPeaks[0], std::memory_order_relaxed);
}

bool NkbTwinAudioProcessor::loadCabinetImpulseResponse(const juce::File& file)
{
    if (!file.existsAsFile())
        return false;

    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(file));
    if (reader == nullptr || reader->numChannels == 0 || reader->numChannels > 2
        || reader->lengthInSamples <= 0)
        return false;

    const auto stereo = reader->numChannels > 1 ? juce::dsp::Convolution::Stereo::yes
                                                 : juce::dsp::Convolution::Stereo::no;
    cabinetConvolution.loadImpulseResponse(file, stereo,
                                            juce::dsp::Convolution::Trim::yes, 0,
                                            juce::dsp::Convolution::Normalise::yes);
    cabinetIRPath = file.getFullPathName();
    cabinetIRLoaded.store(true, std::memory_order_relaxed);
    if (auto* parameter = parameters.getParameter(irEnabledId))
        parameter->setValueNotifyingHost(1.0f);
    return true;
}

void NkbTwinAudioProcessor::clearCabinetImpulseResponse()
{
    cabinetIRPath.clear();
    cabinetIRLoaded.store(false, std::memory_order_relaxed);
    cabinetConvolution.reset();
    if (auto* parameter = parameters.getParameter(irEnabledId))
        parameter->setValueNotifyingHost(0.0f);
}

juce::String NkbTwinAudioProcessor::getCabinetImpulseResponseName() const
{
    return cabinetIRPath.isNotEmpty() ? juce::File(cabinetIRPath).getFileName() : juce::String{};
}

void NkbTwinAudioProcessor::setParameterToDefault(const juce::String& parameterId)
{
    if (auto* parameter = parameters.getParameter(parameterId))
        parameter->setValueNotifyingHost(parameter->getDefaultValue());
}

juce::AudioProcessorEditor* NkbTwinAudioProcessor::createEditor()
{
    return new NkbTwinAudioProcessorEditor(*this);
}

void NkbTwinAudioProcessor::getStateInformation(juce::MemoryBlock& destinationData)
{
    auto state = parameters.copyState();
    state.setProperty(cabinetIRPathProperty, cabinetIRPath, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, destinationData);
}

void NkbTwinAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
    {
        if (!xml->hasTagName(stateTag))
            return;

        auto restoredState = juce::ValueTree::fromXml(*xml);
        const auto restoredIRPath = restoredState.getProperty(cabinetIRPathProperty).toString();
        restoredState.removeProperty(cabinetIRPathProperty, nullptr);
        parameters.replaceState(restoredState);

        if (restoredIRPath.isNotEmpty() && juce::File(restoredIRPath).existsAsFile())
        {
            const auto shouldEnable = parameters.getRawParameterValue(irEnabledId)->load() >= 0.5f;
            loadCabinetImpulseResponse(juce::File(restoredIRPath));
            if (!shouldEnable)
                setParameterToDefault(irEnabledId);
        }
        else
        {
            cabinetIRPath.clear();
            cabinetIRLoaded.store(false, std::memory_order_relaxed);
            setParameterToDefault(irEnabledId);
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NkbTwinAudioProcessor();
}
