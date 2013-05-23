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


#include "vm/clmodule.h"
#include "vm/clcontext.h"
#include "value/clvalue.h"
#include "value/cltable.h"
#include "value/clexternalfunction.h"

#include <iostream>
using namespace std;

CLModule::CLModule(const std::string &name) : name(name)
{
}

CLModule::~CLModule()
{
}

CLExternalFunctionPtr CLModule::getExternalFunctionPtr(const std::string &ident)
{
	std::list<RegisteredFunction>::iterator it = reg_funcs.begin(), end = reg_funcs.end();
	for (; it!=end; ++it)
	{
		if (ident == it->id) return it->func;
	}
	return 0;
}

void CLModule::registerFunction(std::string name, std::string id, CLExternalFunctionPtr func)
{
	reg_funcs.push_back(RegisteredFunction(name, id, func));
}

void CLModule::registerFunction(std::string id, CLExternalFunctionPtr func)
{
	reg_funcs.push_back(RegisteredFunction("", id, func));
}

void CLModule::init()
{
	// create module namespace table
	CLValue ns = CLValue(new CLTable()); 

	// populate namespace
	std::list<RegisteredFunction>::iterator it = reg_funcs.begin(), end = reg_funcs.end();
	for (; it!=end; ++it)
	{
		if (it->name != "") // not anonymous?
			ns.set(CLValue(it->name.c_str()), CLValue(new CLExternalFunction(it->id.c_str())));
	}

	// add namespace into root environment
	getName();
	CLValue root = CLContext::inst().getRootTable();
	CLValue ns_name = CLValue(getName().c_str());
	root.set(ns_name, ns); 
}

void CLModule::deinit()
{
	// remove module namespace table
	CLValue root = CLContext::inst().getRootTable();
	root.set(CLValue(getName().c_str()), CLValue::Null());
}

