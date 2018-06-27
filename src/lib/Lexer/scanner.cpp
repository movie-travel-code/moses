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

Scanner::Scanner(const std::string &srcFileName)
    : FileName(srcFileName), CurLine(1), CurCol(0), CurrentChar(0),
      state(State::NONE) {
  // ���ļ��洢���ݵ�input
  input.open(FileName);
  if (input.fail()) {
    errorReport("When trying to open file " + FileName + ", occurred error.");
    errorFlag = true;
  }
}

void Scanner::getNextChar() {
  CurrentChar = input.get();
  if (CurrentChar == '\n') {
    CurLine++;
    CurCol = 0;
  } else {
    CurCol++;
  }
}

char Scanner::peekChar() {
  char c = input.peek();
  return c;
}

void Scanner::addToBuffer(char c) { buffer.push_back(c); }

void Scanner::reduceBuffer() { buffer.pop_back(); }

// ���ɱ�ʶ��Token
void Scanner::makeToken(TokenValue tv, const TokenLocation &Loc,
                        std::string name) {
  std::cout << name << std::endl;
  LastTok = Tok;
  Tok = Token(tv, Loc, name);
  buffer.clear();
  // ���ɳɹ�һ��token��Ȼ���ٰ�״̬�ÿ�
  state = State::NONE;
}

// ��������Token
void Scanner::makeToken(TokenValue tv, const TokenLocation &loc, long intvalue,
                        std::string name) {
  std::cout << name << std::endl;
  LastTok = Tok;
  Tok = Token(tv, loc, intvalue, name);
  buffer.clear();
  state = State::NONE;
}

// ���ɸ�����Token
void Scanner::makeToken(TokenValue tv, const TokenLocation &loc,
                        double realvalue, std::string name) {
  std::cout << name << std::endl;
  LastTok = Tok;
  Tok = Token(tv, loc, realvalue, name);
  buffer.clear();
  state = State::NONE;
}

void Scanner::preprocess() {
  do {
    while (std::isspace(CurrentChar)) {
      getNextChar();
    }
    handleLineComment();
    // handleBlockComment();
  } while (std::isspace(CurrentChar));
}

void Scanner::handleLineComment() {
  CurLoc = getTokenLocation();

  if (CurrentChar == '#') {
    getNextChar();

    while (CurrentChar != '\n' && CurrentChar != '\r' && CurrentChar != EOF) {
      getNextChar();
    }

    // ����
    if (CurrentChar == '\n') {
      CurLine++;
      CurCol = 0;
      getNextChar();
    } else if (CurrentChar == '\r' && peekChar() == '\n') {
      getNextChar();
      getNextChar();
      CurLine++;
      CurCol = 0;
    } else {
      // �ļ�β
      return;
    }

    if (!input.eof()) {
      // \r
      getNextChar();
    }
  }
}

// void Scanner::handleBlockComment()
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
//				errorReport("end of file hapend in comment, */ is
//expected!, but find " + CurrentChar); 				break;
//			}
//		}

//		if (!input.eof())
//		{
//			// ���ĵ�*
//			getNextChar();
//			// ���ĵ�/
//			getNextChar();
//		}
//	}
//}

// ���ʷ�����
Token Scanner::getNextToken() {
  bool matched = false;
  do {
    if (state != State::NONE) {
      matched = true;
    }

    // �൱��һ������Զ����ֱ�ָ��ͬ�����Զ���
    // ����ÿһ�����Զ�������ʶ��ĳЩģʽ���ַ���
    switch (state) {
      // ���¿�ʼ��һToken�Ĵʷ���������
    case Scanner::State::NONE:
      getNextChar();
      break;
      // �����ļ�β
    case Scanner::State::END_OF_FILE:
      handleEOFState();
      LastTok = Tok;
      Tok = Token();
      return Tok;
      break;
      // ���������ʶ�����Զ���
    case Scanner::State::IDENTIFIER:
      handleIdentifierState();
      break;
      // �����������͵��Զ���
    case Scanner::State::NUMBER:
      handleNumberState();
      break;
      // ���������ַ������Զ���
    case Scanner::State::STRING:
      handleStringState();
      break;
      // ����������������Զ���
    case Scanner::State::OPERATION:
      handleOperationState();
      break;
    default:
      errorReport("Match token state error.");
      errorFlag = true;
      break;
    }

    // ��ʼ״̬ʱ������CurrentChar����ʹ���ĸ��Զ�����ʶ���Ժ��Token
    if (state == State::NONE) {
      preprocess();
      if (input.eof()) {
        state = State::END_OF_FILE;
      } else {
        if (std::isalpha(CurrentChar)) {
          state = State::IDENTIFIER;
        }
        // if it is digit or xdigit
        else if (std::isdigit(CurrentChar) || (CurrentChar == '$')) {
          state = State::NUMBER;
        } else if (CurrentChar == '\"') {
          state = State::STRING;
        } else {
          state = State::OPERATION;
        }
      }
    }
  } while (!matched);
  return Tok;
}

