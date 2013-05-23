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

#include "vm/clcontext.h"

#include "value/clvalue.h"
#include "value/cltable.h"

#include "vm/clmathmodule.h"

#include <assert.h>
#include <iostream>

#include "serialize/clserializer.h"

#include <stdexcept>

using namespace std;

#ifdef DEBUG
int ocount = 0;
#endif

CLContext::CLContext() : gc_heap_list(0), gc_finalized_list(0)
{
	if (instance) throw std::runtime_error("VM context already created!");
	instance = this;

	clear();
	addModule(&sys);
}

CLContext::~CLContext()
{
	shutdown();

	// 
	instance = 0;
}

// singleton instance
CLContext *CLContext::instance = 0;

// singleton getter
CLContext &CLContext::inst()
{
	if (instance) return *instance;
	else throw std::runtime_error("No VM context instance created");
}

void CLContext::shutdown()
{
	// Free root table //////////////////////////////////
	roottable.setNull();

	// Finalized all remaining objects //////////////////
	if (gc_heap_list)
	{
	redo:
		CLCollectable *it = gc_heap_list;
		while (it)
		{
			if (!it->gc_isFinalized()) 
			{
				it->gc_finalize();
				moveToFinalizedList(it);
				goto redo; // finalization might invalidate iterator 'it'
			}
			it = it->next;
		}
	}

	// Free finalized objects ///////////////////////////
	freeFinalized();

#ifdef DEBUG
	if (ocount != 0)            clog << "Internal error: Uncollected objects left after shutdown." << endl;
	if (gc_heap_list != 0)      clog << "Internal error: gc_heap_list != 0 after shutdown" << endl;
	if (gc_finalized_list != 0) clog << "Internal error: gc_finalized_list != 0 after shutdown" << endl;
	if (threads.size() != 0)    clog << "Internal error: threads.size() != 0 after shutdown" << endl;
#endif

	// Should be 0 anyway..
	gc_heap_list = 0;
	gc_finalized_list = 0;
}

void CLContext::registerThread(CLValue thread) // called by thread constructor
{
	assert(thread.type == CL_THREAD);

	threads.push_back(thread);
}

void CLContext::unregisterThread(CLValue thread) // called by thread destructor
{
	assert(thread.type == CL_THREAD);

	std::list<CLValue>::iterator it = threads.begin(), end = threads.end();
	for (;it!=end;++it)
	{
		if (GET_THREAD(thread) == GET_THREAD(*it)) 
		{
			it = threads.erase(it); 
			return; 
		}
	}

	clog << "Internal vm error: unregisterThread failed." << endl;
	assert(0);
}

int CLContext::countRunningThreads()
{
	int result = 0;

	std::list<CLValue>::iterator it = threads.begin(), end = threads.end();
	for (;it!=end;++it) if (GET_THREAD(*it)->isRunning()) ++result; 

	return result;
}

CLExternalFunctionPtr CLContext::getExternalFunctionPtr(const std::string &func_id)
{
	std::list<CLModule*>::iterator it = modules.begin(), end = modules.end();
	for (;it!=end;++it)
	{
		CLExternalFunctionPtr p = (*it)->getExternalFunctionPtr(func_id);
		if (p) return p;
	}
	return 0;
}

void CLContext::addModule(CLModule *module)
{
	modules.push_back(module);
	module->init();
}

void CLContext::removeModule(CLModule *module)
{
	module->deinit();
	modules.remove(module);
}

void CLContext::roundRobin(int timeout)
{
	std::list<CLValue>::iterator it = threads.begin(), end = threads.end();
	for (;it!=end;++it)
	{
		CLValue &thread = *it;
		if (GET_THREAD(thread)->isRunning()) GET_THREAD(thread)->run(timeout);
	}
}

void CLContext::clear()
{
	shutdown();
	roottable = CLValue(new CLTable());

	// reinit all modules
	std::list<CLModule*>::iterator it = modules.begin(), end = modules.end();
	for (;it!=end; ++it)
	{
		(*it)->init();
	}
}

void CLContext::save(CLSerializer &S)
{
#ifdef DEBUG
	S.magic("[CONTEXT]");
#endif

	CLValue::save(S, roottable); // save global environment

	unsigned int tmp;
	S.IO(tmp = threads.size());  // save number of threads

	std::list<CLValue>::iterator it = threads.begin(), end = threads.end();
	for (;it!=end;++it)		// save each thread
	{
#	ifdef DEBUG
		S.magic("[THREAD]");
#	endif
		CLValue::save(S, *it);
#	ifdef DEBUG
		S.magic("[THREAD-END]");
#	endif
	}
#ifdef DEBUG
	S.magic("[CONTEXT-END]");
#endif
}

void CLContext::load(CLSerializer &S)
{
	clear();

#ifdef DEBUG
	assert(threads.empty());
	S.magic("[CONTEXT]");
#endif

	roottable = CLValue::load(S); // load global environment

	unsigned int tmp;
	S.IO(tmp); // load number of threads
	
	for (unsigned i=0; i<tmp; ++i)
	{
#	ifdef DEBUG
		S.magic("[THREAD]");
#	endif
		CLValue thr = CLValue::load(S);
#	ifdef DEBUG
		S.magic("[THREAD-END]");
#	endif
	}

#ifdef DEBUG
	S.magic("[CONTEXT-END]");
#endif
}

////////////////////////////////////////////////////////////////////////////////
// Garbage collection main routines                                           //
////////////////////////////////////////////////////////////////////////////////

void CLContext::markObjects()
{
	// mark root table
	roottable.markObject();

	// mark all running threads
	std::list<CLValue>::iterator it = threads.begin(), end = threads.end();
	for (;it!=end;++it) if (GET_THREAD(*it)->isRunning()) it->markObject();
}

void CLContext::sweepObjects()
{
redo:
	CLCollectable *it = gc_heap_list;
	while (it)
	{
		if (!it->gc_isMarked() && !it->gc_isLocked()) 
		{
			it->gc_finalize();
			moveToFinalizedList(it);
			goto redo; // finalization invalidates iterator 'it'
		}
		it = it->next;
	}
}

void CLContext::addToHeapList(CLCollectable *C)
{
	assert(C);

	// add to heap list
	assert(C->prev == 0);
	assert(C->next == 0);

	C->prev = 0;
	C->next = gc_heap_list;
	gc_heap_list = C;

	if (gc_heap_list->next) gc_heap_list->next->prev = gc_heap_list;
}

void CLContext::moveToFinalizedList(CLCollectable *C)
{
	assert(C);

	// remove from heap list
	if (C->prev) C->prev->next = C->next;
	if (C->next) C->next->prev = C->prev;
	if (C == gc_heap_list) gc_heap_list = C->next;

	// add to finalized list
	C->prev = 0;
	C->next = gc_finalized_list;
	gc_finalized_list = C;
	if (gc_finalized_list->next) gc_finalized_list->next->prev = gc_finalized_list;
}

void CLContext::freeFinalized()
{
	CLCollectable *it = gc_finalized_list;
	while (it)
	{
		CLCollectable *del = it;
		it = it->next;
		delete del;
	}

	gc_finalized_list = 0;
}

void CLContext::unmarkObjects()
{
	CLCollectable *it = gc_heap_list;
	while (it)
	{
		it->marked = false;
		it = it->next;
	}
}


