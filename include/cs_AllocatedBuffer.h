/**
 * @author Christopher Mason
 * @author Anne van Rossum
 * License: TODO
 */
#pragma once

#include <stdint.h>
#include <stdlib.h>

class AllocatedBuffer {
private:
    uint16_t length;
    uint8_t * data;
    bool allocated;

public:
    AllocatedBuffer() : length(0), data(0), allocated(false) {}
    AllocatedBuffer(uint16_t _length, uint8_t* _data) : length(_length), data(_data), allocated(false) {}
    ~AllocatedBuffer() { deallocate(); }
    void allocate(uint16_t size) {
        if (size == 0) return;
        if (allocated && length < size) {
            deallocate();
        }
        data = (uint8_t*)calloc(size, sizeof(uint8_t));
        length = size;
        allocated = true;
    }
    void release() {
        allocated = false;
    }
    void deallocate() {
        if (!data) return;
        free(data);
        data = 0;
        length = 0;
    }
};

//class ISerializable {
//public:
//    /** Return length of buffer required to store the serialized form of this object.  If this method returns 0,
//    * it means that the object does not need external buffer space. */
//    virtual uint32_t getSerializedLength() const = 0;
//
//    /** Copy data representing this object into the given buffer.  Buffer will be preallocated with at least
//    * getLength() bytes. */
//    virtual void serialize(AllocatedBuffer& buffer) const = 0;
//
//    /** Copy data from the given buffer into this object. */
//    virtual void deserialize(AllocatedBuffer& buffer) = 0;
//};
