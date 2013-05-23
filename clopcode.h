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


#ifndef CLOPCODE_H
#define CLOPCODE_H

enum CLOpcode
{
	//                 Stack in               | Stack out                    | arg0
	// --------------------------------------------------------------------------------------------------
	OP_NOP,         //                        |                              |

	OP_PUSH0,       //                        | null value                   |
	OP_PUSHSELF,    //                        | self context                 |
	OP_PUSHROOT,    //                        | root table                   |
	OP_PUSHCONST,   //                        | constant value               | <i> constant id#
	OP_PUSHEXTFUNC, //                        | external_function value      | <s> identification string
	OP_PUSHI,       //                        | integer value                | <i> value to push
	OP_PUSHF,       //                        | float value                  | <f> value to push
	OP_PUSHS,       //                        | string value                 | <s> value to push
	OP_POP,	        //                        |                              | <i> number of items to pop

	OP_DUP,         //                        |                              | <i> (positive) offset of value to dup (0 = stack top)

	// objects
	OP_NEWTABLE,    //                        | new table                    |
	OP_NEWARRAY,

	OP_TABGET,      // table,key              | value                        |
	OP_TABGET2,     // table,key              | value,table                  | // this is for member calls (table=new self)
	OP_TABSET,      // table,key,value        | input value                  |
	OP_TABIT,       // table                  | table,iterator               | // returns iterator
	OP_TABNEXT,     // table,iterator         | table,++iterator,value,key   |

	OP_CLONE,       // object                 | cloned_object                |

	// local variable creation/removal
	OP_PUSHL,       //                        | local variable contents      | <i> local var #
	OP_POPL,        // new value              |                              | <i> local var #
	OP_ADDL,        //                        |                              | <i> number of locals to add
	OP_DELL,        //                        |                              | <i> number of locals to remove
	
	// arithmetic
	OP_ADD,		// 2 Operands             | 1: Result
	OP_SUB,		// 2 Operands             | 1: Result                    | 
	OP_MUL,		// 2 Operands             | 1: Result                    |
	OP_DIV,		// 2 Operands             | 1: Result                    |
	OP_MODULO,	// 2 Operands             | 1: Result                    |
	OP_NEG,		// 1 Operand              | 1: Result                    |

	// bitwise
	OP_BITOR,	// 2 Operands             | 1: Result                    |
	OP_BITAND,	// 2 Operands             | 1: Result                    |
	OP_BITXOR,	// 2 Operands             | 1: Result                    |
	OP_SHL,		// 2 Operands             | 1: Result                    |
	OP_SHR,		// 2 Operands             | 1: Result                    |

	// logical
	OP_AND,		// 2 Operands             | 1: Result                    |
	OP_OR,		// 2 Operands             | 1: Result                    |
	OP_NOT,		// 1 Operand              | 1: Result                    |

	// comparison
	OP_EQ,		// 2 Operands             | 1: Result                    |
	OP_NEQ,		// 2 Operands             | 1: Result                    |
	OP_LT,		// 2 Operands             | 1: Result                    |
	OP_GT,		// 2 Operands             | 1: Result                    |
	OP_LE,		// 2 Operands             | 1: Result                    |
	OP_GE,		// 2 Operands             | 1: Result                    |

	OP_MCALL,       //func,self,arg[1..n],argc| function result              |
	OP_RET,         // function result        |                              |                       
	OP_YIELD,       // yield result           |                              | 

	OP_JMP,	        //                        |                              | <i> new instruction pointer
	OP_JMPT,        // condition              |                              | <i> new instruction pointer (if condition is true)
	OP_JMPF,        // condition              |                              | <i> new instruction pointer (if condition is false)

	OP_FILE,        //                        |                              | <s> file name
	OP_LINE,        //                        |                              | <i> line number
};

enum CLArgType
{
	ARG_NONE,
	ARG_INTEGER,
	ARG_FLOAT,
	ARG_STRING,
};

struct CLOpcodeDesc
{
	CLOpcode op;
	char *name;
	CLArgType arg_type;
};

extern CLOpcodeDesc getOpcodeDesc(CLOpcode op);

#endif

