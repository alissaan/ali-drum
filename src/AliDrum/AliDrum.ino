#include <MIDIUSB.h>
#include <TaskScheduler.h>

struct Sensor
{
    String Name;                // Name of sensor
    int Pin;                    // Stands for analog port (0 = A0, 1 = A1, ...).
    int Note;                   // MIDI note.
    int Threshold;              // Disallow hit values below this limit.
    bool UseVelocitySenstivity; // Specifies that velocity will be calculated based on hit value.

public:
    int MaxValue; // Specifies a max value expected to be read by the piezo sensor. This value is used in velocity calculation.
public:
    int MinVelocity; // Minimum velocity value that will be used in velocity calculation.
public:
    int MaxVelocity; // Maximum velocity value that will be used in velocity calculation.
public:
    int MaxPlayTime; // How many cycles to wait before allow a second hit.

    int DecayCount;
    unsigned long DecayTime;

public:
    bool Debug;

public:
    int Value;

    bool IsActive;
    int PlayTime;

    int ScanTime = 2;

    bool LockDecay;

public:
    Sensor(String name, int pin, int note, int threshold, bool useVelocitySensitity = true)
    {
        Name = name;
        Pin = pin;
        Note = note;
        Threshold = threshold;
        UseVelocitySenstivity = useVelocitySensitity;
        Value = 0;
        DecayTime = 0;
    }

    void ControlPlaytime()
    {
        if (PlayTime >= MaxPlayTime)
        {
            IsActive = false;
            PlayTime = 0;
        }
        else
        {
            IsActive = true;
            PlayTime++;
        }
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

        if (MaxPlayTime == 0)
        {
            MaxPlayTime = 90;
        }
    }

    void ShowParameters()
    {
        String message = (String)Name + " => Pin: " + Pin + ", Note: " + Note + ", CutOff: " + Threshold + ", UseVelocitySensitivy: " + UseVelocitySenstivity + ", MaxHitValue: " + MaxValue + ", MinVelocity: " + MinVelocity + ", MinVelocity: " + MaxVelocity + ", MaxPlayTime: " + MaxPlayTime + ", PlayTime: " + PlayTime;

        Serial.println(message);
    }

    int RawThreshold = 0;
    int LastValue = 0;

    bool CheckHit()
    {
        Value = analogRead(Pin);

        unsigned long scanMark = millis();

        while (millis() - scanMark <= ScanTime)
        {
            int dValue = analogRead(Pin);
            Value = dValue > Value ? dValue : Value;
        }

        if (Value > Threshold)
        {
            if (!IsActive)
            {
                RawThreshold = Threshold;
                Threshold = 2000;
                IsActive = true;
                PlayTime = 0;
                return true;
            }
            else
            {
                PlayTime++;
                return false;
            }
        }
        else if (IsActive)
        {
            PlayTime++;

            if (PlayTime > MaxPlayTime)
            {
                Threshold = RawThreshold;
                IsActive = false;
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
                velocity = abs(MaxVelocity / abs((MaxValue - RawThreshold) / (Value - RawThreshold)));
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

        midiEventPacket_t noteOn = {0x09, 0x90 | 1, (uint8_t)Note, (uint8_t)velocity};
        MidiUSB.sendMIDI(noteOn);

        midiEventPacket_t noteOff = {0x08, 0x80 | 1, (uint8_t)Note, (uint8_t)0};
        MidiUSB.sendMIDI(noteOff);
        MidiUSB.flush();
    }
};

Sensor s1 = Sensor("S1", 0, 52, 200, true);

void setup()
{
    s1.Debug = true;
    s1.MaxPlayTime = 60;
    s1.IsActive = false;
    s1.Value = 0;
    s1.MaxValue = 1023;
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
