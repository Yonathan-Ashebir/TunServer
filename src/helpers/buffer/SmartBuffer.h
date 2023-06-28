//
// Created by DELL on 6/28/2023.
//

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#ifndef TUNSERVER_SMARTBUFFER_H
#define TUNSERVER_SMARTBUFFER_H

#include "../Include.h"

template<typename Object=char>
class SmartBuffer {
public:
    SmartBuffer(Object *buf, unsigned int size, void *release = nullptr) : data(
            new Data{buf, size, release}) {};

    /* Generate a smart buffer with backing array of capacity @param todo capacity*/
    explicit SmartBuffer(unsigned int size) : data(new Data{new Object[size]{}, size}) {
    }

    SmartBuffer(SmartBuffer &other) {
        data = other.data;
        data->count++;
    }

    SmartBuffer(SmartBuffer &&other) noexcept {
        data = other.data;
        other.data = nullptr;
    }

    SmartBuffer &operator=(const SmartBuffer &other) { // NOLINT(bugprone-unhandled-self-assignment)
        if (data != other.data) {
            if (data && !--data->count) { //TODO: check whether this decrements the field then retries it again
                if (data->releaseBuffer && !*--data->bufferCount) delete data->releaseBuffer;
                delete data;
            }
            data = other.data;
            data->datacount++;
        }
    }

    SmartBuffer &operator=(SmartBuffer &&other) noexcept {
        if (data != other.data) {
            if (data != nullptr && !--data->count) {
                if (data->releaseBuffer && !*--data->bufferCount) delete data->releaseBuffer;
                delete data;
            }
            data = other.data;
            other.data = nullptr;
        }
    }

    ~SmartBuffer() {
        if (data != nullptr && !--data->count) {
            if (data->releaseBuffer && !*--data->bufferCount) delete data->releaseBuffer;
            delete data;
        }
    }

/*TODO: comparison*/

/** Generates a new smart buffer that is backed by the same array, with independent offset (set to the same value), position, mark, and limit. */
    SmartBuffer copy() {
        SmartBuffer<Object> newBuffer(data->buffer, data->capacity, data->releaseBuffer);
        newBuffer.data->offset = data->offset;
        if (data->releaseBuffer) {
            newBuffer.data->bufferCount = data->bufferCount;
            ++*data->bufferCount;
        }
        return newBuffer;
    }

    /*TODO: make less discarding*/
    template<typename Object2=Object>
    SmartBuffer as(unsigned int off) {
        if (off > data->capacity)throw out_of_range("Off can not exceed the capacity");
        auto totalOff = (data->capacity - data->offset > off) ? data->offset + off : off -
                                                                                     (data->capacity - data->offset);
        unsigned int newOff = (totalOff * sizeof(Object)) / sizeof(Object2);
        unsigned int remaining = ((data->capacity - totalOff) * sizeof(Object)) / sizeof(Object2);

        SmartBuffer<Object2> newBuffer((Object2 *) (data->buffer + totalOff) - newOff, newOff + remaining,
                                       data->releaseBuffer);
        newBuffer.data->offset = newOff;
        if (data->releaseBuffer) {
            newBuffer.data->bufferCount = data->bufferCount;
            ++*data->bufferCount;
        }
        return newBuffer;
    }

/** Generates a copy of this buffer with position of 0, points to <b>off</b> of this buffer, and the limit is set to <b>limit</b>*/
    SmartBuffer slice(int off, int limit) {
        if (off > data->capacity)throw out_of_range("Off can not exceed the capacity");
        auto totalOff = (data->capacity - data->offset > off) ? data->offset + off : off -
                                                                                     (data->capacity - data->offset);
        totalOff -= data->capacity;
        SmartBuffer newBuffer(data->buffer, data->capacity, data->releaseBuffer);
        newBuffer.data->offset = totalOff;
        newBuffer.setLimit(limit);
        if (data->releaseBuffer) {
            newBuffer.data->bufferCount = data->bufferCount;
            ++*data->bufferCount;
        }
        return newBuffer;
    }

