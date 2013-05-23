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


#ifndef CL_SERIALSAVER_H
#define CL_SERIALSAVER_H

#include "serialize/clserializer.h"

#include <string>
#include <ostream>

class CLSerialSaver : public CLSerializer
{
public:
	CLSerialSaver(std::ostream &output);
	virtual ~CLSerialSaver();

	void IO(unsigned int &value);
	void IO(int &value);
	void IO(char &value);
	void IO(float &value);
	void IO(std::string &value);
	void IO(bool &value);
	//void IO(size_t &value);

	void magic(const std::string &code);
	void magic(unsigned int code);

private:
	std::ostream &output;
};

#endif

