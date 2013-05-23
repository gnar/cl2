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

#include <sstream>

#include "value/clexternalfunction.h"
#include "vm/clcontext.h"
#include "serialize/clserializer.h"

CLExternalFunction::CLExternalFunction(const std::string &func_id)
	: func_id(func_id), cache_funcptr(0)
{
}

CLExternalFunctionPtr CLExternalFunction::getExternalFunctionPtr()
{
	if (cache_funcptr == 0)
	{
		cache_funcptr = CLContext::inst().getExternalFunctionPtr(func_id);
	}

	return cache_funcptr;
}

//static member
CLExternalFunction *CLExternalFunction::load(class CLSerializer &S)
{
	std::string func_id;
	S.IO(func_id);

	CLExternalFunction *fn = new CLExternalFunction(func_id); S.addPtr(fn);
	return fn;
}

//static member
void CLExternalFunction::save(class CLSerializer &S, CLExternalFunction *O)
{
	S.IO(O->func_id);
}

CLValue CLExternalFunction::clone()
{
	return CLValue(this);
}

std::string CLExternalFunction::toString()
{
	std::stringstream ss; ss << "<externalfunction@" << (void*)this << ":" << func_id << ">";
	return ss.str();
}

