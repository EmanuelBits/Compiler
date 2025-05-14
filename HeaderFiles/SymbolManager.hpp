#ifndef SYMBOLMANAGER_HPP
#define SYMBOLMANAGER_HPP

#include <vector>
#include <string>
#include <unordered_map>
#include "Token.hpp"
#include "ErrorHandler.hpp"
#include <algorithm>

using namespace std;

enum TypeBase { TB_INT, TB_DOUBLE, TB_CHAR, TB_STRUCT, TB_VOID };
enum ClassType { CLS_VAR, CLS_FUNC, CLS_EXTFUNC, CLS_STRUCT };
enum MemoryType { MEM_GLOBAL, MEM_ARG, MEM_LOCAL };

struct Type {
    TypeBase typeBase;
    struct Symbol* s = nullptr;  // only if TB_STRUCT
    int nElements = -1;          // <0 = not array, 0 = [] no size, >0 = [n]
};

struct Symbol {
    string name;
    ClassType cls;
    MemoryType mem;
    Type type;
    int depth;

    vector<Symbol*> args;     // only for CLS_FUNC
    vector<Symbol*> members;  // only for CLS_STRUCT
};

class SymbolManager {
private:
    vector<Symbol*> symbols;

public:
    void clear() {
        for (auto s : symbols) delete s;
        symbols.clear();
    }

    Symbol* add(const string& name, ClassType cls, int crtDepth) {
        Symbol* sym = new Symbol();
        sym->name = name;
        sym->cls = cls;
        sym->depth = crtDepth;
        symbols.push_back(sym);
        return sym;
    }

    Symbol* find(const string& name) const {
        for (auto it = symbols.rbegin(); it != symbols.rend(); ++it) {
            if ((*it)->name == name) return *it;
        }
        return nullptr;
    }

    void deleteAfter(Symbol* afterSymbol) {
        auto it = std::find(symbols.begin(), symbols.end(), afterSymbol);
        if (it != symbols.end()) {
            for (auto del = it + 1; del != symbols.end(); ++del) delete *del;
            symbols.erase(it + 1, symbols.end());
        }
    }

    Symbol* last() const {
        return symbols.empty() ? nullptr : symbols.back();
    }

    const vector<Symbol*>& getAll() const { return symbols; }
};

#endif