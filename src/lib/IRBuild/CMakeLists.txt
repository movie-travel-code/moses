cmake_minimum_required(VERSION 3.15)
ADD_LIBRARY(IRBuild STATIC
	CGCall.cpp
	CGDecl.cpp
	CGExpr.cpp
	CGExprAgg.cpp
	CGStmt.cpp
	CodeGenModule.cpp
	CodeGenTypes.cpp
)
