#include <math.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include <common.h>
#include <protocols/expression_parser.hpp>

double parse_expression( const char *expr ){
	double val;
	parser_data *pd = parser_data_new( expr );
	val = parser_parse( pd );
	
	// if pd->error is non-NULL, an error occured, print out the error string
	if( pd->error ){
		print(log_error,"Error: %s\n","Expression Parser", pd->error );
		print(log_error,"Expression failed to parse, returning nan", "Expression Parser");
	}
	parser_data_free( pd );
	return val;
}

double parser_parse( parser_data *pd ){
	// set the jump position and launch the parser
	if( !setjmp( pd->err_jmp_buf ) ){
		return parser_read_expr( pd );
	} else {
		// error was returned, output a nan silently
		return sqrt( -1.0 );
	}
}

parser_data *parser_data_new( const char *str ){
	parser_data *pd = (parser_data *)malloc( sizeof( parser_data ) );
	if( !pd ) return NULL;
	pd->str = str;
	pd->len = strlen( str )+1;
	pd->pos = 0;
	pd->error = NULL;
	return pd;
}

void parser_data_free( parser_data *pd ){
	free( pd );
}

void parser_error( parser_data *pd, const char *err ){
	pd->error = err;
	longjmp( pd->err_jmp_buf, 1);
}

char parser_peek( parser_data *pd ){
	if( pd->pos < pd->len )
		return pd->str[pd->pos];
	parser_error( pd, "Tried to read past end of string!" );
	return '\0';
}

char parser_eat( parser_data *pd ){
	if( pd->pos < pd->len )
		return pd->str[pd->pos++];
	parser_error( pd, "Tried to read past end of string!" );
	return '\0';
}

void parser_eat_whitespace( parser_data *pd ){
	while( isspace( parser_peek( pd ) ) )
		parser_eat( pd );
}

double parser_read_double( parser_data *pd ){
	char c, token[PARSER_MAX_TOKEN_SIZE];
	uint16_t pos=0;
	
	// read a leading sign
	c = parser_peek( pd );
	if( c == '+' || c == '-' )
		token[pos++] = parser_eat( pd );
	
	// read optional digits leading the decimal point
	while( isdigit(parser_peek(pd)) )
		token[pos++] = parser_eat( pd );
	
	// read the optional decimal point
	c = parser_peek( pd );
	if( c == '.' )
		token[pos++] = parser_eat( pd );
	
	// read optional digits after the decimal point
	while( isdigit(parser_peek(pd)) )
		token[pos++] = parser_eat( pd );
	
	// read the exponent delimiter
	c = parser_peek( pd );
	if( c == 'e' || c == 'E' ){
		token[pos++] = parser_eat( pd );
		
		// check if the expoentn has a sign,
		// if so, read it 
		c = parser_peek( pd );
		if( c == '+' || c == '-' ){
			token[pos++] = parser_eat( pd );
		}
	}
	
	// read the exponent delimiter
	while( isdigit(parser_peek(pd) ) )
		token[pos++] = parser_eat( pd );
	
	// remove any trailing whitespace
	parser_eat_whitespace( pd );
	
	// null-terminate the string and return 
	// the converted result as a double
	token[pos] = '\0';
	return atof( token );
}

double parser_read_argument( parser_data *pd ){
	char c;
	double val;
	// eat leading whitespace
	parser_eat_whitespace( pd );
	
	// read the argument
	val = parser_read_expr( pd );
	
	// read trailing whitespace
	parser_eat_whitespace( pd );
	
	// check if there's a comma
	c = parser_peek( pd );
	if( c == ',' )
		parser_eat( pd );
	
	// eat trailing whitespace
	parser_eat_whitespace( pd );
	
	// return result
	return val;
}

double parser_read_builtin( parser_data *pd ){
	double v0, v1;
	char c, token[PARSER_MAX_TOKEN_SIZE];
	int pos=0;
	
	c = parser_peek( pd );
	if( isalpha(c) || c == '_' ){
		while( isalpha(c) || isdigit(c) || c == '-' ){
			token[pos++] = parser_eat( pd );
			c = parser_peek( pd );
		}
		token[pos] = '\0';
		
		// eat opening bracket
		if( parser_eat(pd) != '(' )
			parser_error( pd, "Expected '(' in builtin call!" );
		
		// start handling the specific builtin functions
		if( strcmp( token, "pow" ) == 0 ){
			v0 = parser_read_argument( pd );
			v1 = parser_read_argument( pd );
			v0 = pow( v0, v1 );
		} else if( strcmp( token, "sqrt" ) == 0 ){
			v0 = parser_read_argument( pd );
			if( v0 < 0.0 ) 
				parser_error( pd, "sqrt(x) undefined for x < 0!" );
			v0 = sqrt( v0 );
		} else if( strcmp( token, "log" ) == 0 ){
			v0 = parser_read_argument( pd );
			if( v0 <= 0 )
				parser_error( pd, "log(x) undefined for x <= 0!" );
			v0 = log( v0 );
		} else if( strcmp( token, "exp" ) == 0 ){
			v0 = parser_read_argument( pd );
			v0 = exp( v0 );
		} else if( strcmp( token, "sin" ) == 0 ){
			v0 = parser_read_argument( pd );	
			v0 = sin( v0 );
		} else if( strcmp( token, "asin" ) == 0 ){
			v0 = parser_read_argument( pd );
			if( fabs(v0) > 1.0 )
				parser_error( pd, "asin(x) undefined for |x| > 1!" );
			v0 = asin( v0 );
		} else if( strcmp( token, "cos" ) == 0 ){
			v0 = parser_read_argument( pd );
			if( fabs(v0 ) > 1.0 )
				parser_error( pd, "acos(x) undefined for |x| > 1!" );
			v0 = cos( v0 );
		} else if( strcmp( token, "acos" ) == 0 ){
			v0 = parser_read_argument( pd );
			v0 = acos( v0 );
		} else if( strcmp( token, "tan" ) == 0 ){
			v0 = parser_read_argument( pd );	
			v0 = tan( v0 );
		} else if( strcmp( token, "atan" ) == 0 ){
			v0 = parser_read_argument( pd );
			v0 = atan( v0 );
		} else if( strcmp( token, "atan2" ) == 0 ){
			v0 = parser_read_argument( pd );
			v1 = parser_read_argument( pd );
			v0 = atan2( v0, v1 );
		} else if( strcmp( token, "abs" ) == 0 ){
			v0 = parser_read_argument( pd );
			v0 = abs( v0 );
		} else if( strcmp( token, "fabs" ) == 0 ){
			v0 = parser_read_argument( pd );
			v0 = fabs( v0 );
		} else if( strcmp( token, "floor" ) == 0 ){
			v0 = parser_read_argument( pd );
			v0 = floor( v0 );
		} else if( strcmp( token, "ceil" ) == 0 ){
			v0 = parser_read_argument( pd );
			v0 = floor( v0 );
		} else if( strcmp( token, "round" ) == 0 ){
			v0 = parser_read_argument( pd );
			v0 = round( v0 );
		} else {
			parser_error( pd, "Tried to call unknown builtin function!" );
		}
		
		// eat closing bracket of function call
		if( parser_eat( pd ) != ')' )
			parser_error( pd, "Expected ')' in builtin call!" );
		
	} else {
		// not a builtin function call, just read a literal double
		v0 = parser_read_double( pd );
	}
	
	// consume whitespace
	parser_eat_whitespace( pd );
	
	// return the value
	return v0;
}

