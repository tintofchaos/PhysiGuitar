#pragma once

#include <JuceHeader.h>

class GuitarSound : public juce::SynthesiserSound
{
public:
    GuitarSound() 
    {
    }
    
    bool appliesToNote (int) override    { return true; }
    bool appliesToChannel (int) override    { return true; }
};