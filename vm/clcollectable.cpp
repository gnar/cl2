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

#include "vm/clcollectable.h"
#include "vm/clcontext.h"

#include <iostream>
using namespace std;

CLCollectable::CLCollectable() : marked(false), finalized(false), prev(0), next(0), lock_cnt(0)
{
	CLContext::inst().addToHeapList(this);
}

CLCollectable::~CLCollectable()
{
}

void CLCollectable::gc_mark()
{
	gc_setMarked();
}

bool CLCollectable::gc_isMarked()
{
	return marked;
}

void CLCollectable::gc_setMarked()
{
	marked = true;
}

void CLCollectable::gc_finalize()
{
	gc_setFinalized();
}

bool CLCollectable::gc_isFinalized()
{
	return finalized;
}

void CLCollectable::gc_setFinalized()
{
	finalized = true;
}

