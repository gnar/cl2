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

#include "compiler/clcompiler.h"
#include "compiler/cllexer.h"
#include "compiler/clifunction.h"
#include "compiler/cliinstruction.h"

#include <stdarg.h>
#include <assert.h>

#include <iostream>
#include <iomanip>
#include <fstream>

CLCompiler::CLCompiler(CLLexer &lexer) : lexer(lexer), last_lineop(-1), l(TOK_ERROR), fp(0), stack_usage(0)
{ 
}

// static member
CLValue CLCompiler::compile(std::istream &input)
{
	CLLexer lexer(input);
	CLCompiler comp(lexer);
	return comp.compile();
}

// static member
CLValue CLCompiler::compile(const std::string &path)
{
	std::ifstream input(path.c_str());
	CLLexer lexer(input, path);
	CLCompiler comp(lexer);
	return comp.compile();
}

void CLCompiler::error(const char *type, const char *s, ...)
{
	char temp[256];
	va_list vl;
	va_start(vl, s);
	vsprintf(temp, s, vl);
	va_end(vl);

	char temp2[256];
	sprintf(temp2, "%s(%i): %s error: %s", lexer.getFile().c_str(), lexer.getLine(), type, temp);

	//TODO: Delete all generated CLIFunctions..

	throw CLParserException(temp2);
}

void CLCompiler::expect(CLToken tok)
{
	if (tok != l.tok)
	{
		error("parse", "Expected other token.");
	}
	lex();
}

void CLCompiler::lex()
{
	l = lexer.lex();
	if (l.tok == TOK_ERROR)
	{
		error("syntax", l.str.c_str());
	}
}

void CLCompiler::addLineOp(int line)
{
	int l = (line != -1)? line : lexer.getLine();
	if (last_lineop == l) return;
	fp->addInstruction(new CLIInstruction(OP_LINE, l));
	last_lineop = l;
}

void CLCompiler::addFileOp(const std::string &file)
{
	fp->addInstruction(new CLIInstruction(OP_FILE, file));
}

CLValue CLCompiler::compile()
{
	// read first token
	lex();

	// compile main function
	CLIFunction *main_fp = compileFunction(true);
	CLValue mainfunc = main_fp->generateFunction();
	delete main_fp;

	return mainfunc;
}

void CLCompiler::pushStringConstant(const std::string &str)
{
	fp->addInstruction(new CLIInstruction(OP_PUSHCONST, fp->addStringConstant(str)));
}

/////////////////////////////////////////////////////////////////////////////
// STATEMENTS                                                              //
/////////////////////////////////////////////////////////////////////////////

void CLCompiler::statement()
{
	switch (l.tok)
	{
		case ';': break;

		case TOK_LOCAL:	// local variable definition
			localStatement();
			break;

		case '{':
		{
			lex();
			fp->beginBlock();
			while (l.tok != '}')
			{
				statement();
			} 
			expect(CLToken('}'));
			fp->endBlock();
			break;
		}

		case TOK_RETURN:
		{
			lex();

			// clear used up stack
			if (stack_usage > 0)
			{
				fp->addInstruction(new CLIInstruction(OP_POP, stack_usage));
			}

			// push return result onto stack
			expect(CLToken('('));
			if (l.tok == ')') 
			{ 	// no return value: default to null
				fp->addInstruction(new CLIInstruction(OP_PUSH0));
			} else { // optional expression to return
				expressionExpr();
			}
			expect(CLToken(')'));

			// clear local variables....
			int in_scope = fp->getLocalsInScope();
			if (in_scope) fp->addInstruction(new CLIInstruction(OP_DELL, in_scope));

			// add return opcode
			fp->addInstruction(new CLIInstruction(OP_RET));
			break;
		}

		case TOK_YIELD:
			lex();
			expect(CLToken('('));

			if (l.tok == ')') 
			{ 	// no yield value: default to null
				fp->addInstruction(new CLIInstruction(OP_PUSH0));
			} else { // optional expression
				expressionExpr();
			}

			expect(CLToken(')'));

			fp->addInstruction(new CLIInstruction(OP_YIELD));
			break;

		case TOK_WHILE:
			whileStatement();
			break;

		case TOK_FOR:
			forStatement();
			break;

		case TOK_FOREACH:
			foreachStatement();
			break;

		case TOK_IF:
			ifStatement();	
			break;

		case TOK_BREAK:
			breakStatement();
			break;

		case TOK_SWITCH:
			switchStatement();
			break;

		// function statement (or expression statement beginning with TOK_FUNCTION!!)
		case TOK_FUNCTION:
			lex(); // accept TOK_FUNCTION
			if (l.tok == TOK_IDENTIFIER) // statement? (syntax: "function xyz(...) { ... }")
			{
				std::string func_id = l.str;
				lex(); // accept TOK_IDENTIFIER

				// compile function (argument list + body)
				CLIFunction *ifn = compileFunction(false);
				CLValue func = ifn->generateFunction();
				delete ifn;

				// add function object to parent function's constant list
				int f_id = fp->addConstant(func);

				// add function object to root table
				fp->addInstruction(new CLIInstruction(OP_PUSHROOT)); // push self table
				fp->addInstruction(new CLIInstruction(OP_PUSHS, func_id)); // push key
				fp->addInstruction(new CLIInstruction(OP_PUSHCONST, f_id)); // push function
				fp->addInstruction(new CLIInstruction(OP_TABSET)); // make table entry
				fp->addInstruction(new CLIInstruction(OP_POP, 1)); // discard tabset result

			} else { // expression statement? (syntax: "function(...) { ... }")
				functionExpr();
				suffixedExpr(SUF_EXPR);
				fp->addInstruction(new CLIInstruction(OP_POP, 1)); // discard result of expression
			}

			break;


#ifdef MINDBENDER_EXT
		case TOK_EVENT:
			eventStatement();
			break;
#endif


		default: // expression statement (discards result)
			expressionExpr(); 
			fp->addInstruction(new CLIInstruction(OP_POP, 1));
			break;
	}

	if (l.tok == ';') lex();
}