void Scanner::handleEOFState() {
  CurLoc = getTokenLocation();
  makeToken(TokenValue::FILE_EOF, CurLoc, std::string("FILE_EOF"));
  input.close();
}

void Scanner::handleNumberState() {
  CurLoc = getTokenLocation();
  bool matched = false;
  bool isFloat = false;
  int numberBase = 10;

  if (CurrentChar == '$') {
    numberBase = 16;
    getNextChar();
  }
  enum class NumberState { INTEGER, FRACTION, EXPONENT, DONE };

  NumberState numberState = NumberState::INTEGER;

  do {
    switch (numberState) {
    case NumberState::INTEGER:
      if (numberBase == 10) {
        handleDigit();
      } else if (numberBase == 16) {
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

    if (CurrentChar == '.') {
      numberState = NumberState::FRACTION;
    } else if (CurrentChar == 'E' || CurrentChar == 'e') {
      numberState = NumberState::EXPONENT;
    } else {
      numberState = NumberState::DONE;
    }
  } while (numberState != NumberState::DONE);

  if (!errorFlag) {
    if (isFloat) {
      makeToken(TokenValue::REAL_LITERAL, CurLoc, std::stod(buffer), buffer);
    } else {
      makeToken(TokenValue::INTEGER_LITERAL, CurLoc,
                std::stol(buffer, 0, numberBase), buffer);
    }
  } else {
    // just clear buffer and set the state to State::NONE
    buffer.clear();
    state = State::NONE;
  }
}

void Scanner::handleStringState() {
  CurLoc = getTokenLocation();
  // eat ' and NOT update currentChar
  // because we don't want ' (single quote).
  getNextChar();

  while (true) {
    if (CurrentChar == '\"') {
      break;
    }
    addToBuffer(CurrentChar);
    getNextChar();
  }

  // eat end ' and update currentChar
  getNextChar();

  // just one char
  if (buffer.length() == 1) {
    makeToken(TokenValue::CHAR_LITERAL, CurLoc, static_cast<long>(buffer.at(0)),
              buffer);
  } else {
    makeToken(TokenValue::STRING_LITERAL, CurLoc, buffer);
  }
}

// �����ʶ����Ϣ
void Scanner::handleIdentifierState() {
  CurLoc = getTokenLocation();
  // add first char
  addToBuffer(CurrentChar);
  getNextChar();

  while (std::isalnum(CurrentChar) || CurrentChar == '_') {
    addToBuffer(CurrentChar);
    getNextChar();
  }

  // �жϵ�ǰ�Ƿ��ǹؼ��֣��ǹؼ��ֵĻ�����������token
  TokenValue tokenValue = table.isKeyword(buffer);
  if (tokenValue == TokenValue::UNKNOWN) {
    // ���ǹؼ��֣�˵��tokenValue�� identifier
    tokenValue = TokenValue::IDENTIFIER;
  }
  // ����һ�������õ�token
  makeToken(tokenValue, CurLoc, buffer);
}

// ���������
void Scanner::handleOperationState() {
  CurLoc = getTokenLocation();

  bool matched = false;

  addToBuffer(CurrentChar);
  // ���ڴ󲿷����������Ҫ��ǰ��һ��
  addToBuffer(peekChar());

  auto tokenKind = table.isOperator(buffer);
  // �ȼ����ǰ����buffer�Ƿ���ŷ��ű��е����������
  if (tokenKind == TokenValue::UNKNOWN) {
    // ���������ַ��������һ��
    reduceBuffer();
    tokenKind = table.isOperator(buffer);
  } else {
    // ��ǰ��һ���Ĺ����з��ֲ���operator�����³�����Ȼ�������������
    matched = true;
    getNextChar();
  }

  if (tokenKind == TokenValue::UNKNOWN) {
    std::cerr << "Bad Token!" << std::endl;
    exit(1);
  }

  // ʶ��������ɹ���������token
  makeToken(tokenKind, CurLoc, buffer);
  getNextChar();
}

// ��������
void Scanner::handleDigit() {
  addToBuffer(CurrentChar);
  getNextChar();
  // ѭ�����ȥȡ�ַ���ֱ��ȡ���ַ���������Ϊֹ
  while (std::isdigit(CurrentChar)) {
    addToBuffer(CurrentChar);
    getNextChar();
  }
  // ���������п�����.(dot)����E/e��ʽ
  // �������������.(dot)�����Σ�����ת��fraction����
}

void Scanner::handleXDigit() {
  bool readFlag = false;

  while (std::isxdigit(CurrentChar)) {
    readFlag = true;
    addToBuffer(CurrentChar);
    getNextChar();
  }

  if (!readFlag)
    errorReport("Hexadecimal number format error!");
}

void Scanner::handleFraction() {
  if (!std::isdigit(peekChar()))
    errorReport("Fraction number part should be numbers");

  addToBuffer(CurrentChar);
  getNextChar();

  while (std::isdigit(CurrentChar)) {
    addToBuffer(CurrentChar);
    getNextChar();
  }
}

void Scanner::handleExponent() { addToBuffer(CurrentChar); }

void Scanner::errorReport(const std::string &msg) {
  errorToken(getTokenLocation().toString() + msg);
}