//
// Created by DELL on 6/28/2023.
//
#include "SmartBuffer.h"

void testConstructor() {
    SmartBuffer<char> buf{100};
    char b[100]{};
    SmartBuffer<char> buf2{b, sizeof b};
    cout << "Buffer constructed successfully" << endl;
}

struct Data {
    unsigned int a;
    unsigned int b;
};

void testConsistency() {

    SmartBuffer<Data> _buffer{10000};
    SmartBuffer<Data> buffer = _buffer;

    for (int ind = 0; ind < 10; ind++) {
        _buffer.put((char) (ind + 64));
        char c{33};
        _buffer.put(c);
    }
    buffer.setMark();
    _buffer.put(Data{11, 21});
    buffer.put(Data{12, 22});
    buffer.reset();
    auto d = buffer.get();
    if (d.a != 11 || d.b != 21)throw logic_error("Bytes were corrupted at destructor test");
    buffer.rewind();
    for (int ind = 0; ind < 10; ind++) {
        if (buffer.get<char>() != (char) (ind + 64))throw logic_error("Bytes were corrupted at destructor test");
        if (buffer.get<char>() != (char) 33)throw logic_error("Bytes were corrupted at destructor test");
    }

    printf("Consistency test passed\n");
}

void testDestructor() {
    SmartBuffer<char> buffer{10000};

    for (int ind = 0; ind < 100000; ind++) {
        SmartBuffer<char> buf = buffer;
        SmartBuffer<char> buf2{10000};
        for (int i = 0; i < 10; i++) {
            buf2.put((char) (i + 64));
        }
        buf2.rewind();
        for (int i = 0; i < 10; i++) {
            if (buf2.get() != (char) (i + 64))throw logic_error("Bytes were corrupted at destructor test");
        }
    }
    cout << "Check memory usage of the program please..." << endl;
    this_thread::sleep_for(chrono::seconds(10));
}

void testAllMethods() {
    SmartBuffer<Data> buffer{10000};
    const char *msg = "Hello";
    if ('c' != buffer.put('c').put(msg, strlen(msg)).setMark().reset().rewind().get<char>())
        throw logic_error("Bytes were corrupted at destructor test");
    char buf[10]{};
    buffer.get<char>(buf, strlen(msg));
    if (strcmp(msg, buf))throw logic_error("Bytes were corrupted at destructor test");

    if ('e' != buffer.put('d').put('e').put('f').compact(1).flip().get<char>())
        logic_error("Bytes were corrupted at destructor test");
    buffer.clear().clear();
    if (buffer.position() != 0)logic_error("Bytes were corrupted at destructor test");
    if (buffer.getLimit() != buffer.getCapacity())logic_error("Bytes were corrupted at destructor test");
    if (buffer.getBytesOffset() != 2)logic_error("Bytes were corrupted at destructor test");
    printf("All methods test passed\n");
}

int main() {
    testConstructor();
    testConsistency();
    testAllMethods();
    testDestructor();
}