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


#include "value/clfunction.h"
#include "value/clvalue.h"

#include "serialize/clserializer.h"

#include <sstream>

#include <iostream>
using namespace std;

CLFunction::CLFunction()
	: num_args(0)
{
}

CLFunction::~CLFunction()
{
}

//static member
void CLFunction::save(CLSerializer &S, CLFunction *O)
{
	// write number of arguments
	S.IO(O->num_args);
	
	// write code
	int codesize = O->code.size();
	S.IO(codesize);
	for (int i=0; i<codesize; ++i)
	{
		CLInstruction &inst = O->code[i];

		// write opcode
		char opcode = inst.op;
		S.IO(opcode);
		
		// write argument, if any
		CLOpcodeDesc desc = getOpcodeDesc(inst.op);
		switch (desc.arg_type)
		{
			case ARG_NONE: break;
			case ARG_INTEGER: S.IO(inst.arg); break;
			case ARG_FLOAT: S.IO(inst.arg_float); break;
			case ARG_STRING: S.IO(inst.arg_str); break;
		}
	}
	
	// write constants
	int tmp;
	S.IO(tmp = O->constants.size());
	for (int i=0; i<tmp; ++i)
	{
		CLValue::save(S, O->constants[i]);
	}
}

//static member
CLFunction *CLFunction::load(CLSerializer &S)
{
	CLFunction *f = new CLFunction(); S.addPtr(f);

	// read number of argument
	S.IO(f->num_args);

	// read code
	int codesize;
	S.IO(codesize);
	f->code.resize(codesize);
	for (int i=0; i<codesize; ++i)
	{
		CLInstruction &inst = f->code[i];

		// read opcode
		char opcode;
		S.IO(opcode); inst.op = CLOpcode(opcode);

		// load argument, if any
		CLOpcodeDesc desc = getOpcodeDesc(inst.op);
		switch (desc.arg_type)
		{
			case ARG_NONE: break;
			case ARG_INTEGER: S.IO(inst.arg); break;
			case ARG_FLOAT: S.IO(inst.arg_float); break;
			case ARG_STRING: S.IO(inst.arg_str); break;
		}
	}

	// read constants
	int tmp;
	S.IO(tmp); 
	for (int i=0; i<tmp; ++i)
	{
		f->constants.push_back(CLValue::load(S));
	}

	return f;
}

CLValue CLFunction::clone()
{
	return CLValue(this);
}

std::string CLFunction::toString()
{
	std::stringstream ss; ss << "<function@" << (void*)this << ">";
	return ss.str();
}

// GC
void CLFunction::gc_mark()
{
	if (gc_isMarked()) return;
	gc_setMarked();

	size_t size = constants.size();
	for (size_t i=0; i<size; ++i)
	{
		constants[i].markObject();
	}
}



