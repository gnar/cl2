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

#include "value/clarray.h"
#include "value/clstring.h"
#include "serialize/clserializer.h"

#include <assert.h>

#include <sstream>
#include <algorithm>

void CLArray::set(CLValue &key, CLValue &val)
{
	if (key.type != CL_INTEGER) return;

	int idx = GET_INTEGER(key);
	if (idx < 0) return;	// TODO

	if (idx >= static_cast<int>(array.size())) array.resize(idx+1);

	array[idx] = val;
}

bool CLArray::get(CLValue &key, CLValue &val)
{
	switch (key.type)
	{
		case CL_INTEGER:
		{
			int idx = GET_INTEGER(key);
			if (idx < 0 || idx >= static_cast<int>(array.size())) return false;
		
			val = array[idx];
			return true;
		}

		case CL_STRING:
			if (GET_STRING(key)->get() == "n")
			{
				val = CLValue(static_cast<int>(array.size()));
				return true;
			}
			return false;

		default:
			return false;
	}
	return false;
}

CLValue CLArray::clone()
{
	CLArray *dst = new CLArray();
	CLArray *src = this;

	dst->array = src->array;

	return CLValue(dst);
}

std::string CLArray::toString()
{
	std::stringstream ss;
	ss << '[';
	for (size_t i=0; i<array.size(); ++i)
	{
		ss << array[i].toString() << ", ";
	}
	ss << ']';
	return ss.str();
}

CLValue CLArray::begin()
{
	if (array.empty()) return CLValue(); else return CLValue(0);
}

CLValue CLArray::next(CLValue iterator, CLValue &key, CLValue &value)
{
	int it = GET_INTEGER(iterator);
	
	key = iterator;
	value = array[it];
	
	++it;
	if (it < static_cast<int>(array.size())) return CLValue(it); else return CLValue();
}

/*static member*/
CLArray *CLArray::load(class CLSerializer &S)
{
	size_t size;
	S.IO(size);

	CLArray *a = new CLArray(); S.addPtr(a);
	a->array.resize(size, CLValue());
	for (size_t i=0; i<size; ++i)
	{
		a->array[i] = CLValue::load(S);
	}

	return a;
}

/*static member*/
void CLArray::save(class CLSerializer &S, CLArray *O)
{
	size_t size = O->array.size();
	S.IO(size);
	
	for (size_t i=0; i<size; ++i)
	{
		CLValue::save(S, O->array[i]);
	}
}

// GC
void CLArray::gc_mark()
{
	if (gc_isMarked()) return;

	gc_setMarked();

	size_t size = array.size();
	for (size_t i=0; i<size; ++i)
	{
		array[i].markObject();
	}
}

