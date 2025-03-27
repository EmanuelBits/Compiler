#ifndef TOKEN_HPP
#define TOKEN_HPP

#include <iostream>
#include <string>

using namespace std;

enum class TokenType {
    ID, BREAK, CHAR, DOUBLE, ELSE, FOR, IF, INT, RETURN, STRUCT, VOID, WHILE, FLOAT,
    CT_INT, CT_REAL, CT_CHAR, CT_STRING,
    COMMA, SEMICOLON, LPAR, RPAR, LBRACKET, RBRACKET, LACC, RACC,
    ADD, SUB, MUL, DIV, DOT, AND, OR, NOT, ASSIGN, EQUAL, NOTEQ, LESS, LESSEQ, GREATER, GREATEREQ,
    UNKNOWN
};

struct Token {
    TokenType type;
    string lexeme;
    int line;
    int column;

    Token(TokenType type, const string& lexeme, int line, int column)
        : type(type), lexeme(lexeme), line(line), column(column) {}

    string toString() const {
        return "Token(" + lexeme + ", type: " + to_string(static_cast<int>(type)) +
               ", line: " + to_string(line) + ", column: " + to_string(column) + ")";
    }
};

#endif