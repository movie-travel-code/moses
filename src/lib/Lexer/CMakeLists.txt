cmake_minimum_required(VERSION 2.8)
ADD_LIBRARY(Lexer STATIC
	PreStoreToken.cpp
	scanner.cpp
	Token.cpp
)