CLIFunction *CLCompiler::compileFunction(bool root)
{
	CLIFunction *old_fp = fp;
	CLIFunction *new_fp = new CLIFunction();
	fp = new_fp;

#ifdef DEBUG
	if (stack_usage != 0)
	{
		std::cout << "Internal compiler error!!!" << std::endl;
		assert(stack_usage == 0);
	}
#endif

	// add debug info
	addFileOp(lexer.getFile());

	if (root)
	{
		fp->beginBlock();
		while (l.tok != TOK_EOF)
		{
			statement();
		} 
		fp->endBlock();
		
	} else {
		fp->beginBlock();

		// Argument name list
		expect(CLToken('('));
		parameterList();
		expect(CLToken(')'));

		// Function body
		expect(CLToken('{'));
		while (l.tok != '}')
		{
			statement();
		} 
		expect(CLToken('}'));

		fp->endBlock();
	}

	if (fp->needReturnGuard())
	{
		fp->addInstruction(new CLIInstruction(OP_PUSH0));
		fp->addInstruction(new CLIInstruction(OP_RET));
	}

	fp = old_fp;
	return new_fp;
}

void CLCompiler::parameterList()
{
	if (l.tok == ')') return;
	for (;;)
	{
		if (l.tok != TOK_IDENTIFIER) error("parse", "expected parameter identifier in function header");
		fp->addParameter(l.str);
		lex();
		if (l.tok == ')') break;
		expect(CLToken(','));
	}
}

void CLCompiler::whileStatement()
{
	CLIInstruction *loop_start;
	CLIInstruction *loop_exit;
	CLIInstruction *loop_jump_tostart;
	CLIInstruction *loop_jump_toexit;

	loop_exit = new CLIInstruction(OP_NOP);
	loop_start = new CLIInstruction(OP_NOP);

	// while
	expect(TOK_WHILE);

	// '(' <expr> ')'
	expect(CLToken('('));	
	fp->addInstruction(loop_start);
	expressionExpr();
	expect(CLToken(')'));

	fp->addInstruction(loop_jump_toexit = new CLIInstruction(OP_JMPF, -1));

	// <statement>
	fp->beginBlock(loop_exit);
	statement();
	fp->endBlock();
	fp->addInstruction(loop_jump_tostart = new CLIInstruction(OP_JMP, -1));

	fp->addInstruction(loop_exit);

	loop_jump_tostart->jump_target = loop_start;
	loop_jump_toexit->jump_target = loop_exit;
}

