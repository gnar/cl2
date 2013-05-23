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

#include "vm/clsysmodule.h"
#include "vm/clcontext.h"
#include "vm/clthread.h"

#include "value/clvalue.h"
#include "value/clstring.h"
#include "value/cltable.h"
#include "value/clexternalfunction.h"

#include <iostream>

#define DECL_FUNC(name) CLValue name (CLThread &thread, std::vector<CLValue> &args, CLValue self)

// global functions
static DECL_FUNC(version);
static DECL_FUNC(print);
static DECL_FUNC(println);
static DECL_FUNC(startthread);
static DECL_FUNC(import); 

// string member functions
static DECL_FUNC(string_length);
static DECL_FUNC(string_concat);
static DECL_FUNC(string_substr);
static DECL_FUNC(string_replace);

// thread member functions
static DECL_FUNC(thread_kill);
static DECL_FUNC(thread_isrunning);
static DECL_FUNC(thread_suspend);
static DECL_FUNC(thread_resume);

CLSysModule::CLSysModule() : CLModule("sys")
{
	// global functions
	registerFunction("version",      "sys_version",         &version);
	registerFunction("print",        "sys_print",           &print);
	registerFunction("println",      "sys_println",         &println);
	registerFunction("startthread",  "sys_startthread",     &startthread);
	registerFunction("import",       "sys_import",          &import);

	// string member functions
	registerFunction("sys_string_length",                   &string_length);
	registerFunction("sys_string_concat",                   &string_concat);
	registerFunction("sys_string_substr",                   &string_substr);
	registerFunction("sys_string_replace",                  &string_replace);

	// thread member functions
	registerFunction("sys_thread_kill",                     &thread_kill);
	registerFunction("sys_thread_isrunning",                &thread_isrunning);
	registerFunction("sys_thread_suspend",                  &thread_suspend);
	registerFunction("sys_thread_resume",                   &thread_resume);
}

CLSysModule::~CLSysModule()
{
}

///////////////////////
// Global functions  //
///////////////////////

static DECL_FUNC(version)
{
	return CLValue(new CLString("CL2 script language -- version 0"));
}

static DECL_FUNC(print)
{
	int i, argc = args.size();
	for (i=0; i<argc; ++i)
	{
		std::cout << args[i].toString();
	}
	std::cout << std::flush;
	return CLValue::Null();
}

static DECL_FUNC(println)
{
	print(thread, args, self);
	std::cout << std::endl;
	return CLValue::Null();
}

static DECL_FUNC(startthread) // startthread(func, arg0, ...argN, self)
{
	CLValue func = args[0]; args.erase(args.begin());
	CLValue self_= args[args.size()-1]; args.pop_back();
	CLValue result = CLValue(new CLThread());
	GET_THREAD(result)->init(func, args, self_);
	return result;
}

static DECL_FUNC(import)
{
	CLObject *dst = GET_OBJECT(args[0]);
	CLObject *src = GET_OBJECT(args[1]);
	CLValue it, key, val;

	it = src->begin();
	while (it.isTrue())
	{
		it = src->next(it, key, val);
		dst->set(key, val);
	}

	return CLValue::True();
}


// String member functions

static DECL_FUNC(string_concat) // <str>.concat(<str>) => <str (new)>
{
	// check arguments
	if ((self.type == CL_STRING) && (args.size() > 0) && (args[0].type == CL_STRING))
	{
		const std::string &other = GET_STRING(args[0])->get();
		const std::string &self_ = GET_STRING(self)->get();
		return CLValue(new CLString(self_ + other));
	} else {
		return CLValue::Null();
	}
}

static DECL_FUNC(string_length) // <str>.length() => <int>
{
	if (self.type == CL_STRING) 
	{
		return CLValue(int(GET_STRING(self)->get().length()));
	} else {
		return CLValue::Null();
	}
}

static DECL_FUNC(string_substr) // <str>.substr(pos, len) => <str (new)>
{
	if ((self.type == CL_STRING) && (args.size() >= 2) && (args[0].type == CL_INTEGER) && (args[1].type == CL_INTEGER))
	{
		const std::string &str = GET_STRING(self)->get();
		size_t pos = GET_INTEGER(args[0]);
		size_t len = GET_INTEGER(args[1]);
		return CLValue(new CLString(str.substr(pos, len)));
	} else {
		return CLValue::Null();
	}
}

static DECL_FUNC(string_replace) // <str>.replace(pos, len, <str>) => <str (self)>
{
	if ((self.type == CL_STRING) && (args.size() >= 3) && 
            (args[0].type == CL_INTEGER) && (args[1].type == CL_INTEGER) && (args[2].type == CL_STRING))
	{
		std::string self_str = GET_STRING(self)->get();
		const std::string &other_str = GET_STRING(args[2])->get();
		size_t pos = GET_INTEGER(args[0]);
		size_t len = GET_INTEGER(args[1]);
		GET_STRING(self)->set(self_str.replace(pos, len, other_str));
		return self;
	} else {
		return CLValue::Null();
	}
}

// Thread member functions

static DECL_FUNC(thread_kill)
{
	CLThread *thr = GET_THREAD(self);
	if (thr->isRunning())
	{
		thr->kill();
		return CLValue::True();
	} else {
		return CLValue::False();
	}
}

static DECL_FUNC(thread_isrunning)
{
	CLThread *thr = GET_THREAD(self);
	if (thr->isRunning())
	{
		return CLValue::True();
	} else {
		return CLValue::False();
	}
}

static DECL_FUNC(thread_suspend)
{
	CLThread *thr = GET_THREAD(self);
	if (thr->isSuspended())
	{
		return CLValue::False();
	} else {
		thr->suspend();
		return CLValue::True();
	}
}

static DECL_FUNC(thread_resume)
{
	CLThread *thr = GET_THREAD(self);
	if (thr->isSuspended())
	{
		thr->resume();
		return CLValue::True();
	} else {
		return CLValue::False();
	}
}

