#ifndef ERRORHANDLER_HPP
#define ERRORHANDLER_HPP

#include <iostream>
#include <string>

//Regular text + reset
#define BLACK   "\e[0;30m"
#define RED     "\e[0;31m"
#define GREEN   "\e[0;32m"
#define YELLOW  "\e[0;33m"
#define BLUE    "\e[0;34m"
#define MAGENTA "\e[0;35m"
#define CYAN    "\e[0;36m"
#define WHITE   "\e[0;37m"
#define reset   "\e[0m"

using namespace std;

class ErrorHandler {
public:
    ErrorHandler() = delete;
    ~ErrorHandler() = delete;

    // FileHandler Errors
    static void printErrorOpeningTheFile(string file) {
        cerr << RED << "Error at opening the file: " << file << reset << endl;
    }

    static void printErrorClosingTheFile(string file) {
        cerr << RED << "Error at closing the file: " << file << " File is NOT opened." << reset << endl;
    }


    // Analyzers Errors
    static void printLexicalError(const string& message, int line, int column) {
        cerr << YELLOW << "Lexical Error at line " << line << ", column " << column << ": " << message << reset << endl;
    }

    static void printSyntaxError(const string& message, int line, int column) {
        cerr << RED << "Syntax Error at line " << line << ", column " << column << ": " << message << reset << endl;
    }
};

#endif