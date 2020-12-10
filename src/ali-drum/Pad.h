#include <MIDIUSB.h>

struct Pad
{
    int Pin;                    // Stands for analog port (0 = A0, 1 = A1, ...).
    int Note;                   // MIDI note.
    int CutOff;                 // Disallow hit values below this limit.
    bool UseVelocitySenstivity; // Specifies that velocity will be calculated based on hit value.
    int HitValuePeak;           // Specifies a max value expected to be read by the piezo sensor. This value is used in velocity calculation.
    int MinVelocity;            // Minimum velocity value that will be used in velocity calculation.
    int MaxVelocity;            // Maximum velocity value that will be used in velocity calculation.
    int MaxPlayTime;            // How many cycles to wait before allow a second hit.

    bool IsActive;
    int PlayTime;

    void ControlPlaytime()
    {
        if (PlayTime > MaxPlayTime)
        {
            PlayTime = 0;
            IsActive = false;
        }
        else
        {
            PlayTime++;
            IsActive = true;
        }
    }

    void Normalize()
    {
        if (HitValuePeak == 0 || HitValuePeak > 1023 || HitValuePeak < CutOff)
        {
            HitValuePeak = 1023;
        }

        if (MinVelocity == 0)
        {
            MinVelocity = 30;
        }

        if (MaxVelocity == 0)
        {
            MaxVelocity = 127;
        }

        if (MaxPlayTime == 0)
        {
            MaxPlayTime = 90;
        }
    }

    void SendNote(int hitValue)
    {
        int velocity = 0;

        if (UseVelocitySenstivity)
        {
            velocity = MaxVelocity / ((HitValuePeak - CutOff) / (hitValue - CutOff)); // With full range (Too sensitive ?)

            if (velocity < MinVelocity)
            {
                velocity = MinVelocity;
            }
            else if (velocity > MaxVelocity)
            {
                velocity = MaxVelocity;
            }
        }
        else
        {
            velocity = MaxVelocity;
        }

        String message = (String) "Hit value: " + hitValue + ", Velocity: " + velocity;
        Serial.println(message);

        midiEventPacket_t noteOn = {0x09, 0x90 | 1, Note, velocity};
        MidiUSB.sendMIDI(noteOn);
        MidiUSB.flush();

        midiEventPacket_t noteOff = {0x08, 0x80 | 1, Note, 0};
        MidiUSB.sendMIDI(noteOff);
        MidiUSB.flush();
    }
};