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


#ifndef CLSERIALIZER_H
#define CLSERIALIZER_H

#include <string>
#include <vector>

#include <assert.h>

class CLUserDataSerializer;

class CLSerializer
{
public:
	CLSerializer() : userdata_serializer(0) {}
	virtual ~CLSerializer() {}

	virtual void IO(unsigned int &value) = 0;
	virtual void IO(int &value) = 0;
	virtual void IO(char &value) = 0;
	virtual void IO(std::string &value) = 0;
	virtual void IO(float &value) = 0;
	virtual void IO(bool &value) = 0;
	//virtual void IO(size_t &value) = 0;

	inline int addPtr(void *ptr)
	{
		ptr_stack.push_back(ptr);
		return ptr_stack.size() - 1;
	}

	inline void *getPtr(int idx)
	{
		assert((idx >= 0) && ((size_t)idx < ptr_stack.size()));
		return ptr_stack[idx];
	}

	inline int findPtr(void *ptr)
	{
		for (unsigned i=0; i<ptr_stack.size(); ++i) if (ptr == ptr_stack[i]) return i;
		return -1;
	}

	void setUserDataSerializer(CLUserDataSerializer *uds) { userdata_serializer = uds; }
	CLUserDataSerializer *getUserDataSerializer() { return userdata_serializer; }

	virtual void magic(const std::string &code) = 0;
	virtual void magic(unsigned int code) = 0;

private:
	std::vector<void*> ptr_stack;
	CLUserDataSerializer *userdata_serializer;
};

#endif