double parser_read_paren( parser_data *pd ){
	double val;
	
	// check if the expression has a parenthesis
	if( parser_peek( pd ) == '(' ){
		// eat the character
		parser_eat( pd );
		
		// eat remaining whitespace
		parser_eat_whitespace( pd );
		
		// if there is a parenthesis, read it 
		// and then read an expression, then
		// match the closing brace
		val = parser_read_expr( pd );
		
		// consume remaining whitespace
		parser_eat_whitespace( pd );
		
		// match the closing brace
		if( parser_peek(pd) != ')' )
			parser_error( pd, "Expected ')'!" );		
		parser_eat(pd);
	} else {
		// otherwise just read a literal value
		val = parser_read_builtin( pd );
	}
	// eat following whitespace
	parser_eat_whitespace( pd );
	
	// return the result
	return val;
}

double parser_read_power( parser_data *pd ){
	double v0, v1=1.0, s=1.0;
	
	// read the first operand
	v0 = parser_read_paren( pd );
	
	// eat remaining whitespace
	parser_eat_whitespace( pd );
	
	// attempt to read the exponentiation operator
	while( parser_peek(pd) == '^' ){
		parser_eat(pd );
		
		// eat remaining whitespace
		parser_eat_whitespace( pd );
		
		// handles case of a negative immediately 
		// following exponentiation but leading
		// the parenthetical exponent
		if( parser_peek( pd ) == '-' ){
			parser_eat( pd );
			s = -1.0;
			parser_eat_whitespace( pd );
		}
		
		// read the second operand
		v1 = s*parser_read_power( pd );
		
		// perform the exponentiation
		v0 = pow( v0, v1 );
		
		// eat remaining whitespace
		parser_eat_whitespace( pd );
	}
	
	// return the result
	return v0;
}

double parser_read_term( parser_data *pd ){
	double v0;
	char c;
	
	// read the first operand
	v0 = parser_read_power( pd );
	
	// eat remaining whitespace
	parser_eat_whitespace( pd );
	
	// check to see if the next character is a
	// multiplication or division operand
	c = parser_peek( pd );
	while( c == '*' || c == '/' ){
		// eat the character
		parser_eat( pd );
		
		// eat remaining whitespace
		parser_eat_whitespace( pd );
		
		// perform the appropriate operation
		if( c == '*' ){
			v0 *= parser_read_power( pd );
		} else if( c == '/' ){
			v0 /= parser_read_power( pd );
		}
		
		// eat remaining whitespace
		parser_eat_whitespace( pd );
		
		// update the character
		c = parser_peek( pd );
	}
	return v0;
}

double parser_read_expr( parser_data *pd ){
	double v0 = 0.0;
	char c;
	
	// handle unary minus
	c = parser_peek( pd );
	if( c == '+' || c == '-' ){
		parser_eat( pd );
		parser_eat_whitespace( pd );
		if( c == '+' )
			v0 += parser_read_term( pd );
		else if( c == '-' )
			v0 -= parser_read_term( pd );
	} else {
		v0 = parser_read_term( pd );
	}
	parser_eat_whitespace( pd );
	
	// check if there is an addition or
	// subtraction operation following
	c = parser_peek( pd );
	while( c == '+' || c == '-' ){
		// advance the input
		parser_eat( pd );
		
		// eat any extra whitespace
		parser_eat_whitespace( pd );
		
		// perform the operation
		if( c == '+' ){		
			v0 += parser_read_term( pd );
		} else if( c == '-' ){
			v0 -= parser_read_term( pd );
		}
		
		// eat whitespace
		parser_eat_whitespace( pd );
		
		// update the character being tested in the while loop
		c = parser_peek( pd );
	}
	
	// return expression result
	return v0;
}
