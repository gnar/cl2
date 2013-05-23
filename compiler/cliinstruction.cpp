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


#include "compiler/cliinstruction.h"

#include <string>
#include <sstream>

std::string debugprint_instruction(const CLIInstruction &iinst)
{
	std::stringstream result; 
	CLOpcodeDesc desc = getOpcodeDesc(iinst.op);

	result << desc.name << " ";

	switch (desc.arg_type)
	{
		case ARG_NONE: break;
		case ARG_INTEGER: result << iinst.arg; break;
		case ARG_FLOAT: result << iinst.arg_float; break;
		case ARG_STRING: result << iinst.arg_str; break;
	}

	return result.str();
}

