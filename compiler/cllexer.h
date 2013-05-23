/*
    This file is part of the CL2 script language interpreter.

    Gunnar Selke <gunnar@gmx.info>

    CL2 is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    CL2 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with CL2; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#ifndef CLLEXER_H
#define CLLEXER_H

#include <iostream>
#include <string>

enum CLToken
{
	TOK_ERROR=-1,

	TOK_EOF=256,
	TOK_INTEGER,
	TOK_FLOAT,
	TOK_STRING,
	TOK_IDENTIFIER,

	TOK_EQ,		// ==
	TOK_NEQ,	// !=
	TOK_GE,		// >=
	TOK_LE,		// <=
	TOK_SHL,	// <<
	TOK_SHR,	// >>
	TOK_AND,	// and
	TOK_OR,		// or
	TOK_NOT,	// not (unary)

	TOK_FOR,	// for
	TOK_IF,		// if
	TOK_ELSE,	// else
	TOK_WHILE,	// while
	TOK_DO,		// do
	TOK_BREAK,      // break
	TOK_FOREACH,    // foreach
	TOK_IN,         // in

	TOK_NULL,	// null
	TOK_TRUE, 	// true
	TOK_FALSE,	// false
	TOK_SELF,	// self
	TOK_ROOT,	// root
	TOK_DOUBLECOLON,// ::

	TOK_LOCAL,	// local

	TOK_FUNCTION,	// function
	TOK_RETURN,	// return
	TOK_YIELD,	// yield

	TOK_EXTERNAL,	// external
	
	TOK_ARRAY,	// array
	TOK_TABLE,	// table

	TOK_CLONE,	// clone

	TOK_SWITCH,	// switch
	TOK_CASE,	// case

#ifdef MINDBENDER_EXT
	TOK_EVENT	// event
#endif
};

struct CLLexeme
{
	CLLexeme(CLToken t) : tok(t) {}
	
	CLLexeme(char t)
	{
		tok = (CLToken)t;
	}

	CLToken tok;
	std::string str;
	int integer;
	float real;
};

class CLLexer
{
public:
	CLLexer(std::istream &input, const std::string &filename = "<input>");

	CLLexeme lex();  // return next token 

	int getLine() { return lineno; }
	const std::string getFile() { return filename; }

private:
	std::istream &input;
	std::string filename;
	int lineno; // line position in file, line position of last returned token
	char tmpstr[2048];

	char ch; // current char
	bool eof;
	bool err;
	void next();
	
	bool eatComment();
	CLLexeme readNumber();
	CLLexeme readString();
	CLLexeme readKeywordOrIdentifier();
	CLLexeme error(const char *reason);
};

#endif

