cmake_minimum_required(VERSION 3.15)

ADD_SUBDIRECTORY(Support)
ADD_LIBRARY(IR STATIC
	BasicBlock.cpp
	ConstantAndGlobal.cpp
	Dominators.cpp
	Function.cpp
	Instruction.cpp
	IRType.cpp
	MosesIRContext.cpp
	Value.cpp
	ValueSymbolTable.cpp
)
