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


#ifndef CLIINSTRUCTION_H
#define CLIINSTRUCTION_H

#include <string>

#include "clopcode.h"

struct CLIInstruction	// intermediate intruction
{
	CLIInstruction(CLOpcode op_) : op(op_), jump_target(0) {}
	CLIInstruction(CLOpcode op_, int arg_) : op(op_), arg(arg_), jump_target(0) {}
	CLIInstruction(CLOpcode op_, float arg_float_) : op(op_), arg_float(arg_float_), jump_target(0) {}
	CLIInstruction(CLOpcode op_, std::string arg_str_) : op(op_), arg_str(arg_str_), jump_target(0) {}

	CLOpcode op;
	int arg;
	float arg_float;
	std::string arg_str;
	class CLFunction *arg_func;

	CLIInstruction *jump_target; // unrsolved jump target
	
	int ip; // position in function
};

extern std::string debugprint_instruction(const CLIInstruction &iinst);

#endif