void CLCompiler::forStatement()
{
	// for (a; b; c) d;
	//
	//             expr a
	//             pop 1
	// loop_start: nop                      = loop_start
	//             expr b
	//             jmpf loop_exit           = loop_jump_toexit
	//             jmp loop_begin           = loop_jump_tobegin
	// loop_incr:  nop                      = loop_incr
	//             expr c
	//             pop 1
        //             jmp loop_start           = loop_jump_tostart
	// loop_begin: nop                      = loop_begin
	//             stmt d
	//             jmp loop_incr            = loop_jump_toincr
	// loop_exit:  nop                      = loop_exit

	CLIInstruction *loop_start = new CLIInstruction(OP_NOP);
	CLIInstruction *loop_jump_toexit = new CLIInstruction(OP_JMPF, -1);
	CLIInstruction *loop_jump_tobegin = new CLIInstruction(OP_JMP, -1);
	CLIInstruction *loop_incr = new CLIInstruction(OP_NOP);
	CLIInstruction *loop_jump_tostart = new CLIInstruction(OP_JMP, -1);
	CLIInstruction *loop_begin = new CLIInstruction(OP_NOP);
	CLIInstruction *loop_jump_toincr = new CLIInstruction(OP_JMP, -1);
	CLIInstruction *loop_exit = new CLIInstruction(OP_NOP);

	loop_jump_toexit->jump_target = loop_exit;
	loop_jump_tobegin->jump_target = loop_begin;
	loop_jump_tostart->jump_target = loop_start;
	loop_jump_toincr->jump_target = loop_incr;

	// for (
	expect(TOK_FOR);
	expect(CLToken('('));

	// initializer expression
	if (l.tok != ';') 
	{
		expressionExpr(); // expr a
		fp->addInstruction(new CLIInstruction(OP_POP, 1));
	}
	expect(CLToken(';'));

	fp->addInstruction(loop_start);

	// loop condition
	if (l.tok != ';')
	{
		expressionExpr(); // expr b
		fp->addInstruction(loop_jump_toexit);
	} 
	expect(CLToken(';'));

	fp->addInstruction(loop_jump_tobegin);
	fp->addInstruction(loop_incr);
	
	// loop increment expression
	if (l.tok != ')')
	{
		expressionExpr(); // expr c
		fp->addInstruction(new CLIInstruction(OP_POP, 1));
	}
	expect(CLToken(')'));

	fp->addInstruction(loop_jump_tostart);
	fp->addInstruction(loop_begin);
	fp->beginBlock(loop_exit);
	statement();
	fp->endBlock();
	fp->addInstruction(loop_jump_toincr);
	fp->addInstruction(loop_exit);
}

void CLCompiler::foreachStatement()
{
	// foreach([key,]val in <expr>) <stmt>          key,val: Local variables; key optional
	//
	//             <expr>
	//             tabit
	// loop_start: dup 0
	//             jmpf exit          = loop_jmp_to_exit
	//             tabnext
	//             popl #key     or     pop 1
	//             popl #value
	//             <stmt>
	//             jmp loop_start     = loop_jmp_to_start
	// loop_exit:  pop 2

	CLIInstruction *loop_start = new CLIInstruction(OP_DUP, 0);
	CLIInstruction *loop_jmp_to_start = new CLIInstruction(OP_JMP, -1);
	CLIInstruction *loop_exit = new CLIInstruction(OP_POP, 2);
	CLIInstruction *loop_jmp_to_exit = new CLIInstruction(OP_JMPF, -1);
	loop_jmp_to_start->jump_target = loop_start;
	loop_jmp_to_exit->jump_target = loop_exit;

	// foreach (
	expect(TOK_FOREACH);
	expect(CLToken('('));

	int local1_id = -1;
	int local2_id = -1;

	// get variable names [key,]val
	if (l.tok != TOK_IDENTIFIER) error("parse", "Expected local variable identifier in foreach(..)");
	local1_id = fp->getLocal(l.str);
	if (local1_id < 0) error("parse", "Expected local variable identifier in foreach(..)");
	lex();

	bool use_key = false;
	if (l.tok == ',')
	{
		use_key = true;
		lex();
		if (l.tok != TOK_IDENTIFIER)
			error("parse", "Expected local variable identifier after ',' in foreach(..)");
		local2_id = fp->getLocal(l.str);
		if (local1_id < 0)
			error("parse", "Expected local variable identifier after ',' in foreach(..)");
		lex();
	}

	int key_id = use_key ? local1_id : -1;
	int val_id = use_key ? local2_id : local1_id;

	// 'in' <expr> ')'
	expect(TOK_IN);
	addLineOp(); //XXX
	expressionExpr();
	expect(CLToken(')'));
	
	fp->addInstruction(new CLIInstruction(OP_TABIT));
	fp->addInstruction(loop_start);
	fp->addInstruction(loop_jmp_to_exit);
	fp->addInstruction(new CLIInstruction(OP_TABNEXT));
	if (use_key)
		fp->addInstruction(new CLIInstruction(OP_POPL, key_id));
	else
		fp->addInstruction(new CLIInstruction(OP_POP, 1));
	fp->addInstruction(new CLIInstruction(OP_POPL, val_id));
	
	stack_usage += 2;
		fp->beginBlock(loop_exit);
		statement();
		fp->endBlock();
	stack_usage -= 2;

	fp->addInstruction(loop_jmp_to_start);
	fp->addInstruction(loop_exit);
}

