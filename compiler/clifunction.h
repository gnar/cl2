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


#ifndef CLIFUNCION_H
#define CLIFUNCION_H

#include "clopcode.h"
#include "compiler/cliinstruction.h"

#include "value/clvalue.h"

#include <vector>
#include <list>
#include <string>

class CLFunction;

class CLIFunction
{
public:
	CLIFunction();
	~CLIFunction();

	CLValue generateFunction();

	void addInstruction(CLIInstruction *iinst);

	void beginBlock(CLIInstruction *break_target = 0);
	void endBlock();
	CLIInstruction *getBreakTarget(int &locals_to_pop, int level = 1);

	void addParameter(const std::string &name);
	void addLocal(const std::string &name);
	int getLocal(const std::string &name);		// -1: none found
	int getLocalsInScope();

	int addStringConstant(const std::string &str);
	int addConstant(CLValue val);

	bool needReturnGuard();

private:
	int num_args;
	struct Block
	{
		int first_id;
		std::vector<std::string> locals;
		CLIInstruction *break_target; // 0 if this block is not a loop (i.e. breakable)
	};
	std::vector<Block> blocks;
	
	std::vector<CLIInstruction*> icode;	// intermediate code
	std::vector<CLValue> constants;
};

#endif

