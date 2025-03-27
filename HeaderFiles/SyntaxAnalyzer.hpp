#ifndef SYNTAXANALYZER_HPP
#define SYNTAXANALYZER_HPP

#include <iostream>
#include "Token.hpp"
#include "ErrorHandler.hpp"
#include "LexicalRules.hpp"

using namespace std;

class SyntaxAnalyzer {
private:
    vector<Token> tokens;
    size_t currentTokenIndex;
    Token* consumedTk;

public:
    SyntaxAnalyzer(vector<Token> tokenStream) : tokens(tokenStream), currentTokenIndex(0) {}

    Token* crtTk() {
        return (currentTokenIndex < tokens.size()) ? &tokens[currentTokenIndex] : nullptr;
    }

    bool consume(TokenType type) {
        if (crtTk() && crtTk()->type == type) {
            consumedTk = crtTk();
            currentTokenIndex++;
            return true;
        }
        return false;
    }

    void tkerr(const string& message) {
        if (crtTk()) {
            ErrorHandler::printSyntaxError(message, crtTk()->line, crtTk()->column);
        } else {
            ErrorHandler::printSyntaxError("Syntax error at end of file: " + message, -1, -1);
        }
    }

    bool unit() {
        while (declStruct() || declFunc() || declVar());
        return true;
    }

    bool declStruct() {
        if (!consume(TokenType::STRUCT)) return false;
        if (!consume(TokenType::ID)) tkerr("Expected struct name after STRUCT.");
        if (!consume(TokenType::LACC)) tkerr("Expected { after struct name.");
        while (declVar());
        if (!consume(TokenType::RACC)) tkerr("Expected } at the end of struct.");
        if (!consume(TokenType::SEMICOLON)) tkerr("Expected ; after struct declaration.");
        return true;
    }

    bool declVar() {
        if (!typeBase()) return false;
        if (!consume(TokenType::ID)) tkerr("Expected variable name after type.");
        arrayDecl();
        while (consume(TokenType::COMMA)) {
            if (!consume(TokenType::ID)) tkerr("Expected variable name after ,.");
            arrayDecl();
        }
        if (!consume(TokenType::SEMICOLON)) tkerr("Expected ; at the end of variable declaration.");
        return true;
    }

    bool typeBase() {
        return consume(TokenType::INT) || consume(TokenType::DOUBLE) || 
               consume(TokenType::CHAR) || (consume(TokenType::STRUCT) && consume(TokenType::ID));
    }

    bool arrayDecl() {
        if (!consume(TokenType::LBRACKET)) return false;
        expr();
        if (!consume(TokenType::RBRACKET)) tkerr("Expected ] after array declaration.");
        return true;
    }

    bool typeName() {
        if (!typeBase()) return false;
        arrayDecl();
        return true;
    }

    bool declFunc() {
        if (!(typeBase() || consume(TokenType::VOID))) return false;
        consume(TokenType::MUL); // optional pointer
        if (!consume(TokenType::ID)) tkerr("Expected function name.");
        if (!consume(TokenType::LPAR)) tkerr("Expected ( after function name.");
        if (funcArg()) {
            while (consume(TokenType::COMMA)) {
                if (!funcArg()) tkerr("Expected function argument after ,.");
            }
        }
        if (!consume(TokenType::RPAR)) tkerr("Expected ) after function parameters.");
        if (!stmCompound()) tkerr("Expected function body after function declaration.");
        return true;
    }

    bool funcArg() {
        if (!typeBase()) return false;
        if (!consume(TokenType::ID)) tkerr("Expected argument name.");
        arrayDecl();
        return true;
    }

