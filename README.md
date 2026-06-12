datLeak C VM v0.1 - by gearn
==============================

DESCRIPTION
-----------
datLeak is a lightweight C interpreter/virtual machine.
It runs C code without compiling it natively.

BUILDING
--------
make

USAGE
-----
Run a file:
  ./datleak myfile.c

Interactive mode:
  ./datleak
  (write your code and press Ctrl+D when done)

SUPPORTED FEATURES
------------------
- printf with %d, %s, %c, %%, \n, \t
- int, char, long, short, unsigned variables
- Basic arithmetic: + - * /
- Operator precedence: * / before + -
- Unary negation: -x
- Parenthesized expressions: (x + y) * z
- Variable assignment: x = expr;
- return statement
- Single-line comments: //
- Block comments: /* */
- Preprocessor directives ignored: #include, #define

EXAMPLE
-------
#include <stdio.h>
int main() {
    int x = 10;
    int y = 5;
    printf("x = %d\n", x);
    printf("x * y = %d\n", x * y);
    return 0;
}

LICENSE
-------
MIT License - see LICENSE file.
Check LICENSE before using this as a base.
