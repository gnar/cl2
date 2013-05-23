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


#include "vm/clmathmodule.h"
#include "vm/clcontext.h"
#include "vm/clthread.h"

#include "value/clvalue.h"
#include "value/clstring.h"
#include "value/cltable.h"
#include "value/clexternalfunction.h"

#include <cmath>

#define DECL_FUNC(name) CLValue name (CLThread &thread, std::vector<CLValue> &args, CLValue self)

#ifndef M_PI
#define M_PI 3.1416
#endif

static DECL_FUNC(math_sin);
static DECL_FUNC(math_cos);
static DECL_FUNC(math_tan);
static DECL_FUNC(math_asin);
static DECL_FUNC(math_acos);
static DECL_FUNC(math_atan);
static DECL_FUNC(math_sqrt);
static DECL_FUNC(math_random);
static DECL_FUNC(math_atan2);

CLMathModule::CLMathModule() : CLModule("math")
{
	registerFunction("sin",    "math_sin",    &math_sin);
	registerFunction("cos",    "math_cos",    &math_cos);
	registerFunction("tan",    "math_tan",    &math_tan);
	registerFunction("asin",   "math_asin",   &math_asin);
	registerFunction("acos",   "math_acos",   &math_acos);
	registerFunction("atan",   "math_atan",   &math_atan);
	registerFunction("sqrt",   "math_sqrt",   &math_sqrt);
	registerFunction("random", "math_random", &math_random);
	registerFunction("atan2",  "math_atan2",  &math_atan2);
}

CLMathModule::~CLMathModule()
{
}

static inline float float_arg0(std::vector<CLValue> &args)
{
	float result = 0.0f;
	if (args.size() >= 1) switch (args[0].type)
	{
		case CL_FLOAT: result = GET_FLOAT(args[0]); break;
		case CL_INTEGER: result = (float)GET_INTEGER(args[0]); break;
		default: assert(0);
	}
	return result;
}

static DECL_FUNC(math_sin)
{
	return CLValue(float(std::sin(float_arg0(args) * M_PI / 180.0f)));
}

static DECL_FUNC(math_cos)
{
	return CLValue(float(std::cos(float_arg0(args) * M_PI / 180.0f)));
}

static DECL_FUNC(math_tan)
{
	return CLValue(float(std::tan(float_arg0(args) * M_PI / 180.0f)));
}

static DECL_FUNC(math_asin)
{
	return CLValue(float(std::asin(float_arg0(args)) / M_PI * 180.0f));
}

static DECL_FUNC(math_acos)
{
	return CLValue(float(std::acos(float_arg0(args)) / M_PI * 180.0f));
}

static DECL_FUNC(math_atan)
{
	return CLValue(float(std::atan(float_arg0(args)) / M_PI * 180.0f));
}

static DECL_FUNC(math_sqrt)
{
	return CLValue(float(std::sqrt(float_arg0(args))));
}

static DECL_FUNC(math_random)
{
	switch (args.size())
	{
		case 0: return CLValue((int)std::rand());
		case 1: 
		{
			int i = GET_INTEGER(args[0]);
			return CLValue(int(std::rand() % i));
		}
	}	

	return CLValue::Null();
}

static DECL_FUNC(math_atan2)
{
	return CLValue::Null(); // TODO
}

