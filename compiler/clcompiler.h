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


#ifndef CLCOMPILER_H
#define CLCOMPILER_H

#include "compiler/cllexer.h"
#include "value/clvalue.h"
#include "clopcode.h"

#include <stdexcept>
#include <fstream>

class CLParserException : public std::runtime_error
{
public:
	CLParserException(const char *what) : std::runtime_error(what) {}
};

class CLIFunction;

class CLCompiler
{
public:
	CLCompiler(CLLexer &lexer);
	
	CLValue compile();

	static CLValue compile(const std::string &path);
	static CLValue compile(std::istream &input);

private:
	CLLexer &lexer;

	void statement();
	CLIFunction *compileFunction(bool root = false);
	void parameterList();

	void localStatement();

	void whileStatement();
	void forStatement();
	void foreachStatement();
	void ifStatement();
	void breakStatement();
	void switchStatement();

#ifdef MINDBENDER_EXT
	void eventStatement();
#endif

	void expressionExpr();
	void logicalOrExpr();
	void logicalAndExpr();
	void bitwiseOrExpr();
	void bitwiseXOrExpr();
	void bitwiseAndExpr();
	void comparisonExpr();
	void shiftExpr();
	void plusExpr();
	void termExpr();
	void factorExpr();

	void tableConstructorExpr();
	void arrayConstructorExpr();

	// function expression (with TOK_FUNCTION already accepted)
	void functionExpr();

	void addLineOp(int line = -1);
	int last_lineop; 

	void addFileOp(const std::string &file);

	enum Suffixed // any expression, which might be followed by:    = [ ( .    (except "arithmetic" parenthesis)
	{
		SUF_TABLE, // a table (2 values on stack: tab|key pair)
		SUF_LOCAL, // a local variable (number given as 'lid' parameter in suffixedExpr below..)
		SUF_EXPR,  // any other expression on stack (e.g. self, global, (<expr>) ) 
	};

	void suffixedExpr(Suffixed suf, int lid = -1);
	int argumentList();

	void lex();	// load next lexeme into 'l'
	void expect(CLToken tok);

	CLLexeme l;		// current unprocessed lexeme
	class CLIFunction *fp;	// current function
	int stack_usage;        // current stack usage _between_ statements, so that return statements can clean the
                                // stack accordingly before OP_RET.

	void pushStringConstant(const std::string &str);

	void error(const char *type, const char *s, ...); // throws a CLParserException
};

#endif

