//
// Created by DELL on 6/28/2023.
//

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#ifndef TUNSERVER_SMARTBUFFER_H
#define TUNSERVER_SMARTBUFFER_H

#include "../Include.h"
#include "../math/Utilities.h"

template<typename Object=char>
class SmartBuffer {
private:

public:
    SmartBuffer(Object *buf, unsigned int size, shared_ptr<void> release = nullptr, bool byteSize = false);;

    /* Generate a smart buffer with a backing array of size <i>size<i>*/
    explicit SmartBuffer(unsigned int size);

    SmartBuffer(SmartBuffer &other) = default;

    SmartBuffer(SmartBuffer &&other) noexcept = default;

    SmartBuffer &operator=(const SmartBuffer &other) = default;

    SmartBuffer &operator=(SmartBuffer &&other) noexcept = default;

/** Generates a new smart buffer that is backed by the same array, with independent bytesOffset (set to the same value), position, mark, and limit. */
    SmartBuffer copy();

/** Generates a copy of this buffer with position of 0, points to <b>off</b> of this buffer, and the limit is set to <b>limit</b rounded down to the closest equivalent>*/
    template<typename Object2=Object>
    SmartBuffer<Object2> as(unsigned int off, unsigned int limit);

    template<typename Object2=Object>
    Object2 get();

    template<typename Object2=Object>
    Object2 get(unsigned int off);

    template<typename Object2=Object>
    SmartBuffer &get(Object2 &obj);

/**Imp: increments the position by rounding*/
    template<typename Object2=Object>
    SmartBuffer &get(Object2 &obj, unsigned int off);


    template<typename Object2=Object>
    SmartBuffer &get(Object2 objs[], unsigned int len);;

    template<typename Object2=Object>
    SmartBuffer &get(Object2 objs[], unsigned int len, unsigned int off);

    unsigned int
    manipulate(function<unsigned int(char *, unsigned int)> fun);

    /*Manipulate bytes of at most <b>max</b> no of elements starting from <b>off</b>. Returns number of bytes manipulated.*/
    unsigned int
    manipulate(function<unsigned int(char *buf, unsigned int len)> fun, unsigned int off,
               unsigned int max = numeric_limits<unsigned int>::max());

    template<typename Object2=Object>
    SmartBuffer &put(Object2 &&obj);

    template<typename Object2=Object>
    SmartBuffer &put(Object2 &obj);

    template<typename Object2=Object>
    SmartBuffer &put(Object2 &&obj, unsigned int off);

    template<typename Object2=Object>
    SmartBuffer &put(Object2 &obj, unsigned int off);

    template<typename Object2=Object>
    SmartBuffer &put(Object2 objs[], unsigned int len);

    template<typename Object2=Object>
    SmartBuffer &put(Object2 objs[], unsigned int len, unsigned int off);

    unsigned int getLimit();

    SmartBuffer &setLimit(unsigned int limit);

    unsigned int getPosition();

    SmartBuffer &setPosition(unsigned int pos);

    SmartBuffer &setMark();

    SmartBuffer &setMark(unsigned int off);

    unsigned int getMark();

    bool isMarked();

    unsigned int getCapacity();

    unsigned int getRawCapacity();

    unsigned int available();

/*Sets the start of the buffer to position - keep, with a position set to keep. The mark, if set, is discarded. */
    SmartBuffer &compact(unsigned int keep = 0);

    SmartBuffer &clear() {
        data->position = 0;
        data->limit = data->byteCapacity / sizeof(Object);
        data->isMarked = false;
        return *this;
    }

    SmartBuffer &flip();

    SmartBuffer &rewind();

    SmartBuffer &reset();

    Object *getBuffer();

    unsigned int getBytesOffset();

private:
    struct Data { // NOLINT(cppcoreguidelines-pro-receiveType-member-init)
        char *buffer;
        size_t byteCapacity; /*In bytes*/
        shared_ptr<void> releaseBuffer{buffer};
        size_t bytesOffset{}; /*In bytes*/
        unsigned int position{}; /*In Objs*/
        bool isMarked{};
        unsigned int mark{}; /*In Objs*/
        unsigned int limit{(unsigned int) (byteCapacity /
                                           sizeof(Object))}; /*In Objs*/ //IMP: A narrowing cast could have changed type to size_t
        Data(char *buf, size_t size, shared_ptr<void> handle) : buffer(buf), byteCapacity(size),
                                                                releaseBuffer(std::move(handle)) {}

        Data(char *buf, size_t size) : buffer(buf), byteCapacity(size) {}
    };

    shared_ptr<Data> data;

    inline void getRaw(unsigned int off, void *buf, unsigned int len);

    inline void putRaw(unsigned int off, void *buf, unsigned int len);

    inline void _copyRaw(unsigned int bytesOff, void *buf, size_t len, bool isWriteTo = false);
};

template<typename Object>
SmartBuffer<Object>::SmartBuffer(Object *buf, unsigned int size, shared_ptr<void> release, bool byteSize) : data(
        new Data{buf, size * (byteSize ? 1 : sizeof(Object)), release}) {}

