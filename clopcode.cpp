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


#include "clopcode.h"

#include <assert.h>

static CLOpcodeDesc opdesc[] =
{
	{OP_NOP, "nop", ARG_NONE},

	{OP_PUSH0, "push0", ARG_NONE},
	{OP_PUSHROOT, "pushroot", ARG_NONE},
	{OP_PUSHSELF, "pushself", ARG_NONE},
	{OP_PUSHCONST, "pushconst", ARG_INTEGER},
	{OP_PUSHEXTFUNC, "pushextfunc", ARG_STRING},
	{OP_PUSHI, "pushi", ARG_INTEGER},
	{OP_PUSHF, "pushf", ARG_FLOAT},
	{OP_PUSHS, "pushs", ARG_STRING},
	{OP_POP, "pop", ARG_INTEGER},

	{OP_DUP, "dup", ARG_INTEGER},

	// tables
	{OP_TABGET, "tabget", ARG_NONE},
	{OP_TABGET2, "tabget2", ARG_NONE},
	{OP_TABSET, "tabset", ARG_NONE},
	{OP_NEWTABLE, "newtable", ARG_NONE},
	{OP_NEWARRAY, "newarray", ARG_NONE},

	{OP_TABIT, "tabit", ARG_NONE},
	{OP_TABNEXT, "tabnext", ARG_NONE},

	{OP_CLONE, "clone", ARG_NONE},

	// local variables..
	{OP_PUSHL, "pushl", ARG_INTEGER},
	{OP_POPL, "popl", ARG_INTEGER},
	{OP_ADDL, "addl", ARG_INTEGER},
	{OP_DELL, "dell", ARG_INTEGER},

	// arithmetic
	{OP_ADD, "add", ARG_NONE},
	{OP_SUB, "sub", ARG_NONE},
	{OP_MUL, "mul", ARG_NONE},
	{OP_DIV, "div", ARG_NONE},
	{OP_MODULO, "modulo", ARG_NONE},
	{OP_NEG, "neg", ARG_NONE},

	// bitwise
	{OP_BITOR, "bitor", ARG_NONE},
	{OP_BITAND, "bitand", ARG_NONE},
	{OP_BITXOR, "bitxor", ARG_NONE},
	{OP_SHL, "shl", ARG_NONE},
	{OP_SHR, "shr", ARG_NONE},

	// logical
	{OP_OR, "or", ARG_NONE},
	{OP_AND, "and", ARG_NONE},
	{OP_NOT, "not", ARG_NONE},

	// comparison
	{OP_EQ, "eq", ARG_NONE},
	{OP_NEQ, "neq", ARG_NONE},
	{OP_LT, "lt", ARG_NONE},
	{OP_GT, "gt", ARG_NONE},
	{OP_LE, "le", ARG_NONE},
	{OP_GE, "ge", ARG_NONE},

	// function call; return
	{OP_MCALL, "mcall", ARG_NONE},
	{OP_RET, "ret", ARG_NONE},
	{OP_YIELD, "yield", ARG_NONE},

	// execution control
	{OP_JMP, "jmp", ARG_INTEGER},
	{OP_JMPT, "jmpt", ARG_INTEGER},
	{OP_JMPF, "jmpf", ARG_INTEGER},

	// debug
	{OP_FILE, "file", ARG_STRING},
	{OP_LINE, "line", ARG_INTEGER}
};
static const int num_opdesc = sizeof(opdesc) / sizeof(CLOpcodeDesc);

CLOpcodeDesc getOpcodeDesc(CLOpcode op)
{
	for (int i=0; i<num_opdesc; ++i)
	{
		if (op == opdesc[i].op) return opdesc[i];
	}
	assert(0);
	return opdesc[0];
}

