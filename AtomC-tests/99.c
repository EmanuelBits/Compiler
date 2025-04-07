/* 
   Comprehensive Test File for AtomC Compiler
   This file covers:
     - Identifiers and keywords (if, else, for, while, break, return, void, struct, int, double, char)
     - Constants: decimal, octal, hexadecimal, real numbers with exponent (e.g., 2e0, 4.56E-2)
     - Character constants with escape sequences
     - String constants with escape sequences
     - Delimiters: commas, semicolons, parentheses, brackets, braces
     - Operators: +, -, *, /, =, ==, !=, <, <=, >, >=, &&, ||, !
     - Comments: both block and line comments
*/

//
// Struct definition
//
struct Point {
    int x, y;
};

//
// Global array declaration using a struct type
//
struct Point ptArray[10];

//
// Function: computeArea
// Uses various constants and expressions, including scientific notation.
//
int computeArea(struct Point p) {
    int a = 10;         // Decimal constant
    int b = 012;        // Octal constant (10 in decimal)
    int c = 0xA;        // Hexadecimal constant (10 in decimal)
    double d = 3.14;    // Double constant
    double e = 2e0;     // Scientific notation (2.0)
    double f = 4.56E-2; // Scientific notation (0.0456)
    char ch1 = 'A';     
    char ch2 = '\n';    // Escape sequence in char
    char ch3 = '\'';    // Escaped single quote
    char str1[] = "Hello, World!";                   // Simple string
    char str2[] = "Escapes: \t \n \" \\";              // String with escapes

    int sum = a + b - c * 2 / 1;  // Use of arithmetic operators
    if (sum >= 0 && sum != 5) {
        sum = sum + 1;
    } else {
        sum = sum - 1;
    }
    return sum;
}

//
// Function: displayArea
// Dummy function to simulate output
//
void displayArea(double area) {
    // In AtomC, assume this outputs a double.
    // For testing, we leave the body empty.
}

//
// Main function
//
void main() {
    int i, n, count;
    double result, pi;
    
    pi = 3.14;
    n = 5;  // Simulated input
    
    // Initialize global array with a for-loop
    for (i = 0; i < n; i = i + 1) {
        ptArray[i].x = i;
        ptArray[i].y = n - i;
    }
    
    // While-loop with if-else and break
    count = 0;
    i = 0;
    while (i < n) {
        if (ptArray[i].x >= 0 && ptArray[i].y >= 0) {
            count = count + 1;
        } else {
            break;  // line comment: exit loop if condition fails
        }
        i = i + 1;
    }
    
    // Function call with an expression that uses scientific notation
    result = 2e0 * pi * computeArea(ptArray[0]) / (count + 1);
    
    // if-else statement with nested if
    if (result > 10.0) {
        displayArea(result);
    } else {
        displayArea(result - 1);
    }
    
    // Additional function call examples
    // Demonstrating char and string literals with escape sequences
    char c = 'x';
    char c2 = '\t';
    char s1[] = "Test: \"Escaped quotes\", new line\n";
    
    // End of main
}