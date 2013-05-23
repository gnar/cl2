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


#ifndef CLCONTEXT_H
#define CLCONTEXT_H

#include "value/clvalue.h"
#include "vm/clthread.h"
#include "vm/clmodule.h"
#include "vm/clsysmodule.h"

#include <list>
#include <string>

#ifdef DEBUG
extern int ocount;
#endif

class CLUserDataSerializer;

class CLContext
{
public:
	CLContext();
	~CLContext();

	// singleton getter
	static CLContext &inst();

	//
	inline CLValue &getRootTable() { return roottable; }

	int countRunningThreads();

	void roundRobin(int timeout = -1);

	// Modules
	void addModule(CLModule *module);
	void removeModule(CLModule *module);
	CLExternalFunctionPtr getExternalFunctionPtr(const std::string &func_id);

	// Save, Load, Clear complete context
	void clear();
	void save(class CLSerializer &S);
	void load(class CLSerializer &S);

private:
	CLValue roottable; // Global variables

	// Threads
	friend class CLThread;
	void registerThread(CLValue thread);
	void unregisterThread(CLValue thread);
	std::list<CLValue> threads;

	// Modules
	std::list<CLModule*> modules;
	CLSysModule sys;

	// GC lists
	std::list<CLCollectable*> gc_visible; // List of objects which are always visible
	CLCollectable *gc_heap_list;          // Chained list of all collectible objects on heap (via CLCollectable::next)
	CLCollectable *gc_finalized_list;     // Chained list of all finalized objects awaiting destruction (via CLCollectable::next)

	friend class CLCollectable;
	void addToHeapList(CLCollectable *C); // add object to heap list
	void moveToFinalizedList(CLCollectable *C); // move object from heap list to finalized list

	// Singleton instance
	static CLContext *instance;

	// Called by destruktor and clear()
	void shutdown();

public:
	// GC
	void markObjects();
	void unmarkObjects();
	void sweepObjects();
	void freeFinalized();
};

#endif

