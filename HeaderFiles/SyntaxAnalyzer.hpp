#ifndef SYNTAXANALYZER_HPP
#define SYNTAXANALYZER_HPP

#include <iostream>
#include <vector>
#include "Token.hpp"
#include "ErrorHandler.hpp"
#include "LexicalRules.hpp"
#include "SemanticContext.hpp"

using namespace std;

class SyntaxAnalyzer {
private:
    vector<Token> tokens;
    size_t currentTokenIndex;
    Token* consumedTk;
    SemanticContext& semCtx;

public:
    SyntaxAnalyzer(vector<Token> tokenStream, SemanticContext& context)
        : tokens(tokenStream), currentTokenIndex(0), semCtx(context) {}

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

    // ------------Centralized methods--------------
    bool parseOptionalInitializer() {
        if (consume(TokenType::ASSIGN)) {
            if (!exprAssign()) {
                tkerr("Invalid initializer.");
                return false;
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
        Token* structNameToken = consumedTk;

        // ❗ Only define symbol if it's really a definition (must follow with `{`)
        if (!crtTk() || crtTk()->type != TokenType::LACC) {
            currentTokenIndex = save;
            return false;
        }

        // --- SEMANTIC ---
        if (semCtx.symbols.find(structNameToken->lexeme)) {
            ErrorHandler::printSemanticErrorDefinition("Symbol redefinition", structNameToken->lexeme);
        }

        Symbol* structSym = semCtx.symbols.add(structNameToken->lexeme, CLS_STRUCT, semCtx.crtDepth);
        semCtx.crtStruct = structSym;

        // Init empty members table
        structSym->members.clear();

        // ----------------

        if (!consume(TokenType::LACC)) {
            tkerr("Expected { after struct name.");
            currentTokenIndex = save;
            return false;
        }

        while (declVar());

        if (!consume(TokenType::RACC)) {
            tkerr("Expected } at the end of struct.");
            return false;
        }

        if (!consume(TokenType::SEMICOLON)) {
            tkerr("Expected ; after struct declaration.");
            return false;
        }

        // --- SEMANTIC ---
        semCtx.crtStruct = nullptr;
        // ----------------

        return true;
    }

    bool declVar() {
        if (!typeBase()) return false;

        Token* varNameToken = nullptr;
        Type varType;

        // --- SYNTAX ---
        if (!consume(TokenType::ID)) {
            tkerr("Expected variable name after type.");
            return false;
        }

        if (consume(TokenType::ASSIGN)) {
            if (!exprAssign()) {
                tkerr("Invalid initializer in declaration.");
                return false;
            }
        }
        
        varNameToken = consumedTk;

        arrayDecl(varType);  // will update nElements if found
        if (varType.nElements < 0) varType.nElements = -1;

        if (!parseOptionalInitializer()) return false;

        // --- SEMANTIC ---
        if (semCtx.crtStruct) {
            auto it = find_if(
                semCtx.crtStruct->members.begin(), 
                semCtx.crtStruct->members.end(),
                [&](Symbol* s) { return s->name == varNameToken->lexeme; }
            );
            if (it != semCtx.crtStruct->members.end()) {
                ErrorHandler::printSemanticErrorDefinition("Member redefinition", varNameToken->lexeme);
            }
            Symbol* s = new Symbol();
            s->name = varNameToken->lexeme;
            s->cls = CLS_VAR;
            s->mem = MEM_LOCAL;  // For members, mem isn't really used much
            s->type = varType;
            s->depth = semCtx.crtDepth;
            semCtx.crtStruct->members.push_back(s);
        }
        else {
            Symbol* existing = semCtx.symbols.find(varNameToken->lexeme);
            if (existing && existing->depth == semCtx.crtDepth) {
                ErrorHandler::printSemanticErrorDefinition("Variable redefinition", varNameToken->lexeme);
            }

            Symbol* s = semCtx.symbols.add(varNameToken->lexeme, CLS_VAR, semCtx.crtDepth);
            s->mem = semCtx.crtFunc ? MEM_LOCAL : MEM_GLOBAL;
            s->type = varType;
        }
        // ------------------

        // Continue with comma-separated variables
        while (consume(TokenType::COMMA)) {
            if (!consume(TokenType::ID)) {
                tkerr("Expected variable name after comma.");
                return false;
            }
            Token* nextVarToken = consumedTk;
            arrayDecl(varType);
            if (varType.nElements < 0) varType.nElements = -1;

            // --- SEMANTIC ---
            if (semCtx.crtStruct) {
                auto it = find_if(
                    semCtx.crtStruct->members.begin(), 
                    semCtx.crtStruct->members.end(),
                    [&](Symbol* s) { return s->name == nextVarToken->lexeme; }
                );
                if (it != semCtx.crtStruct->members.end()) {
                    ErrorHandler::printSemanticErrorDefinition("Member redefinition", varNameToken->lexeme);
                }
                Symbol* s = new Symbol();
                s->name = nextVarToken->lexeme;
                s->cls = CLS_VAR;
                s->mem = MEM_LOCAL;
                s->type = varType;
                s->depth = semCtx.crtDepth;
                semCtx.crtStruct->members.push_back(s);
            }
            else {
                Symbol* existing = semCtx.symbols.find(nextVarToken->lexeme);
                if (existing && existing->depth == semCtx.crtDepth) {
                    ErrorHandler::printSemanticErrorDefinition("Variable redefinition", nextVarToken->lexeme);
                }

                Symbol* s = semCtx.symbols.add(nextVarToken->lexeme, CLS_VAR, semCtx.crtDepth);
                s->mem = semCtx.crtFunc ? MEM_LOCAL : MEM_GLOBAL;
                s->type = varType;
            }
            // ------------------
        }

        if (!consume(TokenType::SEMICOLON)) {
            tkerr("Expected ; at end of variable declaration.");
            return false;
        }

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

    bool arrayDecl(Type& t) {
        if (!consume(TokenType::LBRACKET)) return false;
        if (expr()) {
            t.nElements = 0;  // Optional: For now we don't compute value.
        } else {
            t.nElements = 0;  // Even no expr means unspecified size
        }
        if (!consume(TokenType::RBRACKET)) tkerr("Expected ] after array declaration.");
        return true;
    }

    bool typeName(Type& outType) {
        // Parse the base type and initialize outType
        if (consume(TokenType::INT)) {
            outType.typeBase = TB_INT;
            outType.nElements = -1;
        } else if (consume(TokenType::DOUBLE)) {
            outType.typeBase = TB_DOUBLE;
            outType.nElements = -1;
        } else if (consume(TokenType::CHAR)) {
            outType.typeBase = TB_CHAR;
            outType.nElements = -1;
        } else if (consume(TokenType::STRUCT)) {
            if (!consume(TokenType::ID)) {
                tkerr("Expected struct name after STRUCT.");
                return false;
            }
            Token* structName = consumedTk;

            // --- SEMANTIC ---
            Symbol* s = semCtx.symbols.find(structName->lexeme);
            if (!s) {
                ErrorHandler::printSemanticErrorDefinition("Undefined struct", structName->lexeme);
                return false;
            }
            if (s->cls != CLS_STRUCT) {
                ErrorHandler::printSemanticErrorDefinition("Not a struct", structName->lexeme);
                return false;
            }
            outType.typeBase = TB_STRUCT;
            outType.s = s;
            outType.nElements = -1;
            // ----------------
        } else {
            return false;
        }

        // Check for optional array part
        if (consume(TokenType::LBRACKET)) {
            outType.nElements = 0;
            expr(); // optional index expression, not evaluated here
            if (!consume(TokenType::RBRACKET)) {
                tkerr("Expected ] after array declaration.");
                return false;
            }
        }

        return true;
    }

    bool declFunc() {
        size_t save = currentTokenIndex;
        Type t;  // ← Define return type of the function

        if (consume(TokenType::INT)) {
            t.typeBase = TB_INT;
        } else if (consume(TokenType::DOUBLE)) {
            t.typeBase = TB_DOUBLE;
        } else if (consume(TokenType::CHAR)) {
            t.typeBase = TB_CHAR;
        } else if (consume(TokenType::STRUCT)) {
            if (!consume(TokenType::ID)) {
                tkerr("Expected struct name after STRUCT.");
                return false;
            }
            Token* structName = consumedTk;
            Symbol* structSym = semCtx.symbols.find(structName->lexeme);
            if (!structSym || structSym->cls != CLS_STRUCT) {
                ErrorHandler::printSemanticErrorDefinition("Invalid struct type", structName->lexeme);
                return false;
            }
            t.typeBase = TB_STRUCT;
            t.s = structSym;
        } else if (consume(TokenType::VOID)) {
            t.typeBase = TB_VOID;
        } else {
            return false;  // Not a valid function return type
        }

        // Optional MUL (pointer, not needed here, just placeholder)
        if (consume(TokenType::MUL)) {
            t.nElements = 0;
        } else {
            t.nElements = -1;
        }
        if (!consume(TokenType::ID)) {
            tkerr("Expected function name.");
            return false;
        }

        Token* funcNameToken = consumedTk;

        // --- SEMANTIC ---
        if (semCtx.symbols.find(funcNameToken->lexeme)) {
            ErrorHandler::printSemanticErrorDefinition("Function redefinition", funcNameToken->lexeme);
        }
        Symbol* funcSym = semCtx.symbols.add(funcNameToken->lexeme, CLS_FUNC, semCtx.crtDepth);
        funcSym->type = t;
        semCtx.crtFunc = funcSym;
        semCtx.crtDepth++;
        // Clear args vector (in case of reuse)
        funcSym->args.clear();

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

        semCtx.crtDepth--;

        if (!stmCompound()) {
            tkerr("Expected function body after declaration.");
            return false;
        }

        semCtx.symbols.deleteAfter(funcSym); // remove args & locals
        semCtx.crtFunc = nullptr;

        return true;
    }    

    bool funcArg() {
        Type t;

        // --- Parse the type ---
        if (consume(TokenType::INT)) {
            t.typeBase = TB_INT;
        } else if (consume(TokenType::DOUBLE)) {
            t.typeBase = TB_DOUBLE;
        } else if (consume(TokenType::CHAR)) {
            t.typeBase = TB_CHAR;
        } else if (consume(TokenType::STRUCT)) {
            if (!consume(TokenType::ID)) {
                tkerr("Expected struct name after STRUCT.");
                return false;
            }
            Token* structName = consumedTk;
            Symbol* structSym = semCtx.symbols.find(structName->lexeme);
            if (!structSym || structSym->cls != CLS_STRUCT) {
                ErrorHandler::printSemanticErrorDefinition("Undefined or invalid struct", structName->lexeme);
                return false;
            }
            t.typeBase = TB_STRUCT;
            t.s = structSym;
        } else {
            return false;
        }

        if (consume(TokenType::LBRACKET)) {
            t.nElements = 0;  // open array
            if (!consume(TokenType::RBRACKET)) {
                tkerr("Expected ] after [ in array declaration.");
                return false;
            }
        } else {
            t.nElements = -1;  // not an array
        }

        // --- Expect variable name ---
        if (!consume(TokenType::ID)) {
            tkerr("Expected argument name.");
            return false;
        }

        Token* argName = consumedTk;

        // --- SEMANTIC CHECK ---
        // Check redefinition in the current function scope
        Symbol* existing = semCtx.symbols.find(argName->lexeme);
        if (existing && existing->depth == semCtx.crtDepth) {
            ErrorHandler::printSemanticErrorDefinition("Argument redefinition", argName->lexeme);
        }

        // Add to global symbol table
        Symbol* argSym = semCtx.symbols.add(argName->lexeme, CLS_VAR, semCtx.crtDepth);
        argSym->mem = MEM_ARG;
        argSym->type = t;

        // Also add to function's formal argument list
        Symbol* funcArgSym = new Symbol(*argSym);
        semCtx.crtFunc->args.push_back(funcArgSym);

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

        // --- SEMANTIC: Enter new block scope ---
        Symbol* startMarker = semCtx.symbols.last();
        semCtx.crtDepth++;

        // --- Parse declarations and statements inside block ---
        while (declVar() || stm());

        // --- Expect closing brace ---
        if (!consume(TokenType::RACC)) {
            tkerr("Expected } at end of compound statement.");
            return false;
        }

        // --- SEMANTIC: Exit block and remove block symbols ---
        semCtx.crtDepth--;
        semCtx.symbols.deleteAfter(startMarker);

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
            Type castType;  // synthesized attribute for cast type

            if (typeName(castType)) {
                if (!consume(TokenType::RPAR)) {
                    tkerr("Expected ) after type cast.");
                    return false;
                }

                // --- SEMANTIC ---
                // You might later want to record the cast type here if needed for evaluation/codegen
                // Example: save castType in an AST node or current expression context
                // ----------------

                return exprCast();  // Continue parsing casted expression
            }

            // Not a cast, restore and reparse as regular expression
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