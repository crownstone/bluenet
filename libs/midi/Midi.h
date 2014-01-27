#include <stdint.h>
#include "Serializable.h"

#define NoteOff 0
#define NoteOn 1
#define AfterTouchPoly 2
#define ControlChange 3
#define ProgramChange 4
#define AfterTouchChannel 5
#define PitchBend 6
#define SystemExclusive 7



class MidiMessage : public ISerializable {



};

class MidiNoteMessage : public MidiMessage {
  private:
    uint8_t bytes[3];

  public:
    MidiNoteMessage() : MidiNoteMessage(false, 0, 0, 0) {}
    MidiNoteMessage(bool on, uint8_t channel, uint8_t note, uint8_t velocity) {
        bytes[0] = 1 << 7 | on << 4 | (channel & 0b1111);
        bytes[1] = ~(1 << 7) | note;
        bytes[2] = ~(1 << 7) | velocity;
    }

    MidiNoteMessage& setNoteOn(bool on = true) {
        if (on) {
            bytes[0] |= 1 << 7;
        } else {
            setNoteOff();
        }
        return *this;
    }

    MidiNoteMessage& setNoteOff() {
        bytes[0] &= ~(1<<7);
        return *this;
    }

    MidiNoteMessage& setChannel(uint8_t channel) {
        bytes[0] |= (channel & 0b1111);
        return *this;
    }

    MidiNoteMessage& setNote(uint8_t note) {
        bytes[1] = ~(1 << 7) | note;
        return *this;
    }

    MidiNoteMessage& setVelocity(uint8_t velocity) {
        bytes[2] = ~(1 << 7) | velocity;
        return *this;
    }

    virtual uint32_t getSerializedLength() const {
        return 0; // we have enough space internally.
    }

    virtual void serialize(Buffer& buffer) const {
        buffer.length = 3;
        buffer.data = (uint8_t*)bytes;
    }

    virtual void deserialize(Buffer& buffer) {
        if (buffer.length < 3) return;
        for(int i = 0; i < 3; ++i) bytes[i] = buffer.data[i];
    }

    bool operator==(const MidiNoteMessage& rhs) const {
        for(int i = 0; i < 3; ++i) {
            if (bytes[i] != rhs.bytes[i]) return false;
        }
        return true;
    }
    bool operator!=(const MidiNoteMessage &other) const {
        return !(*this == other);
    }
};

class Midi {

    static MidiNoteMessage note_on(uint8_t channel, uint8_t note, uint8_t velocity) {
        return MidiNoteMessage(true, channel, note, velocity);
    }

};