void CLCompiler::ifStatement()
{
	CLIInstruction *jump_if_false;
	CLIInstruction *jump_toexit;
	CLIInstruction *else_begin;
	CLIInstruction *else_exit;
	CLIInstruction *if_exit;

	bool has_else = false;

	expect(TOK_IF);
	expect(CLToken('('));
	expressionExpr();
	expect(CLToken(')'));
	
	fp->addInstruction(jump_if_false = new CLIInstruction(OP_JMPF, -1));
	
	fp->beginBlock();
	statement();
	fp->endBlock();

	if (l.tok == TOK_ELSE) has_else = true;
	if (has_else)
	{
		fp->addInstruction(jump_toexit = new CLIInstruction(OP_JMP, -1));

		expect(TOK_ELSE);
		fp->addInstruction(else_begin = new CLIInstruction(OP_NOP));

		statement(); // else statement

		fp->addInstruction(else_exit = new CLIInstruction(OP_NOP));

		// connect jumps
		jump_if_false->jump_target = else_begin;
		jump_toexit->jump_target = else_exit;
	} else {
		fp->addInstruction(if_exit = new CLIInstruction(OP_NOP));

		// connect jumps
		jump_if_false->jump_target = if_exit;
	}

}

void CLCompiler::breakStatement()
{
	lex();

	int level = 1;
	int locals_to_pop;

	expect(CLToken('('));

	if (l.tok == TOK_INTEGER) // optional break level constant
	{
		level = l.integer;
		if (level < 1) error("parse", "invalid break level value");
		lex();
	}

	expect(CLToken(')'));
	
	CLIInstruction *jmp, *break_tgt = fp->getBreakTarget(locals_to_pop, level);
	if (!break_tgt)
	{
		error("parse", "break must be inside at least %i loop(s)", level);
	}

	if (locals_to_pop > 0) fp->addInstruction(new CLIInstruction(OP_DELL, locals_to_pop));
	fp->addInstruction(jmp = new CLIInstruction(OP_JMP, -1));
	jmp->jump_target = break_tgt;
}

void CLCompiler::localStatement()
{
	std::string var_name;

	expect(TOK_LOCAL);

	for (;;)
	{
		if (l.tok != TOK_IDENTIFIER) error("parser", "'local': expected variable identifier");
		var_name = l.str;

		fp->addLocal(var_name);
		fp->addInstruction(new CLIInstruction(OP_ADDL, 1));
		
		lex(); // accept TOK_IDENTIFIER

		if (l.tok == '=') // optional initializer?
		{
			lex();
			expressionExpr();
			int id = fp->getLocal(var_name);
			//assert(id != -1);
			fp->addInstruction(new CLIInstruction(OP_POPL, id));
		}

		if (l.tok != ',') break;
		expect(CLToken(','));
	} 
}

void CLCompiler::switchStatement()
{
	expect(TOK_SWITCH);

	// switch (<expr>)
	// {
	//   case (<expr1>) <stmt1>
	//   case (<expr2>) <stmt2>
	//   ...
	//   case (<exprN>) <stmtN>
	//   else <stmtE> 		// optional
	// }
	//
	// Opcodes:
	//   <expr>
	//   dup          |  
	//   <expr1>       |  
	//   cmp           | 
	//   jmpf next1    | N times
	//   <stm1>        |
	//   jmp done      /
	// next1: nop     /
	//   <stmtE>        // optional
	// done:
	//   pop 1

	// push <expr> on stack
	expect(CLToken('(')); expressionExpr(); expect(CLToken(')'));

	expect(CLToken('{'));

	CLIInstruction *nextI, *jmpI, *jmpDone;
	CLIInstruction *done = new CLIInstruction(OP_POP, 1); // done target 

	bool bdone = false;
	bool got_else = false;
	while (!bdone)
	{
		switch (l.tok)
		{
			case TOK_CASE:
				expect(TOK_CASE);
				if (got_else) error("parse", "in switch statement: 'case' after 'else' not allowed"); 
				nextI = new CLIInstruction(OP_NOP);
				fp->addInstruction(new CLIInstruction(OP_DUP, 0));
				expect(CLToken('(')); expressionExpr(); expect(CLToken(')'));
				fp->addInstruction(new CLIInstruction(OP_EQ));
				fp->addInstruction(jmpI = new CLIInstruction(OP_JMPF)); jmpI->jump_target = nextI;
				stack_usage += 1; fp->beginBlock(done); statement(); fp->endBlock(); stack_usage -= 1;
				fp->addInstruction(jmpDone = new CLIInstruction(OP_JMP)); jmpDone->jump_target = done;
				fp->addInstruction(nextI);
				break;

			case TOK_ELSE:
				expect(TOK_ELSE);
				got_else = true;
				stack_usage += 1; fp->beginBlock(done); statement(); fp->endBlock(); stack_usage -= 1;
				break;

			case CLToken('}'):
				expect(CLToken('}'));
				bdone = true;
				break;
	
			default:
				error("parse", "in switch statement: only 'case', 'else' allowed in switch statement");
				break;
		}
	}
	
	// discard <expr> result
	fp->addInstruction(done);
}