template<typename Object>
SmartBuffer<Object>::SmartBuffer(unsigned int size) : data(
        new Data{new char[size * sizeof(Object)]{}, size * sizeof(Object)}) {
}

template<typename Object>
SmartBuffer<Object> SmartBuffer<Object>::copy() {
    SmartBuffer<Object> newBuffer(data->buffer, data->byteCapacity, data->releaseBuffer, true);
    newBuffer.data->bytesOffset = data->bytesOffset;
    return newBuffer;
}

template<typename Object>
template<typename Object2>
SmartBuffer<Object2> SmartBuffer<Object>::as(unsigned int off, unsigned int limit) {
    auto bytesOff = off * sizeof(Object);
    if (bytesOff > data->limit * sizeof(Object))throw out_of_range("Offset can not exceed the limit");

    auto realOff = boundedShiftUnchecked(data->bytesOffset, bytesOff, (size_t) 0, data->byteCapacity);

    SmartBuffer<Object2> newBuffer(data->buffer, data->byteCapacity, data->releaseBuffer, true);
    newBuffer.data->bytesOffset = realOff;
    newBuffer.setLimit(limit * sizeof(Object) / sizeof(Object2));
    return newBuffer;
}

template<typename Object>
template<typename Object2>
Object2 SmartBuffer<Object>::get() {
    return get<Object2>(data->position++);
}

template<typename Object>
template<typename Object2>
Object2 SmartBuffer<Object>::get(unsigned int off) {
    if (sizeof(Object2) == 1) {
        if (off >= data->limit)throw out_of_range("Can not read beyond the limit");
        return *reinterpret_cast<Object2 *>(data->buffer +
                                            boundedShiftUnchecked(data->bytesOffset, off * sizeof(Object),
                                                                  (decltype(data->bytesOffset)) 0,
                                                                  data->byteCapacity));
    } else {
        Object2 obj;
        get<Object2>(obj, off);
        return obj;
    }
}

template<typename Object>
template<typename Object2>
SmartBuffer<Object> &SmartBuffer<Object>::get(Object2 &obj) {
    get<Object2>(obj, data->position);
    data->position += (unsigned int) ceil((double) sizeof obj / (double) sizeof(Object));
    return *this;
}

template<typename Object>
template<typename Object2>
SmartBuffer<Object> &SmartBuffer<Object>::get(Object2 &obj, unsigned int off) {
    getRaw(off * sizeof(Object), &obj, sizeof obj);
    return *this;
}

template<typename Object>
template<typename Object2>
SmartBuffer<Object> &SmartBuffer<Object>::get(Object2 *objs, unsigned int len) {
    get<Object2>(objs, len, data->position);
    data->position += (unsigned int) ceil((double) sizeof(Object2) / (double) sizeof(Object) * len);
    return *this;
}

template<typename Object>
template<typename Object2>
SmartBuffer<Object> &SmartBuffer<Object>::get(Object2 *objs, unsigned int len, unsigned int off) {
    getRaw(off * sizeof(Object), objs, len * sizeof(Object2));
    return *this;
}

template<typename Object>
unsigned int SmartBuffer<Object>::manipulate(function<unsigned int(char *, unsigned int)> fun) {
    auto result = manipulate(fun, data->position);
    data->position += result;
    return result;
}

template<typename Object>
unsigned int
SmartBuffer<Object>::manipulate(function<unsigned int(char *, unsigned int)> fun, unsigned int off,
                                unsigned int max) {
    if (off > data->limit)throw out_of_range("Offset can not exceed the limit");
    if (off == data->limit)return 0;
    auto remaining = max = min(max, data->limit - off) * sizeof(Object);
    if (data->byteCapacity - data->bytesOffset > off * sizeof(Object)) {
        auto realOff = data->bytesOffset + off * sizeof(Object);
        auto total = min(max, (unsigned int) (data->byteCapacity - realOff));
        auto amt = fun((data->buffer + realOff), total);
        if (amt < total)return amt;
        remaining -= amt;
    }
    if (remaining) {
        auto amt = fun(data->buffer, remaining);
        remaining -= amt;
    }
    return max - remaining;
}

template<typename Object>
template<typename Object2>
SmartBuffer<Object> &SmartBuffer<Object>::put(Object2 &&obj) {
    put<Object2>(obj, data->position);
    data->position += (unsigned int) ceil((double) sizeof obj / (double) sizeof(Object));
    return *this;
}

template<typename Object>
template<typename Object2>
SmartBuffer<Object> &SmartBuffer<Object>::put(Object2 &obj) {
    put<Object2>(obj, data->position);
    data->position += (unsigned int) ceil((double) sizeof obj / (double) sizeof(Object));
    return *this;
}

template<typename Object>
template<typename Object2>
SmartBuffer<Object> &SmartBuffer<Object>::put(Object2 &&obj, unsigned int off) {
    put<Object2>(obj, off);
    return *this;
}

