#ifndef ERRORHANDLER_HPP
#define ERRORHANDLER_HPP

#include <iostream>
#include <string>

//Regular text + RESET
#define BLACK   "\033[0;30m"
#define RED     "\033[0;31m"
#define GREEN   "\033[0;32m"
#define YELLOW  "\033[0;33m"
#define BLUE    "\033[0;34m"
#define MAGENTA "\033[0;35m"
#define CYAN    "\033[0;36m"
#define WHITE   "\033[0;37m"
#define RESET   "\033[0m"

using namespace std;

class ErrorHandler {
public:
    ErrorHandler() = delete;
    ~ErrorHandler() = delete;

    // FileHandler Errors
    static void printErrorOpeningTheFile(string file) {
        cerr << RED << "Error at opening the file: " << file << RESET << endl;
    }

    static void printErrorClosingTheFile(string file) {
        cerr << RED << "Error at closing the file: " << file << " File is NOT opened." << RESET << endl;
    }


    // Analyzers Errors
    static void printLexicalError(const string& message, int line, int column) {
        cerr << YELLOW << "Lexical Error at line " << line << ", column " << column << ": " << message << RESET << endl;
    }

    static void printSyntaxError(const string& message, int line, int column) {
        cerr << RED << "Syntax Error at line " << line << ", column " << column << ": " << message << RESET << endl;
    }

    static void printSemanticError(const string& message, int line, int column) {
        cerr << MAGENTA << "Semantic Error at line " << line << ", column " << column << ": " << message << RESET << endl;
    }

    static void printSemanticErrorDefinition(const string& message, const string& symbolName) {
        cerr << MAGENTA << "Semantic Error (Definition): " << message << " -> '" << symbolName << "'" << RESET << endl;
    }
};

#endif