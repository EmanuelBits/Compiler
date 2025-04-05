#ifndef LEXICALRULES_HPP
#define LEXICALRULES_HPP

#include <iostream>
#include <vector>
#include "FileHandler.hpp"
#include "Token.hpp"
#include "ErrorHandler.hpp"

using namespace std;

enum class State {
    InitialState,           // Start state
    IdentifierState,        // ID and keyword recognition
    NumberState,            // Integer or floating-point number
    HexNumberState,         // Hexadecimal numbers (0x123ABC)
    OctalNumberState,       // Octal numbers (0777)
    RealNumberState,        // Floating point number
    ExponentState,          // For numbers like 10e7
    ExponentNumberState,    // Reading the numbers after 'e'
    StringState,            // Strings
    CharState,              // Character literals
    EscapeSequenceState,    // When you find a special character
    CommentState,           // Line comments
    BlockCommentState,      // Block comments
    EndState                // Final state
};

class LexicalAnalyzer {
private:
    FileHandler& fileHandler;
    vector<Token> tokens;
    int line, column;
    
    bool inCharLiteral = false;

public:
    LexicalAnalyzer(FileHandler& fileHandler) : fileHandler(fileHandler) {
        line = fileHandler.getLineNumber();
        column = fileHandler.getColumnNumber();
    }

