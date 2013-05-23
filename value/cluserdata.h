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


#ifndef CL_USERDATA_H
#define CL_USERDATA_H

#include "value/clobject.h"

class CLContext;
class CLSerializer;

class CLUserData : public CLObject
{
public:
	CLUserData();
	virtual ~CLUserData();

	// access values by key..
	virtual void set(CLValue &key, CLValue &val);
	virtual bool get(CLValue &key, CLValue &val); // returns true if key existed

	// iteration support:
	virtual CLValue begin();
	virtual CLValue next(CLValue iterator, CLValue &key, CLValue &value);

	// serializasion support (behaviour depends on the CLSerializer's CLUserDataSerializer)
	static CLUserData *load(CLSerializer &S);
	static void save(CLSerializer &S, CLUserData *userdata);
};

#endif

