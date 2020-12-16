#include <MIDIUSB.h>

struct Sensor
{
private:
    String Name; // Name of sensor
private:
    int Port; // Stands for analog port (0 = A0, 1 = A1, ...).
private:
    int MidiKey; // MIDI key note.
private:
    int Threshold; // Disallow hit values below this limit.
private:
    bool UseVelocitySenstivity; // Specifies that velocity will be calculated based on hit value.

public:
    int MaxValue; // Specifies a max value expected to be read by the piezo sensor. This value is used in velocity calculation.
public:
    int MinVelocity; // Minimum velocity value that will be used in velocity calculation.
public:
    int MaxVelocity; // Maximum velocity value that will be used in velocity calculation.

public:
    bool Debug;

public:
    int Value;

private:
    bool _isActive;

    int ScanTime = 2;
    int MaskTime = 9;

private:
    unsigned long _currentMaskTimeMark = 0;

private:
    int _rawThreshold = 0;

public:
    Sensor(String name, int pin, int note, int threshold, bool useVelocitySensitity = true)
    {
        Name = name;
        Port = pin;
        MidiKey = note;
        Threshold = threshold;
        UseVelocitySenstivity = useVelocitySensitity;
        Value = 0;
    }

    void Normalize()
    {
        if (MaxValue == 0 || MaxValue > 1023 || MaxValue < Threshold)
        {
            MaxValue = 1023;
        }

        if (MinVelocity == 0)
        {
            MinVelocity = 30;
        }

        if (MaxVelocity == 0)
        {
            MaxVelocity = 127;
        }
    }

    

    bool CheckHit()
    {
        Value = analogRead(Port);

        unsigned long scanMark = millis();

        //Seeks to peak input value
        while (millis() - scanMark <= ScanTime)
        {
            int dValue = analogRead(Port);
            Value = dValue > Value ? dValue : Value;
        }

        if (Value > Threshold)
        {
            if (!_isActive)
            {
                _rawThreshold = Threshold;
                Threshold = 2048; // Momentarily increases threshold helping prevent retriggering
                _isActive = true;
                _currentMaskTimeMark = millis();
                return true;
            }
            else
            {
                return false;
            }
        }
        else if (_isActive)
        {
            if (millis() - _currentMaskTimeMark > MaskTime)
            {
                Threshold = _rawThreshold; // Returns to its default value
                _isActive = false;
            }

            return false;
        }
    }

    void SendMidi()
    {
        int velocity = 0;

        if (UseVelocitySenstivity)
        {
            if (Value > MaxValue)
            {
                velocity = MaxVelocity;
            }
            else
            {
                velocity = abs(MaxVelocity / abs((MaxValue - _rawThreshold) / (Value - _rawThreshold)));
                //velocity = map(Value, RawThreshold, MaxValue, 1, 127);

                if (velocity < MinVelocity)
                {
                    velocity = MinVelocity;
                }
                else if (velocity > MaxVelocity)
                {
                    velocity = MaxVelocity;
                }
            }
        }
        else
        {
            velocity = MaxVelocity;
        }

        if (Debug)
        {
            String message = (String)Name + " => Hit value: " + (String)Value + ", Velocity: " + (String)velocity;
            Serial.println(message);
        }

        midiEventPacket_t noteOn = {0x09, 0x90 | 1, (uint8_t)MidiKey, (uint8_t)velocity};
        MidiUSB.sendMIDI(noteOn);

        midiEventPacket_t noteOff = {0x08, 0x80 | 1, (uint8_t)MidiKey, (uint8_t)0};
        MidiUSB.sendMIDI(noteOff);

        MidiUSB.flush();
    }
};

Sensor s1 = Sensor("S1", 0, 52, 200, true);

void setup()
{
    s1.Debug = true;
    s1.MaxVelocity = 127;
    s1.MinVelocity = 1;
    s1.Normalize();
}

void loop()
{
    if (s1.CheckHit())
    {
        s1.SendMidi();
    }
}