    vector<Token> analyze() {
        char ch;
        string buffer;
        State state = State::InitialState;
        TokenType currentToken;
        
        while ((ch = fileHandler.getNextChar()) != EOF) {
            column = fileHandler.getColumnNumber();
            line = fileHandler.getLineNumber();

            switch (state) {
                case State::InitialState:
                    if (isspace(ch)) {
                        continue;  // Ignore spaces, line tracking handled in FileHandler
                    } else if (isalpha(ch) || ch == '_') {  // Identifier or Keyword
                        buffer += ch;
                        state = State::IdentifierState;
                    } else if (isdigit(ch)) {  // Numbers
                        buffer += ch;
                        
                        if (ch == '0') {  // Check if it's octal, hex, or floating-point
                            char nextChar = fileHandler.getNextChar();
                            if (nextChar == 'x' || nextChar == 'X') {
                                buffer += nextChar;  // Append 'x'
                                state = State::HexNumberState;
                            } else if (nextChar >= '0' && nextChar <= '7') {
                                fileHandler.putBackChar(nextChar);  // Put back in case of octal
                                state = State::OctalNumberState;
                            } else if (nextChar == '.') {
                                buffer += nextChar;  // Start floating-point number
                                state = State::RealNumberState;
                            } else {  // It's just "0"
                                fileHandler.putBackChar(nextChar);
                                tokens.emplace_back(TokenType::CT_INT, buffer, line, column);
                                buffer.clear();
                                state = State::InitialState;
                            }
                        } else {
                            state = State::NumberState;  // Normal decimal number
                        }
                    } else if (ch == '\"') {  // String literal
                        buffer.clear();
                        state = State::StringState;
                    } else if (ch == '\'') {  // Char literal
                        buffer.clear();
                        state = State::CharState;
                    } else if (ch == '/') {  // Start of comment or division
                        char nextChar = fileHandler.getNextChar();
                        if (nextChar == '/') {
                            state = State::CommentState;  // Line comment
                        } else if (nextChar == '*') {
                            state = State::BlockCommentState;  // Block comment
                        } else {
                            tokens.emplace_back(TokenType::DIV, "/", line, column);
                        }
                        fileHandler.putBackChar(nextChar);
                    } else if (ch == '.') {
                        tokens.emplace_back(TokenType::DOT, ".", line, column);
                    } else if (ch == ',') tokens.emplace_back(TokenType::COMMA, ",", line, column);
                    else if (ch == ';') tokens.emplace_back(TokenType::SEMICOLON, ";", line, column);
                    else if (ch == '(') tokens.emplace_back(TokenType::LPAR, "(", line, column);
                    else if (ch == ')') tokens.emplace_back(TokenType::RPAR, ")", line, column);
                    else if (ch == '[') tokens.emplace_back(TokenType::LBRACKET, "[", line, column);
                    else if (ch == ']') tokens.emplace_back(TokenType::RBRACKET, "]", line, column);
                    else if (ch == '{') tokens.emplace_back(TokenType::LACC, "{", line, column);
                    else if (ch == '}') tokens.emplace_back(TokenType::RACC, "}", line, column);
                    else if (ch == '+') tokens.emplace_back(TokenType::ADD, "+", line, column);
                    else if (ch == '-') tokens.emplace_back(TokenType::SUB, "-", line, column);
                    else if (ch == '*') tokens.emplace_back(TokenType::MUL, "*", line, column);
                    else if (ch == '!') {  // Check for !=
                        char nextChar = fileHandler.getNextChar();
                        if (nextChar == '=') tokens.emplace_back(TokenType::NOTEQ, "!=", line, column);
                        else tokens.emplace_back(TokenType::NOT, "!", line, column);
                    } else if (ch == '=') {  // Check for ==
                        char nextChar = fileHandler.getNextChar();
                        if (nextChar == '=') tokens.emplace_back(TokenType::EQUAL, "==", line, column);
                        else {
                            fileHandler.putBackChar(nextChar);  // Put back character if not part of token
                            tokens.emplace_back(TokenType::ASSIGN, "=", line, column);
                        }
                    } else if (ch == '<') {
                        char nextChar = fileHandler.getNextChar();
                        if (nextChar == '=') tokens.emplace_back(TokenType::LESSEQ, "<=", line, column);
                        else {
                            fileHandler.putBackChar(nextChar);  // Put back character if not needed
                            tokens.emplace_back(TokenType::LESS, "<", line, column);
                        }
                    } else if (ch == '>') {  // Check for >=
                        char nextChar = fileHandler.getNextChar();
                        if (nextChar == '=') tokens.emplace_back(TokenType::GREATEREQ, ">=", line, column);
                        else tokens.emplace_back(TokenType::GREATER, ">", line, column);
                    } else if (ch == '&') {
                        char nextChar = fileHandler.getNextChar();
                        if (nextChar == '&') tokens.emplace_back(TokenType::AND, "&&", line, column);
                        else {
                            fileHandler.putBackChar(nextChar);  // Put back if it's a single '&'
                            tokens.emplace_back(TokenType::UNKNOWN, "&", line, column);  // Mark as unknown token
                        }
                    } else if (ch == '|') {  // Check for ||
                        char nextChar = fileHandler.getNextChar();
                        if (nextChar == '|') tokens.emplace_back(TokenType::OR, "||", line, column);
                        else ErrorHandler::printLexicalError("Invalid '|' operator", line, column);
                    } else {
                        ErrorHandler::printLexicalError("Unknown character: " + string(1, ch), line, column);
                    }
                    break;

                case State::IdentifierState:
                    if (isalnum(ch) || ch == '_') {
                        buffer += ch;
                    } else {
                        // fileHandler.getNextChar();  // Lookahead correction
                        currentToken = checkKeyword(buffer);
                        tokens.emplace_back(currentToken, buffer, line, column);
                        buffer.clear();
                        fileHandler.putBackChar(ch);
                        state = State::InitialState;
                    }
                    break;

                case State::NumberState:
                    if (isdigit(ch)) {
                        buffer += ch;
                    } else if (ch == '.') {
                        buffer += ch;
                        state = State::RealNumberState;
                    } else if (ch == 'e' || ch == 'E') {
                        buffer += ch;
                        state = State::ExponentState;
                    } else {
                        tokens.emplace_back(TokenType::CT_INT, buffer, line, column);
                        buffer.clear();
                        fileHandler.putBackChar(ch);
                        state = State::InitialState;
                    }
                    break;                

                case State::RealNumberState:
                    if (isdigit(ch)) {
                        buffer += ch;  // Continue reading decimal digits
                    } else if (ch == 'e' || ch == 'E') {
                        buffer += ch;  // Start exponent notation
                        state = State::ExponentState;
                    } else {
                        fileHandler.putBackChar(ch);
                        tokens.emplace_back(TokenType::CT_REAL, buffer, line, column);
                        buffer.clear();
                        state = State::InitialState;
                    }
                    break;

                case State::ExponentState:
                    if (ch == '+' || ch == '-') {  
                        buffer += ch;  // Handle optional sign
                        char nextChar = fileHandler.getNextChar();
                        if (isdigit(nextChar)) {  
                            buffer += nextChar;
                            state = State::ExponentNumberState;
                        } else {  
                            ErrorHandler::printLexicalError("Invalid exponent notation: missing digits after sign", line, column);
                            fileHandler.putBackChar(nextChar);  // Put back the invalid character
                            tokens.emplace_back(TokenType::CT_REAL, buffer, line, column);
                            buffer.clear();
                            state = State::InitialState;
                        }
                    } else if (isdigit(ch)) {  
                        buffer += ch;  // Directly enter exponent number state
                        state = State::ExponentNumberState;
                    } else {
                        ErrorHandler::printLexicalError("Invalid exponent notation in number", line, column);
                        fileHandler.putBackChar(ch);  // Put back the invalid character
                        tokens.emplace_back(TokenType::CT_REAL, buffer, line, column);
                        buffer.clear();
                        state = State::InitialState;
                    }
                    break;

                case State::ExponentNumberState:
                    if (isdigit(ch)) {
                        buffer += ch;  // Continue reading exponent part
                    } else {
                        fileHandler.putBackChar(ch);
                        tokens.emplace_back(TokenType::CT_REAL, buffer, line, column);
                        buffer.clear();
                        state = State::InitialState;
                    }
                    break;

                case State::OctalNumberState:
                    if (ch >= '0' && ch <= '7') {
                        buffer += ch;
                    } else if (ch == '8' || ch == '9') {
                        ErrorHandler::printLexicalError("Invalid octal number", line, column);
                        state = State::InitialState;
                    } else {
                        fileHandler.putBackChar(ch);
                        tokens.emplace_back(TokenType::CT_INT, buffer, line, column);
                        buffer.clear();
                        state = State::InitialState;
                    }
                    break;                

                case State::HexNumberState:
                    if (isxdigit(ch)) {
                        buffer += ch;
                    } else {
                        fileHandler.putBackChar(ch);
                        tokens.emplace_back(TokenType::CT_INT, buffer, line, column);
                        buffer.clear();
                        state = State::InitialState;
                    }
                    break;

                case State::StringState:
                    inCharLiteral = false;
                    if (ch == '\\') {  
                        state = State::EscapeSequenceState;  // Go to escape processing
                    } else if (ch == '"') {
                        // End of string literal
                        tokens.emplace_back(TokenType::CT_STRING, buffer, line, column);
                        buffer.clear();
                        state = State::InitialState;
                    } else {
                        buffer += ch;
                    }
                    break;

                case State::CharState:
                    inCharLiteral = true;
                    if (ch == '\\') {  
                        state = State::EscapeSequenceState;  // Go to escape processing
                    } else if (ch == '\'') {
                        // End of char literal
                        tokens.emplace_back(TokenType::CT_CHAR, buffer, line, column);
                        buffer.clear();
                        inCharLiteral = false;
                        state = State::InitialState;
                    } else {
                        buffer += ch;
                    }
                    break;
                
                case State::EscapeSequenceState:
                    if (ch == 'a' || ch == 'b' || ch == 'f' || ch == 'n' || ch == 'r' || 
                        ch == 't' || ch == 'v' || ch == '\\' || ch == '\'' || ch == '\"' || ch == '0') {
                        buffer += ch;  // Append only the escaped character
                        // Return to the appropriate literal state based on context:
                        if (inCharLiteral) {
                            state = State::CharState;
                        } else {
                            state = State::StringState;
                        }
                    } else {
                        ErrorHandler::printLexicalError("Invalid escape sequence", line, column);
                        // Recover by returning to the appropriate state:
                        if (inCharLiteral) {
                            state = State::CharState;
                        } else {
                            state = State::StringState;
                        }
                    }
                    break;

                case State::CommentState:
                    if (ch == '\n') {
                        state = State::InitialState;
                    }
                    break;

                case State::BlockCommentState:
                    if (ch == '*' && fileHandler.getNextChar() == '/') {
                        state = State::InitialState;  // End block comment
                    } else if (ch == EOF) {
                        ErrorHandler::printLexicalError("Unterminated block comment", line, column);
                        state = State::InitialState;
                    }
                    break;                

                case State::EndState:
                    return tokens;
            }
        }

        return tokens;
    }

    TokenType checkKeyword(const string& buffer) {
        if (buffer == "if") return TokenType::IF;
        else if (buffer == "else") return TokenType::ELSE;
        else if (buffer == "char") return TokenType::CHAR;
        else if (buffer == "int") return TokenType::INT;
        else if (buffer == "float") return TokenType::FLOAT;
        else if (buffer == "double") return TokenType::DOUBLE;
        else if (buffer == "break") return TokenType::BREAK;
        else if (buffer == "for") return TokenType::FOR;
        else if (buffer == "return") return TokenType::RETURN;
        else if (buffer == "struct") return TokenType::STRUCT;
        else if (buffer == "void") return TokenType::VOID;
        else if (buffer == "while") return TokenType::WHILE;
        return TokenType::ID;
    }
};

#endif