    template<typename Object2=Object>
    Object2 get() {
        return get(data->position++);
    }

    template<typename Object2=Object>
    Object2 get(unsigned int off) {
        if (sizeof(Object2) == 1) {
            if (off >= data->limit)throw out_of_range("Can not read beyond the limit");
            auto totalOff = data->offset + off;
            if (data->capacity > totalOff)return data->buffer[totalOff];
            else return data->buffer[totalOff - data->capacity];
        } else {
            Object obj;
            get(obj, off);
            return obj;
        }
    }


    template<typename Object2=Object>
    SmartBuffer &get(Object2 &obj) {
        get(obj, data->position);
        data->position += (unsigned int) ceil((double) sizeof obj / (double) sizeof(Object));
        return *this;
    }

/**Imp: increments the position by rounding*/
    template<typename Object2=Object>
    SmartBuffer &get(Object2 &obj, unsigned int off) {
        getRaw(off, &obj, sizeof obj);
        return *this;
    }


    template<typename Object2=Object>
    SmartBuffer &get(Object2 objs[], unsigned int len) {
        get(objs, len, data->position);
        data->position += (unsigned int) ceil((double) sizeof(Object2) / (double) sizeof(Object) * len);
        return *this;
    };

    template<typename Object2=Object>
    SmartBuffer &get(Object2 objs[], unsigned int len, unsigned int off) {
        getRaw(off, objs, len * sizeof(Object2));
        return *this;
    }

    template<typename Object2=Object>
    unsigned int
    writeTo(function<unsigned int(Object2 *buf, unsigned int len)> writer) {
        auto written = writeTo(writer, data->position);
        data->position += written;
        return written;
    }

    template<typename Object2=Object>
    unsigned int
    writeTo(function<unsigned int(Object2 *buf, unsigned int len)> writer, unsigned int off,
            unsigned int max = numeric_limits<unsigned int>::max()) {
        alter(writer, off, max);
    }

    template<typename Object2=Object>
    SmartBuffer &put(Object2 &obj) {
        put(obj, data->position);
        data->position += (unsigned int) ceil((double) sizeof obj / (double) sizeof(Object));
        return *this;
    }

    template<typename Object2=Object>
    SmartBuffer &put(Object2 &obj, unsigned int off) {
        putRaw(off, &obj, sizeof obj);
        return *this;
    }

    template<typename Object2=Object>
    SmartBuffer &put(Object2 objs[], unsigned int len) {
        put(objs, len, data->position);
        data->position += (unsigned int) ceil((double) sizeof(Object2) / (double) sizeof(Object) * len);
        return *this;
    }

    template<typename Object2=Object>
    SmartBuffer &put(Object2 objs[], unsigned int len, unsigned int off) {
        putRaw(off, objs, len * sizeof(Object2));
        return *this;
    }

    template<typename Object2=Object>
    unsigned int
    readFrom(function<unsigned int(Object2 *buf, unsigned int len)> writer) {
        auto read = readFrom(writer, data->position);
        data->position += read;
        return read;
    }

    template<typename Object2=Object>
    unsigned int
    readFrom(function<unsigned int(Object2 *buf, unsigned int len)> reader, unsigned int off,
             unsigned int max = numeric_limits<unsigned int>::max()) {
        alter(reader, off, max);
    }

    unsigned int getLimit() {
        return data->limit;
    }


    SmartBuffer &setLimit(unsigned int limit) {
        if (limit > data->capacity)throw out_of_range("Limit can not exceed the capacity");
        data->limit = limit;
        return *this;
    }

    unsigned int getPosition() { return data->position; }

    SmartBuffer &setPosition(unsigned int pos) {
        if (pos > data->limit)throw out_of_range("Position can not exceed the limit");
        data->position = pos;
        return *this;
    }

