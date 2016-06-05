//===-------------------------------scanner.cpp------------------------------===//
//
// This file is used to implements scanner.
//
//===------------------------------------------------------------------------===//
#include "../../include/Lexer/scanner.h"
using namespace compiler::parse;
using namespace compiler::lex;
using namespace compiler::tok;

bool Scanner::errorFlag = false;

Scanner::Scanner(const std::string& srcFileName) :
FileName(srcFileName), CurLine(1), CurCol(0), CurrentChar(0), state(State::NONE)
{
	// 打开文件存储内容到input
	input.open(FileName);
	if (input.fail())
	{
		errorReport("When trying to open file " + FileName + ", occurred error.");
		errorFlag = true;
	}
}

void Scanner::getNextChar()
{
	CurrentChar = input.get();
	if (CurrentChar == '\n')
	{
		CurLine++;
		CurCol = 0;
	}
	else
	{
		CurCol++;
	}
}

char Scanner::peekChar()
{
	char c = input.peek();
	return c;
}

void Scanner::addToBuffer(char c)
{
	buffer.push_back(c);
}

void Scanner::reduceBuffer()
{
	buffer.pop_back();
}

// 生成标识符Token
void Scanner::makeToken(TokenValue tv, const TokenLocation& Loc, std::string name)
{
	std::cout << name << std::endl;
	LastTok = Tok;
	Tok = Token(tv, Loc, name);
	buffer.clear();
	// 生成成功一个token，然后再把状态置空
	state = State::NONE;
}

// 生成整型Token
void Scanner::makeToken(TokenValue tv, const TokenLocation& loc, long intvalue, std::string name)
{
	std::cout << name << std::endl;
	LastTok = Tok;
	Tok = Token(tv, loc, intvalue, name);
	buffer.clear();
	state = State::NONE;
}

// 生成浮点型Token
void Scanner::makeToken(TokenValue tv, const TokenLocation& loc, double realvalue, std::string name)
{
	std::cout << name << std::endl;
	LastTok = Tok;
	Tok = Token(tv, loc, realvalue, name);
	buffer.clear();
	state = State::NONE;
}

void Scanner::preprocess()
{
	do
	{
		while (std::isspace(CurrentChar))
		{
			getNextChar();
		}
		handleLineComment();
		// handleBlockComment();
	} while (std::isspace(CurrentChar));
}

void Scanner::handleLineComment()
{
	CurLoc = getTokenLocation();

	if (CurrentChar == '#')
	{
		getNextChar();

		while (CurrentChar != '\n' && CurrentChar != '\r' && CurrentChar != EOF)
		{
			getNextChar();
		}

		// 换行
		if (CurrentChar == '\n')
		{
			CurLine++;
			CurCol = 0;
			getNextChar();
		}
		else if (CurrentChar == '\r' && peekChar() == '\n')
		{
			getNextChar();
			getNextChar();
			CurLine++;
			CurCol = 0;
		}
		else
		{
			// 文件尾
			return;
		}

		if (!input.eof())
		{
			// \r
			getNextChar();
		}
	}
}

//void Scanner::handleBlockComment()
//{
//	CurLoc = getTokenLocation();
//	if (CurrentChar == '/' && peekChar() == '*')
//	{
//		getNextChar();
//		getNextChar();

//		while (!(CurrentChar == '*' && peekChar() == '/'))
//		{
//			// skip comment content
//			getNextChar();

//			if (input.eof())
//			{
//				errorReport("end of file hapend in comment, */ is expected!, but find " + CurrentChar);
//				break;
//			}
//		}

//		if (!input.eof())
//		{
//			// 消耗掉*
//			getNextChar();
//			// 消耗掉/
//			getNextChar();
//		}
//	}
//}

// 将词法分析
Token Scanner::getNextToken()
{
	bool matched = false;
	do
	{
		if (state != State::NONE)
		{
			matched = true;
		}

		// 相当于一个大的自动机分别指向不同的子自动机
		// 其中每一个子自动机用于识别某些模式的字符串
		switch (state)
		{
			// 重新开始下一Token的词法分析过程
		case Scanner::State::NONE:
			getNextChar();
			break;
			// 遇到文件尾
		case Scanner::State::END_OF_FILE:
			handleEOFState();
			LastTok = Tok;
			Tok = Token();
			return Tok;
			break;
			// 单独处理标识符的自动机
		case Scanner::State::IDENTIFIER:
			handleIdentifierState();
			break;
			// 单独处理整型的自动机
		case Scanner::State::NUMBER:
			handleNumberState();
			break;
			// 单独处理字符串的自动机
		case Scanner::State::STRING:
			handleStringState();
			break;
			// 单独处理运算符的自动机
		case Scanner::State::OPERATION:
			handleOperationState();
			break;
		default:
			errorReport("Match token state error.");
			errorFlag = true;
			break;
		}

		// 初始状态时，根据CurrentChar决定使用哪个自动机来识别以后的Token
		if (state == State::NONE)
		{
			preprocess();
			if (input.eof())
			{
				state = State::END_OF_FILE;
			}
			else
			{
				if (std::isalpha(CurrentChar))
				{
					state = State::IDENTIFIER;
				}
				// if it is digit or xdigit
				else if (std::isdigit(CurrentChar) || (CurrentChar == '$'))
				{
					state = State::NUMBER;
				}
				else if (CurrentChar == '\"')
				{
					state = State::STRING;
				}
				else
				{
					state = State::OPERATION;
				}
			}
		}
	} while (!matched);
	return Tok;
}

