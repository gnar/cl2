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

#ifndef CL_ARRAY_H
#define CL_ARRAY_H

#include "value/clvalue.h"
#include "value/clobject.h"

#include "serialize/clserializer.h"

#include <vector>
#include <string>

class CLArray : public CLObject
{
public:
	// access values by key (= integers) ..
	virtual void set(CLValue &key, CLValue &val);
	virtual bool get(CLValue &key, CLValue &val);

	// clone
	virtual CLValue clone();
	// to string..
	virtual std::string toString();

	// iteration support:
	CLValue begin();
	CLValue next(CLValue iterator, CLValue &key, CLValue &value);

	// load/save
	static CLArray *load(class CLSerializer &ss);
	static void save(class CLSerializer &ss, CLArray *O);

private:
	std::vector<CLValue> array;

	// GC
	virtual void gc_mark();
};

#endif

