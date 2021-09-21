//===---------------------------------error.h-----------------------------===//
//
// This file for error report.
//
//===--------------------------------------------------------------------===//
#pragma once
#include <string>
extern void errorOption(const std::string &msg);
extern void errorToken(const std::string &msg);
extern void errorParser(const std::string &msg);
extern void errorSema(const std::string &sema);