template<typename Object>
template<typename Object2>
SmartBuffer<Object> &SmartBuffer<Object>::put(Object2 &obj, unsigned int off) {
    putRaw(off * sizeof(Object), &obj, sizeof obj);
    return *this;
}

template<typename Object>
template<typename Object2>
SmartBuffer<Object> &SmartBuffer<Object>::put(Object2 *objs, unsigned int len) {
    put<Object2>(objs, len, data->position);
    data->position += (unsigned int) ceil((double) sizeof(Object2) / (double) sizeof(Object) * len);
    return *this;
}

template<typename Object>
template<typename Object2>
SmartBuffer<Object> &SmartBuffer<Object>::put(Object2 *objs, unsigned int len, unsigned int off) {
    putRaw(off * sizeof(Object), (void *) objs, len * sizeof(Object2));
    return *this;
}

template<typename Object>
unsigned int SmartBuffer<Object>::getLimit() {
    return data->limit;
}

template<typename Object>
SmartBuffer<Object> &SmartBuffer<Object>::setLimit(unsigned int limit) {
    if (limit > getCapacity())throw out_of_range("Limit can not exceed the bytesCapacity");
    data->limit = limit;
    return *this;
}

template<typename Object>
unsigned int SmartBuffer<Object>::getPosition() { return data->position; }

template<typename Object>
SmartBuffer<Object> &SmartBuffer<Object>::setPosition(unsigned int pos) {
    if (pos > data->limit)throw out_of_range("Position can not exceed the limit");
    data->position = pos;
    return *this;
}

template<typename Object>
SmartBuffer<Object> &SmartBuffer<Object>::setMark() {
    data->isMarked = true;
    data->mark = data->position;
    return *this;
}

template<typename Object>
SmartBuffer<Object> &SmartBuffer<Object>::setMark(unsigned int off) {
    if (off > data->position)throw out_of_range("Mark can not exceed the position");
    data->isMarked = true;
    data->mark = off;
    return *this;
}

template<typename Object>
unsigned int SmartBuffer<Object>::getMark() {
    return data->mark;
}

template<typename Object>
bool SmartBuffer<Object>::isMarked() {
    return data->isMarked;
}

template<typename Object>
unsigned int SmartBuffer<Object>::getCapacity() {
    return data->byteCapacity / sizeof(Object);
}

template<typename Object>
unsigned int SmartBuffer<Object>::getRawCapacity() {
    return data->byteCapacity;
}

template<typename Object>
unsigned int SmartBuffer<Object>::available() {
    return data->limit - data->position;
}

template<typename Object>
SmartBuffer<Object> &SmartBuffer<Object>::compact(unsigned int keep) {
    if (keep > data->position)throw out_of_range("Amount to keep can not exceed position");


    data->bytesOffset = boundedShiftUnchecked(data->bytesOffset, (data->position - keep) * sizeof(Object),
                                              (size_t) 0,
                                              data->byteCapacity);
    data->position = keep;
    data->limit = data->byteCapacity / sizeof(Object);
    data->isMarked = false;
    return *this;
}

template<typename Object>
SmartBuffer<Object> &SmartBuffer<Object>::flip() {
    data->limit = data->position;
    data->position = 0;
    data->isMarked = false;
    return *this;

}

template<typename Object>
SmartBuffer<Object> &SmartBuffer<Object>::rewind() {
    data->position = 0;
    data->isMarked = false;
    return *this;
}

template<typename Object>
SmartBuffer<Object> &SmartBuffer<Object>::reset() {
    if (!data->isMarked) throw logic_error("Mark is not set before reset call");
    data->position = data->mark;
    return *this;
}

template<typename Object>
Object *SmartBuffer<Object>::getBuffer() {
    return (Object *) data->buffer;
}

template<typename Object>
unsigned int SmartBuffer<Object>::getBytesOffset() {
    return data->bytesOffset;
}

template<typename Object>
void SmartBuffer<Object>::getRaw(unsigned int off, void *buf, unsigned int len) {
    _copyRaw(off, buf, len, true);
}

template<typename Object>
void SmartBuffer<Object>::putRaw(unsigned int off, void *buf, unsigned int len) {
    _copyRaw(off, buf, len);
}

template<typename Object>
void SmartBuffer<Object>::_copyRaw(unsigned int bytesOff, void *buf, size_t len, bool isWriteTo) {
    auto byteLimit = data->limit * sizeof(Object);
    if (byteLimit < bytesOff || byteLimit - bytesOff < len) throw out_of_range("Can not write beyond the limit");

    if (data->byteCapacity - data->bytesOffset > bytesOff) {
        auto realOff = data->bytesOffset + bytesOff;
        auto amt = min(len, data->byteCapacity - realOff);
        memcpy(isWriteTo ? buf : data->buffer + realOff, isWriteTo ? data->buffer + realOff : buf, amt);
        len -= amt;
        buf = (char *) buf + amt;
    }
    if (len)
        memcpy(isWriteTo ? buf : data->buffer, isWriteTo ? data->buffer : buf, len);
}


#endif //TUNSERVER_SMARTBUFFER_H

#pragma clang diagnostic pop