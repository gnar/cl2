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


#include "value/clobject.h"
#include "value/clstring.h"
#include "serialize/clserializer.h"

#ifdef DEBUG
#include "vm/clcontext.h"
#endif

#include <assert.h>

#include <sstream>

#include <iostream>
using namespace std;

CLObject::CLObject()
{
#ifdef DEBUG
	++ocount;
	//cout << "Object created " << ocount << "(" << this << ")" << endl;
#endif
}

CLObject::~CLObject()
{
#ifdef DEBUG
	--ocount;
	//cout << "Object deleted " << ocount << "(" << this << ")" << endl;
#endif
}

void CLObject::set(CLValue &key, CLValue &val)
{
}

bool CLObject::get(CLValue &key, CLValue &val)
{
	return false;
}

CLValue CLObject::begin()
{
	cout << "This object does not support iteration!" << endl;
	return CLValue();
}

CLValue CLObject::next(CLValue iterator, CLValue &key, CLValue &value)
{
	assert(0);
	return CLValue();
}

CLValue CLObject::clone()
{
	cout << "This object does not support cloning!" << endl;
	return CLValue::Null();
}

std::string CLObject::toString()
{
	std::stringstream ss; ss << "<object@" << (void*)this << ">";
	return ss.str();
}