void Scanner::handleEOFState()
{
	CurLoc = getTokenLocation();
	makeToken(TokenValue::FILE_EOF, CurLoc, std::string("FILE_EOF"));
	input.close();
}

void Scanner::handleNumberState()
{
	CurLoc = getTokenLocation();
	bool matched = false;
	bool isFloat = false;
	int numberBase = 10;

	if (CurrentChar == '$')
	{
		numberBase = 16;
		getNextChar();
	}
	enum class NumberState
	{
		INTEGER,
		FRACTION,
		EXPONENT,
		DONE
	};

	NumberState numberState = NumberState::INTEGER;

	do
	{
		switch (numberState)
		{
		case NumberState::INTEGER:
			if (numberBase == 10)
			{
				handleDigit();
			}
			else if (numberBase == 16)
			{
				handleXDigit();
			}
			break;
		case NumberState::FRACTION:
			handleFraction();
			isFloat = true;
			break;
		case NumberState::EXPONENT:
			// handleExponent();
			isFloat = true;
			break;
		case NumberState::DONE:
			break;
		default:
			errorReport("Match number state error.");
			errorFlag = true;
			break;
		}

		if (CurrentChar == '.')
		{
			numberState = NumberState::FRACTION;
		}
		else if (CurrentChar == 'E' || CurrentChar == 'e')
		{
			numberState = NumberState::EXPONENT;
		}
		else
		{
			numberState = NumberState::DONE;
		}
	} while (numberState != NumberState::DONE);

	if (!errorFlag)
	{
		if (isFloat)
		{
			makeToken(TokenValue::REAL_LITERAL, CurLoc, std::stod(buffer), buffer);
		}
		else
		{
			makeToken(TokenValue::INTEGER_LITERAL, CurLoc, std::stol(buffer, 0, numberBase), buffer);
		}
	}
	else
	{
		// just clear buffer and set the state to State::NONE
		buffer.clear();
		state = State::NONE;
	}
}

void Scanner::handleStringState()
{
	CurLoc = getTokenLocation();
	// eat ' and NOT update currentChar
	// because we don't want ' (single quote).
	getNextChar();

	while (true)
	{
		if (CurrentChar == '\"')
		{
			break;
		}
		addToBuffer(CurrentChar);
		getNextChar();
	}

	// eat end ' and update currentChar
	getNextChar();

	// just one char
	if (buffer.length() == 1)
	{
		makeToken(TokenValue::CHAR_LITERAL, CurLoc, static_cast<long>(buffer.at(0)), buffer);
	}
	else
	{
		makeToken(TokenValue::STRING_LITERAL, CurLoc, buffer);
	}
}

// 处理标识符信息
void Scanner::handleIdentifierState()
{
	CurLoc = getTokenLocation();
	// add first char
	addToBuffer(CurrentChar);
	getNextChar();

	while (std::isalnum(CurrentChar) || CurrentChar == '_')
	{
		addToBuffer(CurrentChar);
		getNextChar();
	}

	// 判断当前是否是关键字，是关键字的话，则设置其token
	TokenValue tokenValue = table.isKeyword(buffer);
	if (tokenValue == TokenValue::UNKNOWN)
	{
		// 不是关键字，说明tokenValue是 identifier
		tokenValue = TokenValue::IDENTIFIER;
	}
	// 返回一个创建好的token
	makeToken(tokenValue, CurLoc, buffer);
}

// 处理操作符
void Scanner::handleOperationState()
{
	CurLoc = getTokenLocation();

	bool matched = false;

	addToBuffer(CurrentChar);
	// 由于大部分运算符都需要向前看一步
	addToBuffer(peekChar());

	auto tokenKind = table.isOperator(buffer);
	// 先检查向前看的buffer是否符号符号表中的任意运算符
	if (tokenKind == TokenValue::UNKNOWN)
	{
		// 不是任意字符，则后退一步
		reduceBuffer();
		tokenKind = table.isOperator(buffer);
	}
	else
	{
		// 向前看一步的过程中发现不是operator，则吐出来，然后继续向下运行
		matched = true;
		getNextChar();
	}

	if (tokenKind == TokenValue::UNKNOWN)
	{
		std::cerr << "Bad Token!" << std::endl;
		exit(1);
	}

	// 识别运算符成功，创建该token
	makeToken(tokenKind, CurLoc, buffer);
	getNextChar();
}

// 处理数字
void Scanner::handleDigit()
{
	addToBuffer(CurrentChar);
	getNextChar();
	// 循环向后去取字符，直到取得字符不是数字为止
	while (std::isdigit(CurrentChar))
	{
		addToBuffer(CurrentChar);
		getNextChar();
	}
	// 由于数字有可能有.(dot)或者E/e形式
	// 如果含有上述的.(dot)的情形，则跳转到fraction处理
}

void Scanner::handleXDigit()
{
	bool readFlag = false;

	while (std::isxdigit(CurrentChar))
	{
		readFlag = true;
		addToBuffer(CurrentChar);
		getNextChar();
	}

	if (!readFlag)
		errorReport("Hexadecimal number format error!");
}

void Scanner::handleFraction()
{
	if (!std::isdigit(peekChar()))
		errorReport("Fraction number part should be numbers");

	addToBuffer(CurrentChar);
	getNextChar();

	while (std::isdigit(CurrentChar))
	{
		addToBuffer(CurrentChar);
		getNextChar();
	}
}

void Scanner::handleExponent()
{
	addToBuffer(CurrentChar);
}

void Scanner::errorReport(const std::string& msg)
{
	errorToken(getTokenLocation().toString() + msg);
}