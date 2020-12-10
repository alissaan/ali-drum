#include "Pad.h"

const int _padCount = 1;

Pad pad1 = {0, 52, 100, true};
Pad pads[_padCount] = {pad1};

void setup()
{
    Serial.begin(9600);

    for (int i = 0; i < _padCount; i++)
    {
        pads[i].Normalize();
    }
}

void loop()
{
    for (int pin = 0; pin < _padCount; pin++)
    {
        Pad pad = pads[pin];
        int hitValue = analogRead(pin);
        int dHitValue = analogRead(pin); // Act as sensor debounce

        hitValue = dHitValue > hitValue ? dHitValue : hitValue;

        if (hitValue > pad.CutOff)
        {
            if (!pad.IsActive)
            {
                pad.SendNote(hitValue);
                pad.ControlPlaytime();
            }
        }
        else if (pad.IsActive)
        {
            pad.ControlPlaytime();
        }

        pads[pin] = pad;
    }
}