    SmartBuffer &setMark() {
        return mark(data->position);
    }

    SmartBuffer &setMark(unsigned int off) {
        if (off > data->position)throw out_of_range("Mark can not exceed the position");
        data->isMarked = true;
        data->mark = off;
    }

    unsigned int getMark() {
        return data->mark;
    }

    bool isMarked() {
        return data->isMarked;
    }

    unsigned int getCapacity() {
        return data->capacity;
    }

    unsigned int available() {
        return data->limit - data->position;
    }

    SmartBuffer &compact(unsigned int keep = 0) {
        if (keep > data->position)throw out_of_range("Amount to keep can not exceed position");
        auto off = data->position - keep;
        auto totalOff = (data->capacity - data->offset > off) ? data->offset + off : off -
                                                                                     (data->capacity - data->offset);
        data->offset = totalOff;
        data->position = keep;
        data->limit = data->capacity;
        data->isMarked = false;
    }

    SmartBuffer &clear() {
        data->position = 0;
        data->limit = data->capacity;
        data->isMarked = false;
    }

    SmartBuffer &flip() {
        data->limit = data->position;
        data->position = 0;
        data->isMarked = false;
    }

    SmartBuffer &rewind() {
        data->position = 0;
        data->isMarked = false;
    }

    SmartBuffer &reset() {
        if (!data->isMarked) throw logic_error("Mark is not set before reset call");
        data->position = data->mark;
    }


private:
    struct Data { // NOLINT(cppcoreguidelines-pro-type-member-init)
        laterinit Object *buffer;
        laterinit const unsigned int capacity;
        void *releaseBuffer{};
        unsigned int *bufferCount = new unsigned int{1};
        unsigned int offset{};
        unsigned int position{};
        bool isMarked{};
        laterinit unsigned int mark;
        unsigned int limit{capacity};
        unsigned int count{1};
    } *data;

    inline void getRaw(unsigned int off, void *buf, unsigned int len) {
        if (off + (unsigned int) ceil((double) len / (double) sizeof(Object)) > data->limit) {
            throw out_of_range("Can not read beyond the limit");
        }

        auto totalOff = data->offset + off;
        if (data->capacity > totalOff) {
            auto amt = min(len, data->capacity - totalOff);
            memcpy(buf, data->buffer + totalOff, amt);
            len -= amt;
            buf += amt;
        }
        if (len)
            memcpy(buf, data->buffer, len);
    }

    inline void putRaw(unsigned int off, void *buf, unsigned int len) {
        if (off + (unsigned int) ceil((double) len / (double) sizeof(Object)) > data->limit) {
            throw out_of_range("Can not write beyond the limit");
        }

        auto totalOff = data->offset + off;
        if (data->capacity > totalOff) {
            auto amt = min(len, data->capacity - totalOff);
            memcpy(data->buffer + totalOff, buf, amt);
            len -= amt;
            buf += amt;
        }
        if (len)
            memcpy(data->buffer, buf, len);
    }


    template<typename Object2=Object>
    unsigned int
    alter(function<unsigned int(Object2 *buf, unsigned int len)> fun, unsigned int off,
          unsigned int max = numeric_limits<unsigned int>::max()) {
        auto remaining = max = min(max, data->limit - off);
        auto totalOff = data->offset + off;
        if (data->capacity > totalOff) {
            auto total = min(max, data->capacity - totalOff);
            auto amt = fun((Object2 *) (data->buffer + totalOff), total);
            if (amt < total)return amt;
            remaining -= amt;
        }
        if (remaining) {
            auto amt = fun((Object2 *) data->buffer, remaining);
            remaining -= amt;
        }
        return max - remaining;
    }

    Object *getBuffer() {
        return data->buffer;
    }

    unsigned int getInternalOffset() {
        return data->offset;
    }
};


#endif //TUNSERVER_SMARTBUFFER_H

#pragma clang diagnostic pop