//
// Created by DELL on 6/27/2023.
//
#include "Include.h"
#include "Test.h"

void testInlineVirtual2() {
    InlineVirtualChild ch;
    InlineVirtualChild *chPtr = &ch;
    ch.fun1();
}

void InlineVirtualParent::fun1() { cout << "Parent's called" << endl; }

void InlineVirtualChild::fun1() { cout << "Child's called" << endl; }