/////////////////////////////////////////////////////////////////////////////
// EXPRESSIONS                                                             //
/////////////////////////////////////////////////////////////////////////////

void CLCompiler::expressionExpr()
{
	logicalOrExpr();
}

void CLCompiler::logicalOrExpr()
{
	logicalAndExpr();
	for (;;) switch (l.tok)
	{
		case TOK_OR: lex(); logicalAndExpr(); fp->addInstruction(new CLIInstruction(OP_OR)); break;
		default: goto done;
	}
done:;
}

void CLCompiler::logicalAndExpr()
{
	bitwiseOrExpr();
	for (;;) switch (l.tok)
	{
		case TOK_AND: lex(); bitwiseOrExpr(); fp->addInstruction(new CLIInstruction(OP_AND)); break;
		default: goto done;
	}
done:;
}

void CLCompiler::bitwiseOrExpr()
{
	bitwiseXOrExpr();
	for (;;) switch (l.tok)
	{
		case '|': lex(); bitwiseXOrExpr(); fp->addInstruction(new CLIInstruction(OP_BITOR)); break;
		default: goto done;
	}
done:;	
}

void CLCompiler::bitwiseXOrExpr()
{
	bitwiseAndExpr();
	for (;;) switch (l.tok)
	{
		case '^': lex(); bitwiseAndExpr(); fp->addInstruction(new CLIInstruction(OP_BITXOR)); break;
		default: goto done;
	}
done:;	
}

void CLCompiler::bitwiseAndExpr()
{
	comparisonExpr();
	for (;;) switch (l.tok)
	{
		case '&': lex(); comparisonExpr(); fp->addInstruction(new CLIInstruction(OP_BITAND)); break;
		default: goto done;
	}
done:;
}

void CLCompiler::comparisonExpr()
{
	shiftExpr();
	for (;;) switch (l.tok)
	{
		case TOK_EQ: lex(); shiftExpr(); fp->addInstruction(new CLIInstruction(OP_EQ)); break;	
		case TOK_NEQ: lex(); shiftExpr(); fp->addInstruction(new CLIInstruction(OP_NEQ)); break;
		case '<': lex(); shiftExpr(); fp->addInstruction(new CLIInstruction(OP_LT)); break;
		case '>': lex(); shiftExpr(); fp->addInstruction(new CLIInstruction(OP_GT)); break;
		case TOK_LE: lex(); shiftExpr(); fp->addInstruction(new CLIInstruction(OP_LE)); break;
		case TOK_GE: lex(); shiftExpr(); fp->addInstruction(new CLIInstruction(OP_GE)); break;
		default: goto done;
	}
done:;
}

void CLCompiler::shiftExpr()
{
	plusExpr();
	for (;;) switch (l.tok)
	{
		case TOK_SHL: lex(); plusExpr(); fp->addInstruction(new CLIInstruction(OP_SHL)); break;
		case TOK_SHR: lex(); plusExpr(); fp->addInstruction(new CLIInstruction(OP_SHR)); break;
		default: goto done;
	}
done:;
}

void CLCompiler::plusExpr()
{
	termExpr();
	for (;;) switch (l.tok)
	{
		case '+': lex(); termExpr(); fp->addInstruction(new CLIInstruction(OP_ADD)); break;
		case '-': lex(); termExpr(); fp->addInstruction(new CLIInstruction(OP_SUB)); break;
		default: goto done;
	}
done:;
}

void CLCompiler::termExpr()
{
	factorExpr();
	for (;;) switch (l.tok)
	{
		case '*': lex(); factorExpr(); fp->addInstruction(new CLIInstruction(OP_MUL)); break;
		case '/': lex(); factorExpr(); fp->addInstruction(new CLIInstruction(OP_DIV)); break;
		case '%': lex(); factorExpr(); fp->addInstruction(new CLIInstruction(OP_MODULO)); break;
		default: goto done;
	}
done:;
}

