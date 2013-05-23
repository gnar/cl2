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


#include "value/clstring.h"
#include "value/clexternalfunction.h"
#include "serialize/clserializer.h"

CLString::CLString(const char *cstr)
	: cache_valid(false)
{
	set(std::string(cstr));
}

CLString::CLString(const std::string &str)
{
	set(str);
}

CLString::~CLString()
{
}

unsigned int CLString::hash()
{
	if (cache_valid)
	{
		return cached_hash;
	} else {
		size_t l = value.size();
		unsigned int h = l;  /* seed: string length */
		size_t step = (l >> 5) | 1;  /* if string is too long, don't hash all its chars */
		int pos = 0;
		for (; l>=step; l-=step) h = h ^ ((h<<5)+(h>>2)+(unsigned char)(value[pos++]));

		cache_valid = true;
		cached_hash = h;
		return h;
	}
}

void CLString::set(CLValue &key, CLValue &val)
{
}

bool CLString::get(CLValue &key, CLValue &val)
{
	switch (key.type)
	{
		case CL_INTEGER:
		{
			int pos = GET_INTEGER(key);
			if ((pos < 0) || (pos >= static_cast<int>(value.length()))) return false;

			char ch[2];
			ch[0] = value[pos];
			ch[1] = 0;

			val = CLValue(new CLString(ch));
			return true;
		}

		case CL_STRING:
		{
			const std::string &key_str = GET_STRING(key)->get();
			if (key_str == "length") {
				val = CLValue(new CLExternalFunction("sys_string_length"));
				return true;
			} else if (key_str == "clone") {
				val = CLValue(new CLExternalFunction("sys_string_clone"));
				return true;
			} else if (key_str == "concat") {
				val = CLValue(new CLExternalFunction("sys_string_concat"));
				return true;
			} else if (key_str == "substr") {
				val = CLValue(new CLExternalFunction("sys_string_substr"));
				return true;
			} else if (key_str == "replace") {
				val = CLValue(new CLExternalFunction("sys_string_replace"));
				return true;
			} else {
				return false;
			}
		}

		default:
			return false;
	}

	return false;
}

CLValue CLString::clone()
{
	return CLValue(this);
}

std::string CLString::toString()
{
	return get();
}

CLValue CLString::begin()
{
	return value.empty() ? CLValue::True() : CLValue::False();
}

CLValue CLString::next(CLValue iterator, CLValue &key, CLValue &val)
{
	int pos = GET_INTEGER(iterator);

	get(key = iterator, val);

	++pos;
	if (pos >= static_cast<int>(value.length()))
		return CLValue::Null();
	else 
		return CLValue(pos);
}


/*static member*/
CLString *CLString::load(CLSerializer &ss)
{
	std::string str;
	ss.IO(str);

	CLString *s = new CLString(str); ss.addPtr(s);
	return s;
}

/*static member*/
void CLString::save(CLSerializer &ss, CLString *O)
{
	std::string str = O->get();
	ss.IO(str);
}
	

