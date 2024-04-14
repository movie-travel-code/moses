cmake_minimum_required(VERSION 3.15)
ADD_LIBRARY(Parser STATIC
	ast.cpp
	constant-evaluator.cpp
	EvaluatedExprVisitor.cpp
	parser.cpp
	sema.cpp
	Type.cpp
)
