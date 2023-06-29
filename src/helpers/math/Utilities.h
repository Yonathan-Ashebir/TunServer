//
// Created by DELL on 6/29/2023.
//

#ifndef TUNSERVER_MATH_UTILITIES_H
#define TUNSERVER_MATH_UTILITIES_H

#include "../Include.h"

template<typename Val, typename Diff>
constexpr Val boundedShiftUnchecked(Val i, Diff by, Val lower, Val upper) {
    if (by >= 0) {
        if (upper - i > by) return i + by;
        else return by - (upper - i) + lower;
    } else {
        if (i - lower > -by) return i + by;
        else return by + (i - lower) + upper;
    }
}

template<typename Val, typename Diff>
constexpr Val boundedShift(Val i, Diff by, Val lower, Val upper) {
    if (i < lower || i >= upper)throw out_of_range("'i' is already out of bounds");
    if (lower > upper)throw out_of_range("Upper bound is less than lower bound");
    return boundedShiftUnchecked(i, by, lower, upper);
}


#endif //TUNSERVER_MATH_UTILITIES_H
