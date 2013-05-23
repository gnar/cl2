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

#include "serialize/clserialloader.h"

#include <assert.h>

#include <iostream>

using namespace std;

CLSerialLoader::CLSerialLoader(std::istream &input_)
	: input(input_)
{
}

CLSerialLoader::~CLSerialLoader()
{
}

void CLSerialLoader::IO(unsigned int &value)
{
	input.read((char*)&value, sizeof(unsigned int));
}

void CLSerialLoader::IO(int &value)
{
	input.read((char*)&value, sizeof(int));
}

/*void CLSerialLoader::IO(size_t &value)
{
	input.read((char*)&value, sizeof(size_t));
}*/

void CLSerialLoader::IO(char &value)
{
	input.read((char*)&value, 1);
}

void CLSerialLoader::IO(float &value)
{
	input.read((char*)&value, sizeof(float));
}

void CLSerialLoader::IO(bool &value)
{
	char tmp; IO(tmp);
	value = (tmp != 0);
}

void CLSerialLoader::IO(std::string &value)
{
	unsigned int len = value.size();
	IO(len);

	value = std::string(len, ' ');
	for (unsigned i=0; i<len; ++i) 
	{
		char ch; IO(ch);
		value[i] = ch;
	}
}

void CLSerialLoader::magic(const std::string &code)
{
	std::string in; IO(in);
	if (code != in)
	{
		cout << "Expected magic: " << code << ", got: " << in << endl;
		assert(0);
	}
}

void CLSerialLoader::magic(unsigned int code)
{
	unsigned in; IO(in);
	if (code != in)
	{
		cout << "Expected magic: " << code << ", got: " << in << endl;
		assert(0);
	}
}

