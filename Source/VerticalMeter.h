#pragma once

#include <JuceHeader.h>

class VerticalMeter : public juce::Component {
public:
	
	void paint(juce::Graphics& g) override {
		using namespace juce;
		
		const float cornerSize = 3.f;
		auto bounds = getLocalBounds().toFloat();

		g.setColour(Colours::white.withBrightness(0.4f));
		g.fillRoundedRectangle(bounds, cornerSize);

		g.setColour(Colours::white);
		float scaledY = jmap(level, MIN_LEVEL, 0.f, 0.f, static_cast<float>(getHeight()));
		g.fillRoundedRectangle(bounds.removeFromBottom(scaledY), cornerSize);
	}

	void setLevel(float value) {
		if (value >= level) {
			level = value;
		} else {
			level = level + (value - level) * 0.1;
		}
	}

	const float MIN_LEVEL = -45.f;

private:
	float level = MIN_LEVEL;
};