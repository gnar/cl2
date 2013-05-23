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


#include "compiler/clifunction.h"
#include "value/clfunction.h"
#include "value/clstring.h"

#include <assert.h>

#include <algorithm>
#include <cstring>

#include <iostream>
using namespace std;

CLIFunction::CLIFunction()
	: num_args(0)
{
}

CLIFunction::~CLIFunction()
{
	for (size_t i=0; i<icode.size(); ++i)
	{
		delete icode[i];
	}
}

void CLIFunction::addInstruction(CLIInstruction *iinst)
{
	icode.push_back(iinst);
	//cout << debugprint_instruction(*iinst) << endl;
}

void CLIFunction::beginBlock(CLIInstruction *break_target)
{
	int new_first_id = 0;
	if (!blocks.empty()) 
	{
		Block &top = *(blocks.end()-1);
		new_first_id = top.first_id + top.locals.size();
	}

	blocks.push_back(Block()); Block &top = *(blocks.end()-1);
	top.first_id = new_first_id;
	top.break_target = break_target;
}

void CLIFunction::endBlock()
{
	Block &top = *(blocks.end()-1);
	int num_to_pop = top.locals.size();
	if (num_to_pop > 0) addInstruction(new CLIInstruction(OP_DELL, num_to_pop));
	blocks.pop_back();
}

CLIInstruction *CLIFunction::getBreakTarget(int &locals_to_pop, int level)
{
	std::vector<Block>::reverse_iterator it =  blocks.rbegin(), end = blocks.rend();
	if (it == end) return 0; // error...
	
	locals_to_pop = 0;
	while (level > 0)
	{
		locals_to_pop += it->locals.size();
		if (it->break_target)
		{
			--level;
			if (level == 0) return it->break_target;
		}
		++it;
		if (it == end) return 0; // error...
	}
	return 0; // error...
}

void CLIFunction::addParameter(const std::string &name)
{
	assert(blocks.size() == 1); // parameters are added at the first block..
	addLocal(name);
	++num_args;
}

void CLIFunction::addLocal(const std::string &name)
{
	assert(!blocks.empty());
	if (std::find(blocks.rbegin()->locals.begin(), blocks.rbegin()->locals.end(), name) != blocks.rbegin()->locals.end()) 
		assert(0); // TODO: err msg

	//cout << "added local variable " << name << endl;

	(blocks.end()-1)->locals.push_back(name);
}

int CLIFunction::getLocal(const std::string &name)
{
	std::vector<Block>::reverse_iterator it = blocks.rbegin(), end = blocks.rend();
	for (;it!=end;++it)
	{
		for (size_t i=0; i<it->locals.size(); ++i)
		{
			if (it->locals[i] == name) return it->first_id + i;
		}
	}
	return -1;
}

CLValue CLIFunction::generateFunction()
{
	CLFunction *func = new CLFunction;

	// copy constants
	func->constants = this->constants;

	// 
	for (size_t i=0; i<icode.size(); ++i) icode[i]->ip = i;

	// copy/relocate code
	func->num_args = num_args;
	func->code.resize(icode.size());
	for (size_t i=0; i<icode.size(); ++i)
	{
		CLInstruction *inst = &func->code[i];
		CLIInstruction *iinst = icode[i];

		// copy opcode & args
		inst->op = iinst->op;
		CLOpcodeDesc desc = getOpcodeDesc(iinst->op);
		switch (desc.arg_type)
		{
			case ARG_NONE: break;
			case ARG_INTEGER: inst->arg = iinst->arg; break;
			case ARG_FLOAT: inst->arg_float = iinst->arg_float; break;
			case ARG_STRING: inst->arg_str = iinst->arg_str; break;
		}

		// resolve jump targets..
		switch (iinst->op)
		{
			case OP_JMP:
			case OP_JMPF:
			case OP_JMPT:
				assert(iinst->jump_target);
				inst->arg = iinst->jump_target->ip;
				break;
			default: break;
		}

	}

	return CLValue(func);
}

int CLIFunction::getLocalsInScope()
{
	int result = 0;
	std::vector<Block>::iterator it = blocks.begin(), end = blocks.end();
	for (;it!=end;++it) result += it->locals.size();
	return result;
}

bool CLIFunction::needReturnGuard()
{
	if (icode.empty()) return true;
	return (*icode.rbegin())->op != OP_RET;
}

int CLIFunction::addStringConstant(const std::string &str)
{
	// search if constant was already added
	size_t size = constants.size();
	for (size_t i=0; i<size; ++i)
	{
		CLValue &V = constants[i];
		if (V.type == CL_STRING && GET_STRING(V)->get() == str) return static_cast<int>(i);
	}

	// not found? => Add string constant
	return addConstant(CLValue(new CLString(str)));
}

int CLIFunction::addConstant(CLValue val)
{
	constants.push_back(val);
	return static_cast<int>(constants.size()-1);
}

