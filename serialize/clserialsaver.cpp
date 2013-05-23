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


#include "serialize/clserialsaver.h"

CLSerialSaver::CLSerialSaver(std::ostream &output)
	: output(output)
{
}

CLSerialSaver::~CLSerialSaver()
{
}

void CLSerialSaver::IO(unsigned int &value)
{
	output.write((char*)&value, sizeof(unsigned int));
}

void CLSerialSaver::IO(int &value)
{
	output.write((char*)&value, sizeof(int));
}

/*void CLSerialSaver::IO(size_t &value)
{
	output.write((char*)&value, sizeof(size_t));
}*/

void CLSerialSaver::IO(char &value)
{
	output.write((char*)&value, 1);
}

void CLSerialSaver::IO(float &value)
{
	output.write((char*)&value, sizeof(float));
}

void CLSerialSaver::IO(bool &value)
{
	char tmp = (value ? 1 : 0);
	IO(tmp);
}

void CLSerialSaver::IO(std::string &value)
{
	unsigned int len = value.size();
	IO(len);

	for (unsigned i=0; i<len; ++i) 
	{
		char ch = value[i];
		IO(ch);
	}
}

void CLSerialSaver::magic(const std::string &code)
{
	std::string tmp(code);
	IO(tmp);
}

void CLSerialSaver::magic(unsigned int code)
{
	IO(code);
}

