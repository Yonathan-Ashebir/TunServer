//
// Created by DELL on 6/27/2023.
//
#ifndef TUNSERVER_TEST_H
#define TUNSERVER_TEST_H

#include "Include.h"

class InlineVirtualParent {
public:
    inline virtual void fun1();;
};

class InlineVirtualChild : public InlineVirtualParent {
public:
    inline void fun1() override;
};


#endif //TUNSERVER_TEST_H
