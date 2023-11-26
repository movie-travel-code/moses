cmake_minimum_required(VERSION 3.15)
ADD_LIBRARY(Lexer STATIC
	PreStoreToken.cpp
	scanner.cpp
	Token.cpp
)