void CLCompiler::factorExpr()
{
	switch (l.tok)
	{
		case TOK_IDENTIFIER:
		{
			int id; 
			id = fp->getLocal(l.str);

			if (id != -1) // Identifier is either a local variable name..
			{
				lex();
				suffixedExpr(SUF_LOCAL, id);
			} else { // ..or it is a global variable
				fp->addInstruction(new CLIInstruction(OP_PUSHROOT));
				fp->addInstruction(new CLIInstruction(OP_PUSHCONST, fp->addStringConstant(l.str)));

				lex();
				suffixedExpr(SUF_TABLE);
			}
			break;
		}

		case TOK_SELF:
			lex();
			fp->addInstruction(new CLIInstruction(OP_PUSHSELF));
			suffixedExpr(SUF_EXPR);
			break;

		case TOK_ROOT:
			lex();
			fp->addInstruction(new CLIInstruction(OP_PUSHROOT));
			suffixedExpr(SUF_EXPR);
			break;


		case '$':
			fp->addInstruction(new CLIInstruction(OP_PUSHSELF));
			lex();
			if (l.tok == TOK_IDENTIFIER)
			{
				pushStringConstant(l.str); lex();
				suffixedExpr(SUF_TABLE);
			} else {
				error("parse", "expected identifier after '$'");
			}
			break;

		case TOK_INTEGER:
			fp->addInstruction(new CLIInstruction(OP_PUSHI, l.integer));
			lex(); break;

		case TOK_FLOAT:
			fp->addInstruction(new CLIInstruction(OP_PUSHF, l.real));
			lex(); break;

		case TOK_STRING:
			pushStringConstant(l.str); lex();
			suffixedExpr(SUF_EXPR);
			break;

		case TOK_FALSE:
		case TOK_NULL:
			fp->addInstruction(new CLIInstruction(OP_PUSH0));
			lex(); break;

		case TOK_TRUE:
			fp->addInstruction(new CLIInstruction(OP_PUSHI, 1));
			lex(); break;

		case '-': // unary minus
			lex(); factorExpr();
			fp->addInstruction(new CLIInstruction(OP_NEG));
			break;

		case TOK_NOT: // unary not (boolean)
			lex(); factorExpr();
			fp->addInstruction(new CLIInstruction(OP_NOT));
			break;

		case TOK_CLONE:
			lex(); 
			factorExpr();
			fp->addInstruction(new CLIInstruction(OP_CLONE));
			suffixedExpr(SUF_EXPR);
			break;

		case '(':
			lex();
			expressionExpr();
			expect(CLToken(')'));
			suffixedExpr(SUF_EXPR);
			break;

		case TOK_ARRAY: // array construction
			lex();
			arrayConstructorExpr();
			suffixedExpr(SUF_EXPR);
			break;
	
		case TOK_TABLE: // table construction
			lex();
			// fall-through
		case '[': // table construction (without optional table keyword)
			tableConstructorExpr();
			suffixedExpr(SUF_EXPR);
			break;

		case TOK_EXTERNAL: // syntax: <expr> = external function <identifier>
		{
			lex();
			expect(TOK_FUNCTION);
			if (l.tok != TOK_IDENTIFIER) error("parse", "Expected identifier after 'external function'");
			fp->addInstruction(new CLIInstruction(OP_PUSHEXTFUNC, l.str));
			lex();
			suffixedExpr(SUF_EXPR);
			break;	
		}

		case TOK_FUNCTION:
		{
			lex(); // accept TOK_FUNCTION
			functionExpr();
			suffixedExpr(SUF_EXPR);
			break;
		}

		default:
			error("parse", "Expected value/identifier or '( expression )'");
	}
}

void CLCompiler::functionExpr() // function expression (with TOK_FUNCTION already accepted)
{
	// compile function (argument list + body)
	CLIFunction *ifn = compileFunction(false);
	CLValue func = ifn->generateFunction();
	delete ifn;

	// add function object to parent function's constant list
	int f_id = fp->addConstant(func);

	// push this constant onto the stack
	fp->addInstruction(new CLIInstruction(OP_PUSHCONST, f_id));
}