    bool stm() {
        if (stmCompound()) return true;
        if (consume(TokenType::IF)) {
            if (!consume(TokenType::LPAR)) tkerr("Expected ( after IF.");
            if (!expr()) tkerr("Expected expression inside IF condition.");
            if (!consume(TokenType::RPAR)) tkerr("Expected ) after IF condition.");
            if (!stm()) tkerr("Expected statement after IF.");
            if (consume(TokenType::ELSE)) {
                if (!stm()) tkerr("Expected statement after ELSE.");
            }
            return true;
        }
        if (consume(TokenType::WHILE)) {
            if (!consume(TokenType::LPAR)) tkerr("Expected ( after WHILE.");
            if (!expr()) tkerr("Expected expression inside WHILE condition.");
            if (!consume(TokenType::RPAR)) tkerr("Expected ) after WHILE condition.");
            if (!stm()) tkerr("Expected statement after WHILE.");
            return true;
        }
        if (consume(TokenType::FOR)) {
            if (!consume(TokenType::LPAR)) tkerr("Expected ( after FOR.");
            expr();
            if (!consume(TokenType::SEMICOLON)) tkerr("Expected ; inside FOR loop.");
            expr();
            if (!consume(TokenType::SEMICOLON)) tkerr("Expected ; inside FOR loop.");
            expr();
            if (!consume(TokenType::RPAR)) tkerr("Expected ) after FOR loop.");
            if (!stm()) tkerr("Expected statement after FOR.");
            return true;
        }
        if (consume(TokenType::BREAK)) {
            if (!consume(TokenType::SEMICOLON)) {
                tkerr("Expected ; after BREAK.");
                advanceToNextValidToken();
            }
            return true;
        }
        if (consume(TokenType::RETURN)) {
            expr();
            if (!consume(TokenType::SEMICOLON)) {
                tkerr("Expected ; after RETURN.");
                advanceToNextValidToken();
            }
            return true;
        }
        if (expr()) {
            if (!consume(TokenType::SEMICOLON)) {
                tkerr("Expected ; after expression.");
                advanceToNextValidToken();
            }
            return true;
        }
        return false;
    }

    void advanceToNextValidToken() {
        while (crtTk() && crtTk()->type != TokenType::SEMICOLON && crtTk()->type != TokenType::RACC) {
            currentTokenIndex++;  // Skip tokens to prevent infinite looping
        }
        if (crtTk() && crtTk()->type == TokenType::SEMICOLON) {
            currentTokenIndex++;  // Skip the semicolon if found
        }
    }

    bool stmCompound() {
        if (!consume(TokenType::LACC)) return false;
        while (declVar() || stm());
        if (!consume(TokenType::RACC)) tkerr("Expected } at the end of compound statement.");
        return true;
    }

    bool expr() {
        return exprAssign();
    }

    bool exprAssign() {
        if (exprUnary()) {
            if (consume(TokenType::ASSIGN)) {
                if (!exprAssign()) tkerr("Invalid assignment expression.");
                return true;
            }
        }
        return exprOr();
    }

    bool exprOr() {
        if (!exprAnd()) return false;
        while (consume(TokenType::OR)) {
            if (!exprAnd()) tkerr("Invalid OR expression.");
        }
        return true;
    }

    bool exprAnd() {
        if (!exprEq()) return false;
        while (consume(TokenType::AND)) {
            if (!exprEq()) tkerr("Invalid AND expression.");
        }
        return true;
    }

    bool exprEq() {
        if (!exprRel()) return false;
        while (consume(TokenType::EQUAL) || consume(TokenType::NOTEQ)) {
            if (!exprRel()) tkerr("Invalid equality expression.");
        }
        return true;
    }

    bool exprRel() {
        if (!exprAdd()) return false;
        while (consume(TokenType::LESS) || consume(TokenType::LESSEQ) ||
               consume(TokenType::GREATER) || consume(TokenType::GREATEREQ)) {
            if (!exprAdd()) tkerr("Invalid relational expression.");
        }
        return true;
    }

    bool exprAdd() {
        if (!exprMul()) return false;
        while (consume(TokenType::ADD) || consume(TokenType::SUB)) {
            if (!exprMul()) tkerr("Invalid addition or subtraction expression.");
        }
        return true;
    }

    bool exprMul() {
        if (!exprCast()) return false;
        while (consume(TokenType::MUL) || consume(TokenType::DIV)) {
            if (!exprCast()) tkerr("Invalid multiplication or division expression.");
        }
        return true;
    }

    bool exprCast() {
        if (consume(TokenType::LPAR)) {
            if (typeName()) {  // Check if it's a type name
                if (!consume(TokenType::RPAR)) tkerr("Expected ) after type cast.");
                return exprCast();
            } else {
                currentTokenIndex--;  // "Undo" the token consumption
                return exprUnary();
            }
        }
        return exprUnary();
    }

    bool exprUnary() {
        if (consume(TokenType::SUB) || consume(TokenType::NOT)) return exprUnary();
        return exprPostfix();
    }

    bool exprPostfix() {
        if (!exprPrimary()) return false;
        while (consume(TokenType::LBRACKET) || consume(TokenType::DOT)) {
            if (!consume(TokenType::ID)) tkerr("Expected identifier after . or [].");
        }
        return true;
    }

    bool exprPrimary() {
        return consume(TokenType::ID) || consume(TokenType::CT_INT) || consume(TokenType::CT_REAL) ||
               consume(TokenType::CT_CHAR) || consume(TokenType::CT_STRING) ||
               (consume(TokenType::LPAR) && expr() && consume(TokenType::RPAR));
    }
};

#endif