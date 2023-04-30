//
// Created by yoni_ash on 4/28/23.
//

#ifndef TUNSERVER_ERROR_H
#define TUNSERVER_ERROR_H

#pragma once

#include <cerrno>

char const *getErrorName(int errno_);

#endif //TUNSERVER_ERROR_H
