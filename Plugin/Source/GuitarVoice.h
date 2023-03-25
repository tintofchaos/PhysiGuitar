#pragma once

#include "GuitarSound.h"

#include "DSP/PluckedString.h"
#include "DSP/Pickup.h"

PluckedString *strings[6];
Pickup *pickups[6];

int n = 0;

class GuitarVoice : public juce::SynthesiserVoice
{
    int success;
    int initialized; // check if a note has been played
public:
    GuitarVoice()
    {
        initialized = 0;
        if (n > 5) n = 0;
        string_init(&guitar_string, (int) getSampleRate(), 440.f );
        string_setstiffness(&guitar_string, 0.01);
        string_setposition(&guitar_string, 0.23);
        string_setdamping(&guitar_string, 0.35);
        string_setmuting(&guitar_string, 0.2);
        string_setfrequency(&guitar_string, 20.f);
        
        if (pickup_init(&pickup, (int) getSampleRate() ) ) success = 1;
        else success = 0;
        pickup_setposition(&pickup, 6.375 / 25.5);
        pickup_setpickup(&pickup, 5000.f, 0.707, PICKUP_PRESET_STRAT, 1.f);
        pickups[n] = &pickup;
        strings[n++] = &guitar_string;
        gain = 0;
        release = 0;
        pitch_bend = 0;
    }
    
    ~GuitarVoice()
    {
        pickup_free(&pickup);
        n = 0;
    }
    
    bool canPlaySound(juce::SynthesiserSound* sound) override
    {
        return dynamic_cast<GuitarSound*> (sound) != NULL && success == 1;
    }
    
    void startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound*, int) override
    {
        midi_note = (float) midiNoteNumber;
        auto cyclesPerSecond = juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);
        string_noteon(&guitar_string, cyclesPerSecond, velocity);
        pickup_setfrequency(&pickup, cyclesPerSecond);
        release = 0;
        gain = 0;
        initialized = 1;
    }

    void stopNote (float, bool allowTailOff) override
    {
        release = 1;
    }    
    
    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override
    {
        while (--numSamples >= 0)
        {
            auto currentSample = pickup_process(&pickup, string_process(&guitar_string) ) * gain;
                        
            if (!initialized) currentSample = 0;
                        
            for (auto i = outputBuffer.getNumChannels(); --i >= 0;)                     
                outputBuffer.addSample (i, startSample, currentSample);
                    
            ++startSample;
                                        
            if (0 < gain && release) gain -= 1.f / (getSampleRate() / 5.f);
            if (1 > gain && !release) gain += 1.f / (getSampleRate() / 40.f);

            // end sound at the next zero point
                                        
            if (release && currentSample <= 1e-07 && gain <= 1e-07)
            {
                string_noteoff(&guitar_string);
                clearCurrentNote();   
                break;
            }
            
        }
    }
    
    void pitchWheelMoved (int newPitchWheelValue) override
    {
        if (newPitchWheelValue - 8192 > 0)
            pitch_bend = (float) (newPitchWheelValue - 8192) / 8191.f * 2.f;
                    
        float frequency = 440.f * powf(2.f, (midi_note + pitch_bend - 69.f) / 12.f);
        
        pickup_setfrequency(&pickup, frequency);
        string_setfrequency(&guitar_string, frequency);

    }
    
    void controllerMoved (int, int) override
    {
    }
    
    using juce::SynthesiserVoice::renderNextBlock;
    
    PluckedString guitar_string;
    Pickup pickup;
    float gain;
    int release;
    float midi_note;
    float pitch_bend;
};