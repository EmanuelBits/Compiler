#include "HeaderFiles/TestDriver.hpp"
#include "HeaderFiles/ErrorHandler.hpp"

int main() {
    TestDriver tester;
    bool automaticMode = true;  // Change to false for manual mode
    tester.RunTests(automaticMode);
    return 0;
}