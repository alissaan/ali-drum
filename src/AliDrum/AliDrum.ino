#include <MIDIUSB.h>
#include <TaskScheduler.h>

class Sensor
{
    String Name;                // Name of sensor
    int Pin;                    // Stands for analog port (0 = A0, 1 = A1, ...).
    int Note;                   // MIDI note.
    int CutOff;                 // Disallow hit values below this limit.
    bool UseVelocitySenstivity; // Specifies that velocity will be calculated based on hit value.

public:
    int MaxHitValue; // Specifies a max value expected to be read by the piezo sensor. This value is used in velocity calculation.
public:
    int MinVelocity; // Minimum velocity value that will be used in velocity calculation.
public:
    int MaxVelocity; // Maximum velocity value that will be used in velocity calculation.
public:
    int MaxPlayTime; // How many cycles to wait before allow a second hit.

public:
    bool Debug;

public:
    int Value;

    bool IsActive;
    int PlayTime;

public:
    Sensor(String name, int pin, int note, int cutOff, bool useVelocitySensitity = true)
    {
        Name = name;
        Pin = pin;
        Note = note;
        CutOff = cutOff;
        UseVelocitySenstivity = useVelocitySensitity;
    }

    void ControlPlaytime()
    {
        if (PlayTime > MaxPlayTime)
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
        if (MaxHitValue == 0 || MaxHitValue > 1023 || MaxHitValue < CutOff)
        {
            MaxHitValue = 1023;
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
        String message = (String)Name + " => Pin: " + Pin + ", Note: " + Note + ", CutOff: " + CutOff + ", UseVelocitySensitivy: " + UseVelocitySenstivity + ", MaxHitValue: " + MaxHitValue + ", MinVelocity: " + MinVelocity + ", MinVelocity: " + MaxVelocity + ", MaxPlayTime: " + MaxPlayTime + ", PlayTime: " + PlayTime;

        Serial.println(message);
    }

    bool Handle()
    {
        Value = analogRead(Pin);
        int dValue = analogRead(Pin);
        Value = dValue > Value ? dValue : Value;

        if (Value > CutOff)
        {
            if (!IsActive)
            {
                SendNote();
                Value = 0;
                ControlPlaytime();
            }
        }
        else if (IsActive)
        {
            ControlPlaytime();
        }

        return true;
    }

    void SendNote()
    {
        int velocity = 0;

        if (UseVelocitySenstivity)
        {
            if (Value > MaxHitValue)
            {
                velocity = MaxVelocity;
            }
            else
            {
                velocity = abs(MaxVelocity / abs((MaxHitValue - CutOff) / (Value - CutOff)));
                velocity = map(Value, 0, MaxHitValue, 1, 127);

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
        MidiUSB.flush();

        midiEventPacket_t noteOff = {0x08, 0x80 | 1, (uint8_t)Note, (uint8_t)0};
        MidiUSB.sendMIDI(noteOff);
        MidiUSB.flush();
    }
};

Sensor sensors[1]{Sensor("S1", 0, 52, 200, true)};

void setup()
{
    sensors[0].Debug = true;
    sensors[0].MaxPlayTime = 100;
    sensors[0].IsActive = false;
    sensors[0].MaxHitValue = 600;
    sensors[0].MinVelocity = 15;
    sensors[0].Normalize();
}

void loop()
{
    sensors[0].Handle();
    delayMicroseconds(10);
}