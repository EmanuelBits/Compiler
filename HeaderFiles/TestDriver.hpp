#ifndef TESTDRIVER_HPP
#define TESTDRIVER_HPP

#include <iostream>
#include <filesystem>
#include <vector>
#include "LexicalRules.hpp"
#include "SyntaxAnalyzer.hpp"

#define GREEN   "\e[0;32m"
#define RED     "\e[0;31m"
#define CYAN    "\e[0;36m"
#define reset   "\e[0m"

namespace fs = std::filesystem;

class TestDriver {
private:
    string testDirectory = "AtomC-tests/";

public:
    void RunTests(bool automaticMode) {
        vector<string> testFiles;

        // Collect all test files from the directory
        for (const auto& entry : fs::directory_iterator(testDirectory)) {
            if (entry.path().extension() == ".c") {
                testFiles.push_back(entry.path().string());
            }
        }

        if (testFiles.empty()) {
            cerr << RED << "No test files found in " << testDirectory << reset << endl;
            return;
        }

        cout << CYAN << "Starting Tests...\n" << reset;

        for (const auto& file : testFiles) {
            cout << CYAN << "Testing: " << file << reset << endl;

            FileHandler fileHandler(file);
            if (!fileHandler.ifOpen()) {
                continue;  // Skip this test if the file cannot be opened
            }

            LexicalAnalyzer lexer(fileHandler);
            vector<Token> tokens = lexer.analyze();

            bool lexicalSuccess = !tokens.empty();

            if (lexicalSuccess) {
                cout << GREEN << "Lexical Analysis PASSED!" << reset << endl;
                for (const auto& token : tokens) {
                    cout << token.toString() << endl;
                }
            } else {
                cerr << RED << "Lexical Analysis FAILED: No valid tokens found!" << reset << endl;
                continue;  // Skip syntax analysis if lexical fails
            }

            // Run Syntax Analysis
            SyntaxAnalyzer parser(tokens);
            bool syntaxSuccess = parser.unit();

            if (syntaxSuccess) {
                cout << GREEN << "Syntax Analysis PASSED!" << reset << endl;
            } else {
                cerr << RED << "Syntax Analysis FAILED!" << reset << endl;
            }

            if (!automaticMode) {
                cout << CYAN << "Press Enter to continue, or type 'exit' to stop..." << reset << endl;
                string input;
                getline(cin, input);
                if (input == "exit") break;
            }
        }

        cout << CYAN << "Testing Complete." << reset << endl;
    }
};

#endif