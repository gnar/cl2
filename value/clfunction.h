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


#ifndef CLFUNCTION_H
#define CLFUNCTION_H

#include "value/clobject.h"
#include "value/clvalue.h"
#include "clopcode.h"

#include <vector>
#include <string>

struct CLInstruction
{
	CLOpcode op;
	int arg;
	float arg_float;
	std::string arg_str;
};

class CLFunction : public CLObject
{
public:
	CLFunction();
	~CLFunction();

	// load/save
	static CLFunction *load(class CLSerializer &ss);
	static void save(class CLSerializer &ss, CLFunction *O);

	std::vector<CLInstruction> code;
	std::vector<CLValue> constants;
	int num_args;

	// clone
	virtual CLValue clone();

	// to string..
	virtual std::string toString();

private:
	// GC
	virtual void gc_mark();
};

#endif

