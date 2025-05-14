#ifndef SEMANTICCONTEXT_HPP
#define SEMANTICCONTEXT_HPP

#include "SymbolManager.hpp"

struct SemanticContext {
    SymbolManager symbols;
    int crtDepth = 0;
    Symbol* crtFunc = nullptr;
    Symbol* crtStruct = nullptr;
};

#endif