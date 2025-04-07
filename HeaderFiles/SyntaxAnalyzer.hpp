#ifndef SYNTAXANALYZER_HPP
#define SYNTAXANALYZER_HPP

#include <iostream>
#include <vector>
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

    bool optExprAssign() {
        if (!crtTk()) return true;
        TokenType t = crtTk()->type;
        if (t == TokenType::SEMICOLON || t == TokenType::RPAR) return true;
        return exprAssign();
    }

    bool optExpr() {
        if (!crtTk()) return true;
        TokenType t = crtTk()->type;
        if (t == TokenType::SEMICOLON || t == TokenType::RPAR) return true;
        return expr();
    }

    bool unit() {
        while (currentTokenIndex < tokens.size()) {
            if (!(declStruct() || declFunc() || declVar() || stm())) {
                tkerr("Unexpected token.");
                currentTokenIndex++;  // Skip to avoid infinite loop
            }
        }
        return true;
    }

    // ----------------- Declarations -----------------

    bool declStruct() {
        size_t save = currentTokenIndex;
        if (!consume(TokenType::STRUCT)) return false;
        if (!consume(TokenType::ID)) { 
            tkerr("Expected struct name after STRUCT."); 
            currentTokenIndex = save; 
            return false; 
        }
        // Check if this is a struct definition:
        if (!crtTk() || crtTk()->type != TokenType::LACC) {
            // Not a definition; revert consumption so that "struct Pt" can be used as a type.
            currentTokenIndex = save;
            return false;
        }
        // Proceed with struct definition:
        if (!consume(TokenType::LACC)) { 
            tkerr("Expected { after struct name."); 
            currentTokenIndex = save; 
            return false; 
        }
        while (declVar());  // Zero or more variable declarations
        if (!consume(TokenType::RACC)) { 
            tkerr("Expected } at the end of struct."); 
            currentTokenIndex = save; 
            return false; 
        }
        if (!consume(TokenType::SEMICOLON)) { 
            tkerr("Expected ; after struct declaration."); 
            currentTokenIndex = save; 
            return false; 
        }
        return true;
    }    

    bool declVar() {
        if (!typeBase()) return false;
        if (!consume(TokenType::ID)) tkerr("Expected variable name after type.");
        arrayDecl();
        if (consume(TokenType::ASSIGN)) {
            if (!exprAssign()) tkerr("Invalid initializer in declaration.");
        }
        while (consume(TokenType::COMMA)) {
            if (!consume(TokenType::ID)) tkerr("Expected variable name after comma.");
            arrayDecl();
            if (consume(TokenType::ASSIGN)) {
                if (!exprAssign()) tkerr("Invalid initializer in declaration.");
            }
        }
        if (!consume(TokenType::SEMICOLON)) tkerr("Expected ; at end of variable declaration.");
        return true;
    }

    bool structType() {
        size_t save = currentTokenIndex;
        if (!consume(TokenType::STRUCT))
            return false;
        if (!consume(TokenType::ID)) {
            currentTokenIndex = save;
            return false;
        }
        // If the next token is LACC, then this is a definition, not a type usage.
        if (crtTk() && crtTk()->type == TokenType::LACC) {
            currentTokenIndex = save;
            return false;
        }
        return true;
    }

    bool typeBase() {
        return consume(TokenType::INT) ||
               consume(TokenType::DOUBLE) ||
               consume(TokenType::CHAR) ||
               structType();
    }

    bool arrayDecl() {
        if (!consume(TokenType::LBRACKET)) return false;
        expr();  // Optional size expression
        if (!consume(TokenType::RBRACKET)) tkerr("Expected ] after array declaration.");
        return true;
    }

    bool typeName() {
        if (!typeBase()) return false;
        arrayDecl();
        return true;
    }

    bool declFunc() {
        size_t save = currentTokenIndex;
        if (!(typeBase() || consume(TokenType::VOID)))
            return false;
        consume(TokenType::MUL); // optional pointer type
        if (!consume(TokenType::ID)) {
            tkerr("Expected function name.");
            return false;
        }
        // Lookahead: if the next token is not LPAR, it's not a function declaration.
        if (!crtTk() || crtTk()->type != TokenType::LPAR) {
            currentTokenIndex = save;
            return false;
        }
        if (!consume(TokenType::LPAR)) {
            tkerr("Expected ( after function name.");
            return false;
        }
        if (funcArg()) {
            while (consume(TokenType::COMMA)) {
                if (!funcArg()) {
                    tkerr("Expected function argument after comma.");
                    return false;
                }
            }
        }
        if (!consume(TokenType::RPAR)) {
            tkerr("Expected ) after function parameters.");
            return false;
        }
        if (!stmCompound()) {
            tkerr("Expected function body after declaration.");
            return false;
        }
        return true;
    }    

    bool funcArg() {
        if (!typeBase()) return false;
        if (!consume(TokenType::ID)) tkerr("Expected argument name.");
        arrayDecl();
        return true;
    }

    // ----------------- Statements -----------------

    bool stm() {
        if (stmCompound()) return true;

        if (consume(TokenType::IF)) {
            if (!consume(TokenType::LPAR)) tkerr("Expected ( after IF.");
            if (!expr()) tkerr("Expected expression in IF condition.");
            if (!consume(TokenType::RPAR)) tkerr("Expected ) after IF condition.");
            if (!stm()) tkerr("Expected statement after IF.");
            if (consume(TokenType::ELSE)) {
                if (!stm()) tkerr("Expected statement after ELSE.");
            }
            return true;
        }

        if (consume(TokenType::WHILE)) {
            if (!consume(TokenType::LPAR)) tkerr("Expected ( after WHILE.");
            if (!expr()) tkerr("Expected expression in WHILE condition.");
            if (!consume(TokenType::RPAR)) tkerr("Expected ) after WHILE condition.");
            if (!stm()) tkerr("Expected statement after WHILE.");
            return true;
        }

        if (ruleFor()) return true;

        if (consume(TokenType::BREAK)) {
            if (!consume(TokenType::SEMICOLON)) tkerr("Expected ; after BREAK.");
            return true;
        }

        if (consume(TokenType::RETURN)) {
            if (crtTk() && crtTk()->type != TokenType::SEMICOLON) {
                if (!expr()) tkerr("Invalid return expression.");
            }
            if (!consume(TokenType::SEMICOLON)) tkerr("Expected ; after RETURN.");
            return true;
        }

        if (exprAssign()) {
            if (!consume(TokenType::SEMICOLON)) tkerr("Expected ; after expression.");
            return true;
        }

        return false;
    }

    bool stmCompound() {
        if (!consume(TokenType::LACC)) return false;
        while (declVar() || stm());
        if (!consume(TokenType::RACC)) {
            tkerr("Expected } at end of compound statement.");
            return false;
        }
        return true;
    }

    // ----------------- Expressions -----------------

    bool expr() {
        return exprAssign();
    }

    bool exprAssign() {
        size_t save = currentTokenIndex;
        if (exprUnary() && consume(TokenType::ASSIGN)) {
            if (!exprAssign()) {
                tkerr("Invalid assignment expression.");
                return false;
            }
            return true;
        }
        currentTokenIndex = save;
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
            if (!exprMul()) tkerr("Invalid addition/subtraction expression.");
        }
        return true;
    }

    bool exprMul() {
        if (!exprCast()) return false;
        while (consume(TokenType::MUL) || consume(TokenType::DIV)) {
            if (!exprCast()) tkerr("Invalid multiplication/division expression.");
        }
        return true;
    }

    bool exprCast() {
        if (consume(TokenType::LPAR)) {
            size_t save = currentTokenIndex;
            if (typeName()) {
                if (!consume(TokenType::RPAR)) tkerr("Expected ) after type cast.");
                return exprCast();
            }
            currentTokenIndex = save - 1;
        }
        return exprUnary();
    }

    bool exprUnary() {
        if (consume(TokenType::SUB) || consume(TokenType::NOT))
            return exprUnary();
        return exprPostfix();
    }

    bool exprPostfix() {
        if (!exprPrimary()) return false;
        while (true) {
            if (consume(TokenType::LBRACKET)) {
                if (!expr()) tkerr("Invalid expression in array access.");
                if (!consume(TokenType::RBRACKET)) tkerr("Expected ] after array access.");
            } else if (consume(TokenType::DOT)) {
                if (!consume(TokenType::ID)) tkerr("Expected identifier after .");
            } else {
                break;
            }
        }
        return true;
    }

    bool exprPrimary() {
        if (consume(TokenType::ID)) {
            if (consume(TokenType::LPAR)) {
                if (crtTk() && crtTk()->type != TokenType::RPAR) {
                    if (!expr()) tkerr("Invalid expression in function call.");
                    while (consume(TokenType::COMMA)) {
                        if (!expr()) tkerr("Invalid expression in function call.");
                    }
                }
                if (!consume(TokenType::RPAR)) tkerr("Expected ) after function call.");
            }
            return true;
        }
        if (consume(TokenType::CT_INT) || consume(TokenType::CT_REAL) ||
            consume(TokenType::CT_CHAR) || consume(TokenType::CT_STRING))
            return true;
        if (consume(TokenType::LPAR)) {
            if (!expr()) tkerr("Invalid expression inside parentheses.");
            if (!consume(TokenType::RPAR)) tkerr("Expected ) after expression.");
            return true;
        }
        return false;
    }

    bool ruleFor() {
        if (!consume(TokenType::FOR)) return false;
        if (!consume(TokenType::LPAR)) tkerr("Expected ( after FOR.");
        if (!optExprAssign()) tkerr("Invalid initialization in FOR loop.");
        if (!consume(TokenType::SEMICOLON)) tkerr("Expected ; after initialization.");
        if (!optExpr()) tkerr("Invalid condition in FOR loop.");
        if (!consume(TokenType::SEMICOLON)) tkerr("Expected ; after condition.");
        if (!optExprAssign()) tkerr("Invalid increment in FOR loop.");
        if (!consume(TokenType::RPAR)) tkerr("Expected ) after FOR loop.");
        if (!stm()) tkerr("Expected statement after FOR.");
        return true;
    }
};

#endif