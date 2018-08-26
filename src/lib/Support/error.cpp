//===---------------------------------------error.cpp-----------------------------------===//
//
// This file impletments error mechanism.
//
//===-----------------------------------------------------------------------------------===//
#include "include/Support/error.h"
#include <iostream>

void errorOption(const std::string &msg) {
  std::cerr << "Option Error: " << " ----- " << msg << std::endl;
}

void errorToken(const std::string &msg) {
  std::cerr << "Token Error: " << msg << std::endl;
}

void errorParser(const std::string &msg) {
  std::cerr << "parser error: " << msg << std::endl;
}

void errorSema(const std::string &msg) {
  std::cerr << "sema error: " << msg << std::endl;
}