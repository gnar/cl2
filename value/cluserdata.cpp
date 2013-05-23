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


#include "value/cluserdata.h"

#include "vm/clcontext.h"

#include "serialize/clserializer.h"
#include "serialize/cluserdataserializer.h"

#include <assert.h>

CLUserData::CLUserData()
{
}

CLUserData::~CLUserData()
{
}

void CLUserData::set(CLValue &key, CLValue &val)
{
}

bool CLUserData::get(CLValue &key, CLValue &val)
{
	return false;
}

CLValue CLUserData::begin()
{
	return CLValue();
}

CLValue CLUserData::next(CLValue iterator, CLValue &key, CLValue &value)
{
	assert(0);
	return CLValue();
}

//static 
CLUserData *CLUserData::load(CLSerializer &S)
{
	CLUserDataSerializer *uds = S.getUserDataSerializer();
	if (!uds) assert(0); //TODO
	return uds->load(S);
}

//static 
void CLUserData::save(CLSerializer &S, CLUserData *userdata)
{
	CLUserDataSerializer *uds = S.getUserDataSerializer();
	if (!uds) assert(0); //TODO
	uds->save(S, userdata);
}

