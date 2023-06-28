//
// Created by DELL on 6/28/2023.
//
#include "SmartBuffer.h"

void test() {
    SmartBuffer buf{100};
    char *ch = new char[10];
    buf.put(ch, sizeof ch);
};

int main() {

}