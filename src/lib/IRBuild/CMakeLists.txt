cmake_minimum_required(VERSION 2.8)
ADD_LIBRARY(IRBuild STATIC
	CGCall.cpp
	CGDecl.cpp
	CGExpr.cpp
	CGExprAgg.cpp
	CGStmt.cpp
	CodeGenModule.cpp
	CodeGenTypes.cpp
)
