#pragma once

#include "JuceHeader.h"

class HeadphonesButton : public juce::DrawableButton {
public:

	HeadphonesButton(const juce::String& buttonName) : DrawableButton(buttonName, juce::DrawableButton::ImageOnButtonBackground) {
        using namespace juce;

        headphonesPath.setPath(Drawable::parseSVGPath(headphonesSVG));
        headphonesPath.setFill(FillType(Colours::whitesmoke));
        setImages(&headphonesPath);
        setColour(ComboBox::outlineColourId, Colours::transparentWhite);
        setColour(TextButton::buttonColourId, Colours::transparentWhite);

        setClickingTogglesState(true);
    };

    juce::Rectangle<float> getImageBounds() const override {
        const float border = 0.25f;
        const float xBorder = border * getWidth();
        const float yBorder = border * getHeight();
        return juce::Rectangle<float>(xBorder/2, yBorder/2, getWidth() - xBorder, getHeight() - yBorder);
    }

private:
    juce::DrawablePath headphonesPath;

    const char* headphonesSVG =
        R"(
M 460.25,248.25
           C 460.25,248.25 460.50,329.75 460.00,372.75
             456.75,425.00 408.25,457.50 375.75,460.50
             346.75,460.75 327.75,432.00 327.00,417.00
             327.00,417.00 326.50,337.50 326.75,303.25
             330.75,271.75 359.25,258.00 377.00,258.25
             390.25,258.25 412.75,266.25 426.25,281.75
             436.75,173.00 360.50,85.25 256.00,84.50
             156.25,85.00 76.25,168.50 85.00,282.25
             93.50,268.25 122.75,258.00 137.50,258.50
             157.25,258.75 184.75,280.25 184.75,302.00
             184.75,302.00 184.75,416.00 184.75,416.00
             183.75,440.75 159.25,460.75 137.00,461.25
             103.75,459.25 55.50,422.75 51.75,376.25
             51.75,376.25 51.50,245.25 51.50,245.25
             50.50,156.25 135.25,52.75 256.75,51.50
             361.00,52.75 458.50,137.00 460.25,248.25 Z
)";
};