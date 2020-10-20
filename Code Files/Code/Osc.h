/*
  ==============================================================================

    Osc.h
    Created: 10 Apr 2020 6:05:05pm
    Author:  hadi nisamudeen
	JUST A DEFAULT TRIANGLE WAVE OSCILLATOR
  ==============================================================================
*/

#pragma once
#ifndef Osc_h
#define Osc_h
#include "../JuceLibraryCode/JuceHeader.h"
#include <stdlib.h>
class Osc {
public:

	void setSampleRate(const double& sampleRate) {
		sampleRateInv = 2.f / float(sampleRate);
	}

	void setNote(const int& note) {
		setFreq(pow(2.f, float(note - 69) / 12.f*440.f));
	}

	void setFreq(const float& freq) {
		inc = sampleRateInv * freq;
	}

	void process() {
		phase += inc;
		if (phase > 1.f)
			phase -= 2.f;
	}

	const float& getTriangleWave() {
		if (phase > .5f)
			return 2.f - 2.f * phase;
		else if (phase > -.5f)
			return 2.f * phase;
		else
			return -2.f - 2.f * phase;
	}

private:
	float phase, sampleRateInv, inc;
};



#endif