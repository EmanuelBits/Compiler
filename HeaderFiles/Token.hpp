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

static string tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::ID: return "ID";
        case TokenType::BREAK: return "BREAK";
        case TokenType::CHAR: return "CHAR";
        case TokenType::DOUBLE: return "DOUBLE";
        case TokenType::ELSE: return "ELSE";
        case TokenType::FOR: return "FOR";
        case TokenType::IF: return "IF";
        case TokenType::INT: return "INT";
        case TokenType::RETURN: return "RETURN";
        case TokenType::STRUCT: return "STRUCT";
        case TokenType::VOID: return "VOID";
        case TokenType::WHILE: return "WHILE";
        case TokenType::FLOAT: return "FLOAT";
        case TokenType::CT_INT: return "CT_INT";
        case TokenType::CT_REAL: return "CT_REAL";
        case TokenType::CT_CHAR: return "CT_CHAR";
        case TokenType::CT_STRING: return "CT_STRING";
        case TokenType::COMMA: return "COMMA";
        case TokenType::SEMICOLON: return "SEMICOLON";
        case TokenType::LPAR: return "LPAR";
        case TokenType::RPAR: return "RPAR";
        case TokenType::LBRACKET: return "LBRACKET";
        case TokenType::RBRACKET: return "RBRACKET";
        case TokenType::LACC: return "LACC";
        case TokenType::RACC: return "RACC";
        case TokenType::ADD: return "ADD";
        case TokenType::SUB: return "SUB";
        case TokenType::MUL: return "MUL";
        case TokenType::DIV: return "DIV";
        case TokenType::DOT: return "DOT";
        case TokenType::AND: return "AND";
        case TokenType::OR: return "OR";
        case TokenType::NOT: return "NOT";
        case TokenType::ASSIGN: return "ASSIGN";
        case TokenType::EQUAL: return "EQUAL";
        case TokenType::NOTEQ: return "NOTEQ";
        case TokenType::LESS: return "LESS";
        case TokenType::LESSEQ: return "LESSEQ";
        case TokenType::GREATER: return "GREATER";
        case TokenType::GREATEREQ: return "GREATEREQ";
        case TokenType::UNKNOWN: return "UNKNOWN";
        default: return "UNKNOWN";
    }
}

struct Token {
    TokenType type;
    string lexeme;
    int line;
    int column;

    Token(TokenType type, const string& lexeme, int line, int column)
        : type(type), lexeme(lexeme), line(line), column(column) {}

    string toString() const {
        return "Token(" + lexeme + ", type: " + tokenTypeToString(type) +
               ", line: " + to_string(line) + ", column: " + to_string(column) + ")";
    }
};

#endif