void CLCompiler::suffixedExpr(Suffixed suf, int lid)
{
	addLineOp();

	if (l.tok == '=') // assignment ?
	{
		lex();
		expressionExpr();
		switch (suf)
		{
			case SUF_EXPR: error("parse", "Can't assign to this expression (no lvalue)"); break;
			case SUF_TABLE: fp->addInstruction(new CLIInstruction(OP_TABSET)); break;
			case SUF_LOCAL: 
				fp->addInstruction(new CLIInstruction(OP_DUP, 0));
				fp->addInstruction(new CLIInstruction(OP_POPL, lid)); 
				break;
			default: assert(0);
		}
		return;
	}

	// no assignment.

#define TOSTACK \
	switch (suf) \
	{ \
		case SUF_TABLE: fp->addInstruction(new CLIInstruction(OP_TABGET)); break; \
		case SUF_LOCAL: fp->addInstruction(new CLIInstruction(OP_PUSHL, lid)); break; \
		case SUF_EXPR: break; /* already on stack.. */ \
		default: assert(0); \
	};

	switch (l.tok)
	{
		case '[': // table lookup?
			TOSTACK;
			lex();
			expressionExpr();
			expect(CLToken(']'));
			suffixedExpr(SUF_TABLE);
			break;

		case '.': // table lookup via identifier as string constant 
			TOSTACK;
			lex();
			if (l.tok == TOK_IDENTIFIER)
			{
				pushStringConstant(l.str); lex();
				suffixedExpr(SUF_TABLE);
			} else {
				error("parse", "Expected identifier after '.'"); break;
			}
			break;

		case '(': // Member function call
		{	
			//addLineOp();
			lex();

			if (suf != SUF_TABLE)
			{
				// no table specified? use current object as table (self)
				TOSTACK;
				fp->addInstruction(new CLIInstruction(OP_PUSHSELF));
			} else {
				// use given table
				fp->addInstruction(new CLIInstruction(OP_TABGET2)); // tabget2!!
			}

			int argc = argumentList(); // put each argument on stack
			expect(CLToken(')'));
			fp->addInstruction(new CLIInstruction(OP_PUSHI, argc)); // put argc 

			// STACK CONTENTS:
			//        function
			//        self
			//        args[]
			// top -> argc

			fp->addInstruction(new CLIInstruction(OP_MCALL));
			addFileOp(lexer.getFile());
			suffixedExpr(SUF_EXPR);
			break; 
		}

		default: TOSTACK; break;
	}
#undef TOSTACK
}

int CLCompiler::argumentList()
{
	int argc = 0;

	if (l.tok == ')') return 0;

	for (;;)
	{
		expressionExpr(); ++argc;
		if (l.tok == ')') break;
		expect(CLToken(','));
	}

	return argc;
}

void CLCompiler::tableConstructorExpr()
{
	expect(CLToken('['));

	fp->addInstruction(new CLIInstruction(OP_NEWTABLE));

	while (l.tok != CLToken(']'))
	{
		switch (l.tok)
		{
			case TOK_IDENTIFIER:
			{
				fp->addInstruction(new CLIInstruction(OP_DUP, 0));
				pushStringConstant(l.str); 
				lex(); // TOK_IDENTIFIER
				expect(CLToken('='));
				expressionExpr();
				fp->addInstruction(new CLIInstruction(OP_TABSET));
				fp->addInstruction(new CLIInstruction(OP_POP, 1));
				break;
			}

			case '[':
			{
				lex(); // '['
				fp->addInstruction(new CLIInstruction(OP_DUP, 0));
				expressionExpr(); // key expr
				expect(CLToken(']'));
				expect(CLToken('='));
				expressionExpr(); // value expr
				fp->addInstruction(new CLIInstruction(OP_TABSET));
				fp->addInstruction(new CLIInstruction(OP_POP, 1));
				break;
			}

			// syntax sugar for member functions
			case TOK_FUNCTION:
			{
				lex(); // accept TOK_FUNCTION
				if (l.tok != TOK_IDENTIFIER) error("parse", "need identifer after 'function' in table constructor");
				std::string func_id = l.str;
				lex(); // accept TOK_IDENTIFIER

				// compile function (argument list + body)
				CLIFunction *ifn = compileFunction(false);
				CLValue func = ifn->generateFunction();
				delete ifn;

				// add function object to parent function's constant list
				int f_id = fp->addConstant(func);

				fp->addInstruction(new CLIInstruction(OP_DUP, 0)); // push table 
				fp->addInstruction(new CLIInstruction(OP_PUSHS, func_id)); // push function id/name
				fp->addInstruction(new CLIInstruction(OP_PUSHCONST, f_id)); // push function onto stack
				fp->addInstruction(new CLIInstruction(OP_TABSET));
				fp->addInstruction(new CLIInstruction(OP_POP, 1)); // discard tabset result
				break;
			}

			default: error("parse", "Expected key identifier or function keyword in table constructor");
		}

		if (l.tok == CLToken(',')) lex(); // optional comma
	}
	expect(CLToken(']'));
}

