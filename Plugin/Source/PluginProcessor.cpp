/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "GuitarVoice.h"

float prev_pos = 0.f;
float prev_decay = 0.f;
float prev_damping = 0.f;
float prev_width = 0.f;

float prev_pickuppos = 0.f;

float prev_material = 0;
short prev_harmonics = -1;
short prev_quality = -1;

//==============================================================================
PhysiGuitarAudioProcessor::PhysiGuitarAudioProcessor()
{
    addParameter(material = new juce::AudioParameterFloat({"material", 1}, "String Material", 0.0f, 1.f, 1.f) );
    addParameter(pluck_position = new juce::AudioParameterFloat({"pluck_position", 1}, "Pluck Position", 0.01f, 0.99f, 0.23f) );
    addParameter(decay = new juce::AudioParameterFloat({"decay", 1}, "Decay", 0.0f, 1.f, 0.04f) );
    addParameter(damping = new juce::AudioParameterFloat({"damping", 1}, "Damping", 0.0f, 1.f, 0.08f) );
    
    addParameter(pickup_position = new juce::AudioParameterFloat({"pickup_position", 1}, "Pickup Position", 0.0f, 1.f, 6.375 / 25.5) );
    addParameter(tone = new juce::AudioParameterFloat({"tone", 1}, "Tone", 0.0f, 1.f, 1.f) );
    addParameter(bass = new juce::AudioParameterBool({"bass", 1}, "Bass", false) );
    addParameter(width = new juce::AudioParameterFloat({"pick_width", 1}, "Pick Width", 0.0f, 1.f, 0.5) );
    addParameter(harmonics = new juce::AudioParameterBool({"harmonics", 1}, "Natural Harmonics", false) );
    addParameter(quality = new juce::AudioParameterBool({"quality", 1}, "High Quality Mode", false) );
}

PhysiGuitarAudioProcessor::~PhysiGuitarAudioProcessor()
{
    prev_pos = 0.f;
    prev_decay = 0.f;
    prev_damping = 0.f;
    prev_width = 0.f;

    prev_pickuppos = 0.f;

    prev_material = 0;
    prev_harmonics = -1;
    prev_quality = -1;
}

//==============================================================================
const juce::String PhysiGuitarAudioProcessor::getName() const
{
    return "PhysiGuitar";
}

bool PhysiGuitarAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool PhysiGuitarAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool PhysiGuitarAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double PhysiGuitarAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int PhysiGuitarAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int PhysiGuitarAudioProcessor::getCurrentProgram()
{
    return 0;
}

void PhysiGuitarAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String PhysiGuitarAudioProcessor::getProgramName (int index)
{
    return {};
}

void PhysiGuitarAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void PhysiGuitarAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    synth.setCurrentPlaybackSampleRate(sampleRate);
    for (int i = 0; i < 6; i++)
        synth.addVoice(new GuitarVoice());
    
    synth.addSound(new GuitarSound());
}

void PhysiGuitarAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool PhysiGuitarAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void PhysiGuitarAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    auto numSamples = buffer.getNumSamples();
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    
    for (int i = 0; i < 6; i++) {
        if (prev_pos != *pluck_position)
            string_setposition(strings[i], *pluck_position);
        if (prev_decay != *decay)
            string_setdecay(strings[i], *decay);
        if (prev_damping != *damping)
            string_setdamping(strings[i], *damping);
        if (prev_width != *width)
            string_setwidth(strings[i], *width);
            
        if (*harmonics != prev_harmonics)
            string_setharmonics(strings[i], *harmonics);
               
        if (*material != prev_material)
            string_setmaterial(strings[i], *material);
            
        if (*quality != prev_quality)
            string_sethq(strings[i], *quality);
      
        string_update(strings[i]);
        
        if (*pickup_position != prev_pickuppos)
            pickup_setposition(pickups[i], *pickup_position);
            
        if (*bass)
            pickup_setpickup(pickups[i], 5000.f, 0.707, PICKUP_PRESET_BASS, *tone);
        else
            pickup_setpickup(pickups[i], 5000.f, 0.707, PICKUP_PRESET_GUITAR, *tone);
    }
    
    prev_pos = *pluck_position;
    prev_decay = *decay;
    prev_damping = *damping;
    prev_width = *width;
    prev_harmonics = *harmonics;
    prev_pickuppos = *pickup_position;
    
    prev_material = *material;
    prev_quality = *quality;
    
    synth.renderNextBlock(buffer, midiMessages, 0, numSamples);
}

//==============================================================================
bool PhysiGuitarAudioProcessor::hasEditor() const
{
    return false; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* PhysiGuitarAudioProcessor::createEditor()
{
    return nullptr;
}

//==============================================================================
void PhysiGuitarAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::MemoryOutputStream stream(destData, true);
    
    stream.writeFloat(*material);
    stream.writeFloat(*pluck_position);
    stream.writeFloat(*decay);
    stream.writeFloat(*damping);
    stream.writeFloat(*width);
    
    stream.writeFloat(*pickup_position);
    stream.writeFloat(*tone);
    stream.writeBool(*bass);
    stream.writeBool(*harmonics);
    stream.writeBool(*quality);
}

void PhysiGuitarAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::MemoryInputStream stream(data, sizeInBytes, false);
    
    material->setValueNotifyingHost(stream.readFloat() );
    pluck_position->setValueNotifyingHost(stream.readFloat() );
    decay->setValueNotifyingHost(stream.readFloat() );
    damping->setValueNotifyingHost(stream.readFloat() );
    width->setValueNotifyingHost(stream.readFloat() );
    
    pickup_position->setValueNotifyingHost(stream.readFloat() );
    tone->setValueNotifyingHost(stream.readFloat() );
    bass->setValueNotifyingHost(stream.readBool() );
    harmonics->setValueNotifyingHost(stream.readBool() );
    quality->setValueNotifyingHost(stream.readBool() );

}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PhysiGuitarAudioProcessor();
}
