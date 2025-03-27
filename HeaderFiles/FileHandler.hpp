#ifndef FILEHANDLER_HPP
#define FILEHANDLER_HPP

#include <iostream>
#include <fstream>
#include <string>
#include "ErrorHandler.hpp"

using namespace std;

class FileHandler {
private:
    ifstream file;
    string filePath;
    char currentChar;
    char putBackBuffer; // Buffer to store a single put-back character
    bool hasPutBack;    // Flag to indicate if "putBackBuffer" buffer is occupied
    int lineNumber;
    int columnNumber;

public:
    FileHandler(const string& path) : filePath(path), hasPutBack(false), lineNumber(1), columnNumber(1) {
        openFile();
    }

    void openFile() {
        file.open(filePath, ios::binary);
        if (!file) {
            ErrorHandler::printErrorOpeningTheFile(filePath);
        }
    }

    void closeFile() {
        if (file.is_open()) file.close();
        else ErrorHandler::printErrorClosingTheFile(filePath);
    }

    bool ifOpen() const {
        return file.is_open();
    }

    char getNextChar() {
        if (hasPutBack) {  // If we have a stored character, return it first
            hasPutBack = false;
            return putBackBuffer;
        }

        if (file.is_open() && file.get(currentChar)) {
            if (currentChar == '\n') {  // Track new lines
                lineNumber++;
                columnNumber = 1;
            } else {
                columnNumber++;
            }
            return currentChar;
        }
        
        closeFile(); // Auto-close at EOF
        return EOF;
    }

    void putBackChar(char ch) {  // Store character in the buffer
        hasPutBack = true;
        putBackBuffer = ch;
    }

    int getLineNumber() const {
        return lineNumber;
    }

    int getColumnNumber() const {
        return columnNumber;
    }

    string getFilePath() const {
        return filePath;
    }

    string toString() const {
        return "File Path: " + filePath + 
               "\nCurrent Char: " + string(1, currentChar) +
               "\nLine: " + to_string(lineNumber) + 
               "\nColumn: " + to_string(columnNumber);
    }
};

#endif