void CLCompiler::arrayConstructorExpr()
{
	expect(CLToken('['));

	fp->addInstruction(new CLIInstruction(OP_NEWARRAY));

	int idx = 0;
	bool skipped_comma = false;

	while (l.tok != CLToken(']'))
	{
		if (skipped_comma) error("parse", "Expected comma after expression in array constructor");

		fp->addInstruction(new CLIInstruction(OP_DUP, 0));
		fp->addInstruction(new CLIInstruction(OP_PUSHI, idx++));
		expressionExpr();
		fp->addInstruction(new CLIInstruction(OP_TABSET));
		fp->addInstruction(new CLIInstruction(OP_POP, 1));
		
		skipped_comma = true;
		if (l.tok == ',')
		{
			lex();
			skipped_comma = false;
		}
	}
	
	expect(CLToken(']'));
}

#ifdef MINDBENDER_EXT

static std::string getDefaultArgumentName(int s)
{
	char result[128];
	sprintf(result, "__argname%i", s);
	return std::string(result);
}

void CLCompiler::eventStatement()
{
	// 'event' <identifier> ':' <expr> ['(' <identifier> ')'] [ ',' ... ]    '{' <statements> '}'
	//
	// Ex.: event scumm give: ego, ANY(receiver), ... { <blah> }
	//
	// -> syntactic sugar for:
	//
	// adv.AddEvent("scumm", "give", ego, ANY, function(__dummy0, receiver) { <blah> });
	//
	//
	// pushroot
	// pushstring "adv"
	// tabget
	// pushstring "AddEvent"
	// tabget				// on stack: function to call (1)
	// pushroot				// on stack: self value of function to call (2)
	// pushstring "scumm"			// on stack: first argument (3a)
	// pushstring "give"			// on stack: second argument (3b)
	// "push <ego>"				// on stack: result of expression 'ego' // (4)
	// "push <ANY>"				// on stack: result of expression 'ANY' // (4)
	// pushfunc #				// on stack: event function // (5)
	// pushi 5				// on stack: number of arguments (3 + 2) // (6)
	// mcall				// calls function at adv.AddEvent
	// pop 1				// discard result of AddEvent function call

	addLineOp();

	expect(TOK_EVENT);
	fp->addInstruction(new CLIInstruction(OP_PUSHROOT));
	pushStringConstant("adv");
	fp->addInstruction(new CLIInstruction(OP_TABGET));
	pushStringConstant("AddEvent");
	fp->addInstruction(new CLIInstruction(OP_TABGET)); // (1)
	fp->addInstruction(new CLIInstruction(OP_PUSHROOT)); // (2)

	// get event class&name string
	if (l.tok != TOK_IDENTIFIER) error("parser", "Event statement: expected event class");
	std::string event_class = l.str;
	lex();

	if (l.tok != TOK_IDENTIFIER) error("parser", "Event statement: expected event event");
	std::string event_name = l.str;
	lex();

	pushStringConstant(event_class); // (3a)
	pushStringConstant(event_name); // (3b)

	expect(CLToken(':'));

	// new event function
	CLIFunction *event_fn = new CLIFunction();
	event_fn->beginBlock();
	
	// parse parameter list
	int argc = 0;
	bool first_arg = true;
	while (l.tok != CLToken('{'))
	{
		if (!first_arg) expect(CLToken(','));

		expressionExpr(); // (4)
		std::string arg_name = getDefaultArgumentName(argc);
		
		if (l.tok == ':')
		{
			lex();
			if (l.tok != TOK_IDENTIFIER) error("parser", "Event statement: expected identifier for event argument: arg:<identifier>");
			arg_name = l.str; lex();
		}

		event_fn->addParameter(arg_name);
		++argc;

		first_arg = false;
	}

	{	// compile event function into event_fn
		CLIFunction *old_fp = fp;
		fp = event_fn;

		// add debug info
		addFileOp(lexer.getFile());

		expect(CLToken('{'));
		while (l.tok != '}')
		{
			statement();
		} 
		fp->endBlock();
		expect(CLToken('}'));

		if (fp->needReturnGuard())
		{
			fp->addInstruction(new CLIInstruction(OP_PUSH0));
			fp->addInstruction(new CLIInstruction(OP_RET));
		}
		fp = old_fp;
	}	// done

	CLValue func = event_fn->generateFunction();
	delete event_fn;
	int f_id = fp->addConstant(func);
	fp->addInstruction(new CLIInstruction(OP_PUSHCONST, f_id));
	
	fp->addInstruction(new CLIInstruction(OP_PUSHI, argc+3)); // (6)
	fp->addInstruction(new CLIInstruction(OP_MCALL)); 
	addFileOp(lexer.getFile());
	fp->addInstruction(new CLIInstruction(OP_POP, 1)); 
}

#endif

