#ifndef EXPRESSION_PARSER_H
#define EXPRESSION_PARSER_H

/**
 @file expression_parser.h
 @author James Gregson james.gregson@gmail.com
 @brief A (fairly) simple C expression parser.  Hand-rolled recursive descent style algorithm implements the parser, removing the need for external tools such as  lex/yacc. Reads mathematical expression in infix notation (with a few built-in mathematical functions) and produces double-precision results.  
 
 The library handles:
    - standard arithmetic operations (+,-,*,/) with operator precedence
    - exponentiation ^ and nested exponentiation
    - unary + and -
    - expressions enclosed in parentheses ('(',')'), optionally nested
    - built-in math functions: pow(x,y), sqrt(x), log(x), exp(x), sin(x), asin(x),
      cos(x), acos(x), tan(x), atan(x), atan2(y,x), abs(x), fabs(x), floor(x),
      ceil(x), round(x), with input arguments checked for domain validity, e.g.
	  'sqrt( -1.0 )' returns an error.

 The library is also thread safe, allowing multiple parsers to be operated (on  different inputs) simultaneously.
 
 Error handling is achieved using the setjmp() and longjmp() commands. To the best of my knowledge these are available on nearly all platforms, including embedded, platforms so the library should run happily even on AVRs (this has not been tested).
 
 Licence: This code is free for non-commercial use and may not be redistributed without permission. Contact me for exceptions. This code is provided as-is, with no warranty whatsoever.
*/

#include<setjmp.h>
#include<stdlib.h>
#include<stdint.h>
#include<stdbool.h>

/**
 @brief maximum length for tokens in characters for expressions, define this in the compiler options to change the maximum size
*/
#if !defined(PARSER_MAX_TOKEN_SIZE)
#define PARSER_MAX_TOKEN_SIZE 256
#endif

/**
 @brief main data structure for the parser, holds a pointer to the input string and the index of the current position of the parser in the input
*/
typedef struct { 
	
	/** @brief input string to be parsed */
	const char *str; 
	
	/** @brief length of input string */
	uint32_t    len;
	
	/** @brief current parser position in the input */
	uint32_t    pos;
	
	/** @brief position to return to for exception handling */
	jmp_buf		err_jmp_buf;
	
	/** @brief error string to display, or query on failure */
	const char *error;
} parser_data;

/**
 @brief convenience function for using the library, handles initialization and destruction. basically just wraps parser_parse().
 @param[in] expr expression to parse
 @return expression value
 */
double parse_expression( const char *expr );

/**
 @brief primary public routine for the library
 @param[in] expr expression to parse
 @return expression value
 */
double parser_parse( parser_data *pd );

/**
 @brief allocates a new parser_data structure and initializes the member variables
 @param[in] str input string to be parsed
 @return parser_data structure if successful, or NULL on failure
 */
parser_data *parser_data_new( const char *str );

/**
 @brief frees a previously allocated parser_data structure
 @param[in] pd input parser_data structure to free
 */
void parser_data_free( parser_data *pd );

/**
 @brief error function for the parser, simply bails on the code
 @param[in] error string to print
 */
void parser_error( parser_data *pd, const char *err );

/**
 @brief looks at a input character, potentially offset from the current character, without consuming any
 @param[in] pd input parser_data structure to operate on
 @param[in] offset optional offset for character, relative to current character
 @return character that is offset characters from the current input
 */
char parser_peek( parser_data *pd );
	
/**
 @brief returns the current character, and advances the input position
 @param[in] pd input parser_data structure to operate on
 @return current character
 */
char parser_eat( parser_data *pd );
	
/**
 @brief voraciously consumes whitespace input until a non-whitespace character is reached
 @param[in] pd input parser_data structure to operate on
 */
void parser_eat_whitespace( parser_data *pd );

/**
 @brief reads and converts a double precision floating point value in one of the many forms,
 e.g. +1.0, -1.0, -1, +1, -1., 1., 0.5, .5, .5e10, .5e-2
 @param[in] pd input parser_data structure to operate on
 @return parsed value as double precision floating point number
 */
double parser_read_double( parser_data *pd );

/**
 @brief reads arguments for the builtin functions, auxilliary function for 
 parser_read_builtin()
 @param[in] pd input parser_data structure to operate upon
 @return value of the argument that was read
 */
double parser_read_argument( parser_data *pd ); 

/**
 @brief reads and calls built-in functions, like sqrt(.), pow(.), etc.
 @param[in] pd input parser_data structure to operate upon
 @return resulting value
*/
double parser_read_builtin( parser_data *pd );

/**
 @brief attempts to read an expression in parentheses, or failing that a literal value
 @param[in] pd input parser_data structure to operate upon
 @return expression/literal value
 */
double parser_read_paren( parser_data *pd );

/**
 @brief attempts to read an exponentiation operator, or failing that, a parenthetical expression 
 @param[in] pd input parser_data structure to operate upon
 @return exponentiation value
 */
double parser_read_power( parser_data *pd );
	
/**
 @brief reads a term in an expression
 @param[in] pd input parser_data structure to operate on
 @return value of the term
 */
double parser_read_term( parser_data *pd );

/**
 @brief attempts to read an expression
 @param[in] pd input parser_data structure
 @return expression value
 */
double parser_read_expr( parser_data *pd );
	
#endif