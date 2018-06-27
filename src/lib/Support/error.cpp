//===---------------------------------------error.cpp-----------------------------------===//
//
// This file impletments error mechanism.
//
//===-----------------------------------------------------------------------------------===//
#include "../../include/Support/error.h"
#include <iostream>


void errorToken(const std::string &msg) {
  std::cerr << "Token Error: " << msg << std::endl;
}

void errorParser(const std::string &msg) {
  std::cerr << "parser error: " << msg << std::endl;
}

void errorSema(const std::string &msg) {
  std::cerr << "sema error: " << msg << std::endl;
}