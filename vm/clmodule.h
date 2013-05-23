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


#ifndef CLMODULE_H
#define CLMODULE_H

#include <string>
#include <vector>
#include <list>

class CLThread;
class CLValue;

typedef CLValue (*CLExternalFunctionPtr)(CLThread &thread, std::vector<CLValue> &args, CLValue self);

class CLModule
{
public:
	CLModule(const std::string &name);
	virtual ~CLModule();

	const std::string &getName() { return this->name; }

	virtual CLExternalFunctionPtr getExternalFunctionPtr(const std::string &ident);

	virtual void init();
	virtual void deinit();

protected:
	void registerFunction(std::string name, std::string id, CLExternalFunctionPtr func); // function with name
	void registerFunction(std::string id, CLExternalFunctionPtr func); // function without name

private:
	const std::string name;

	struct RegisteredFunction
	{
		RegisteredFunction(std::string name, std::string id, CLExternalFunctionPtr func) : name(name), id(id), func(func) {}
		~RegisteredFunction() {}

		std::string name; // function name visible to application
		std::string id; // external_function id
		CLExternalFunctionPtr func; // the function
	};
	std::list<RegisteredFunction> reg_funcs;
};

#endif

