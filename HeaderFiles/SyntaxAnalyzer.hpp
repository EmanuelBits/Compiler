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

    // Return pointer to the current token (or nullptr at end)
    Token* crtTk() {
        return (currentTokenIndex < tokens.size()) ? &tokens[currentTokenIndex] : nullptr;
    }

    // Consume a token if it matches the expected type
    bool consume(TokenType type) {
        if (crtTk() && crtTk()->type == type) {
            consumedTk = crtTk();
            currentTokenIndex++;
            return true;
        }
        return false;
    }

    // Print a syntax error using our dedicated syntax error printer.
    void tkerr(const string& message) {
        if (crtTk()) {
            ErrorHandler::printSyntaxError(message, crtTk()->line, crtTk()->column);
        } else {
            ErrorHandler::printSyntaxError("Syntax error at end of file: " + message, -1, -1);
        }
    }

    // Helper: Optional expression assignment (used in for-loop parts)
    bool optExprAssign() {
        if (!crtTk()) return true;
        TokenType t = crtTk()->type;
        if (t == TokenType::SEMICOLON || t == TokenType::RPAR)
            return true;
        return exprAssign();
    }

    // Helper: Optional expression (non-assignment)
    bool optExpr() {
        if (!crtTk()) return true;
        TokenType t = crtTk()->type;
        if (t == TokenType::SEMICOLON || t == TokenType::RPAR)
            return true;
        return expr();
    }

    // Starting rule: process tokens until end.
    bool unit() {
        while (currentTokenIndex < tokens.size()) {
            // Try parsing a declaration (struct, function, var) or a statement.
            if (!(declStruct() || declFunc() || declVar() || stm())) {
                tkerr("Unexpected token.");
                currentTokenIndex++; // skip token to prevent infinite loop
            }
        }
        return true;
    }

    // ----------------- Declarations -----------------
    bool declStruct() {
        if (!consume(TokenType::STRUCT)) return false;
        if (!consume(TokenType::ID)) { tkerr("Expected struct name after STRUCT."); }
        if (!consume(TokenType::LACC)) { tkerr("Expected { after struct name."); }
        while (declVar());  // Zero or more variable declarations
        if (!consume(TokenType::RACC)) { tkerr("Expected } at the end of struct."); }
        if (!consume(TokenType::SEMICOLON)) { tkerr("Expected ; after struct declaration."); }
        return true;
    }

    bool declVar() {
        if (!typeBase()) return false;
        if (!consume(TokenType::ID)) { tkerr("Expected variable name after type."); }
        arrayDecl();
        while (consume(TokenType::COMMA)) {
            if (!consume(TokenType::ID)) { tkerr("Expected variable name after comma."); }
            arrayDecl();
        }
        if (!consume(TokenType::SEMICOLON)) { tkerr("Expected ; at end of variable declaration."); }
        return true;
    }

    bool typeBase() {
        return consume(TokenType::INT) || consume(TokenType::DOUBLE) ||
               consume(TokenType::CHAR) ||
               (consume(TokenType::STRUCT) && consume(TokenType::ID));
    }

    bool arrayDecl() {
        if (!consume(TokenType::LBRACKET)) return false;
        expr();  // Optional expression inside array declaration
        if (!consume(TokenType::RBRACKET)) { tkerr("Expected ] after array declaration."); }
        return true;
    }

    bool typeName() {
        if (!typeBase()) return false;
        arrayDecl();
        return true;
    }

    bool declFunc() {
        if (!(typeBase() || consume(TokenType::VOID))) return false;
        consume(TokenType::MUL); // Optional pointer (for vector return types)
        if (!consume(TokenType::ID)) { tkerr("Expected function name."); }
        if (!consume(TokenType::LPAR)) { tkerr("Expected ( after function name."); }
        if (funcArg()) {
            while (consume(TokenType::COMMA)) {
                if (!funcArg()) { tkerr("Expected function argument after comma."); }
            }
        }
        if (!consume(TokenType::RPAR)) { tkerr("Expected ) after function parameters."); }
        if (!stmCompound()) { tkerr("Expected function body after declaration."); }
        return true;
    }

    bool funcArg() {
        if (!typeBase()) return false;
        if (!consume(TokenType::ID)) { tkerr("Expected argument name."); }
        arrayDecl();
        return true;
    }

    // ----------------- Statements -----------------
    bool stm() {
        // Compound statement
        if (stmCompound()) return true;

        // if-statement
        if (consume(TokenType::IF)) {
            if (!consume(TokenType::LPAR)) { tkerr("Expected ( after IF."); return false; }
            if (!expr()) { tkerr("Expected expression in IF condition."); return false; }
            if (!consume(TokenType::RPAR)) { tkerr("Expected ) after IF condition."); return false; }
            if (!stm()) { tkerr("Expected statement after IF."); return false; }
            if (consume(TokenType::ELSE)) {
                if (!stm()) { tkerr("Expected statement after ELSE."); return false; }
            }
            return true;
        }

        // while-statement
        if (consume(TokenType::WHILE)) {
            if (!consume(TokenType::LPAR)) { tkerr("Expected ( after WHILE."); return false; }
            if (!expr()) { tkerr("Expected expression in WHILE condition."); return false; }
            if (!consume(TokenType::RPAR)) { tkerr("Expected ) after WHILE condition."); return false; }
            if (!stm()) { tkerr("Expected statement after WHILE."); return false; }
            return true;
        }

        // for-statement (rewritten as separate helper)
        if (ruleFor()) return true;

        // break statement
        if (consume(TokenType::BREAK)) {
            if (!consume(TokenType::SEMICOLON)) { tkerr("Expected ; after BREAK."); return false; }
            return true;
        }

        // return statement
        if (consume(TokenType::RETURN)) {
            // Optional expression
            if (crtTk() && crtTk()->type != TokenType::SEMICOLON) {
                if (!expr()) { tkerr("Invalid return expression."); return false; }
            }
            if (!consume(TokenType::SEMICOLON)) { tkerr("Expected ; after RETURN."); return false; }
            return true;
        }

        // Expression statement
        if (exprAssign()) {
            if (!consume(TokenType::SEMICOLON)) { tkerr("Expected ; after expression."); return false; }
            return true;
        }

        return false;
    }

    // Compound statement: { stmt* }
    bool stmCompound() {
        if (!consume(TokenType::LACC)) return false;
        while (declVar() || stm()) { }  // Process declarations and statements inside block
        if (!consume(TokenType::RACC)) { tkerr("Expected } at end of compound statement."); return false; }
        return true;
    }

    // ----------------- Expression Parsing -----------------
    bool expr() {
        return exprAssign();
    }

    bool exprAssign() {
        // Try to parse an assignment expression; if left-hand side is valid, check for '='.
        size_t saveIndex = currentTokenIndex;
        if (exprUnary()) {
            if (consume(TokenType::ASSIGN)) {
                if (!exprAssign()) { tkerr("Invalid assignment expression."); return false; }
                return true;
            }
            // If no '=' found, restore state and try exprOr
            currentTokenIndex = saveIndex;
        }
        return exprOr();
    }

    bool exprOr() {
        if (!exprAnd()) return false;
        while (consume(TokenType::OR)) {
            if (!exprAnd()) { tkerr("Invalid OR expression."); return false; }
        }
        return true;
    }

    bool exprAnd() {
        if (!exprEq()) return false;
        while (consume(TokenType::AND)) {
            if (!exprEq()) { tkerr("Invalid AND expression."); return false; }
        }
        return true;
    }

    bool exprEq() {
        if (!exprRel()) return false;
        while (consume(TokenType::EQUAL) || consume(TokenType::NOTEQ)) {
            if (!exprRel()) { tkerr("Invalid equality expression."); return false; }
        }
        return true;
    }

    bool exprRel() {
        if (!exprAdd()) return false;
        while (consume(TokenType::LESS) || consume(TokenType::LESSEQ) ||
               consume(TokenType::GREATER) || consume(TokenType::GREATEREQ)) {
            if (!exprAdd()) { tkerr("Invalid relational expression."); return false; }
        }
        return true;
    }

    bool exprAdd() {
        if (!exprMul()) return false;
        while (consume(TokenType::ADD) || consume(TokenType::SUB)) {
            if (!exprMul()) { tkerr("Invalid addition/subtraction expression."); return false; }
        }
        return true;
    }

    bool exprMul() {
        if (!exprCast()) return false;
        while (consume(TokenType::MUL) || consume(TokenType::DIV)) {
            if (!exprCast()) { tkerr("Invalid multiplication/division expression."); return false; }
        }
        return true;
    }

    bool exprCast() {
        if (consume(TokenType::LPAR)) {
            size_t saveIndex = currentTokenIndex;
            if (typeName()) {  // This is a cast
                if (!consume(TokenType::RPAR)) { tkerr("Expected ) after type cast."); return false; }
                return exprCast();
            }
            // Not a type cast; revert consumption of '('
            currentTokenIndex = saveIndex - 1;  // "Put back" the LPAR
            return exprUnary();
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
                if (!expr()) { tkerr("Invalid expression in array access."); return false; }
                if (!consume(TokenType::RBRACKET)) { tkerr("Expected ] after array access."); return false; }
            }
            else if (consume(TokenType::DOT)) {
                if (!consume(TokenType::ID)) { tkerr("Expected identifier after ."); return false; }
            }
            else break;
        }
        return true;
    }

    bool exprPrimary() {
        if (consume(TokenType::ID)) {
            // Function call
            if (consume(TokenType::LPAR)) {
                if (crtTk() && crtTk()->type != TokenType::RPAR) {
                    if (!expr()) { tkerr("Invalid expression in function call."); return false; }
                    while (consume(TokenType::COMMA)) {
                        if (!expr()) { tkerr("Invalid expression in function call."); return false; }
                    }
                }
                if (!consume(TokenType::RPAR)) { tkerr("Expected ) after function call."); return false; }
            }
            return true;
        }
        if (consume(TokenType::CT_INT) || consume(TokenType::CT_REAL) ||
            consume(TokenType::CT_CHAR) || consume(TokenType::CT_STRING))
            return true;
        if (consume(TokenType::LPAR)) {
            if (!expr()) { tkerr("Invalid expression inside parentheses."); return false; }
            if (!consume(TokenType::RPAR)) { tkerr("Expected ) after expression."); return false; }
            return true;
        }
        return false;
    }

    // ----------------- FOR loop parsing as a separate helper -----------------
    bool ruleFor() {
        if (!consume(TokenType::FOR)) return false;
        if (!consume(TokenType::LPAR)) { tkerr("Expected ( after FOR."); return false; }
        
        // Optional initialization
        if (!optExprAssign()) { tkerr("Invalid initialization in FOR loop."); return false; }
        if (!consume(TokenType::SEMICOLON)) { tkerr("Expected ; after initialization in FOR loop."); return false; }
        
        // Optional condition
        if (!optExpr()) { tkerr("Invalid condition in FOR loop."); return false; }
        if (!consume(TokenType::SEMICOLON)) { tkerr("Expected ; after condition in FOR loop."); return false; }
        
        // Optional increment
        if (!optExprAssign()) { tkerr("Invalid increment in FOR loop."); return false; }
        if (!consume(TokenType::RPAR)) { tkerr("Expected ) after FOR loop."); return false; }
        
        if (!stm()) { tkerr("Expected statement after FOR loop."); return false; }
        return true;
    }

    // Override stm() to include FOR loop rule.
    bool stmWrapper() {
        if (ruleFor()) return true;
        return stm(); // Fallback to the original stm() rules.
    }
};

#endif