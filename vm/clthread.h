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


#ifndef CLTHREAD_H
#define CLTHREAD_H

#include "value/clobject.h"
#include "value/clvalue.h"

#include <vector>
#include <string>

#include <assert.h>

#ifdef DEBUG
extern unsigned long icount;
#endif

class CLThread : public CLObject
{
public:
	CLThread();
	virtual ~CLThread();

	void enableYield(bool yes = false);

	void init(CLValue fn, std::vector<CLValue> = std::vector<CLValue>(), CLValue self = CLValue::Null());
	void run(int timeout = -1);
	void kill();
	void suspend();
	void resume();

	bool isRunning();
	bool isInitialized();
	bool isSuspended() { return state == SUSPENDED; }

	CLValue getResult() { return result; }

	void runtimeError(std::string err, bool fatal = false); // display runtime error and kill thread if fatal

private:
	bool do_yield;

	enum ThreadState
	{
		UNINITIALIZED=0,
		RUNNING=1,
		DONE=2,
		SUSPENDED=3,
	};
	ThreadState state;

	std::vector<CLValue> stk;

	inline void stackPush(const CLValue &v) { stk.push_back(v); }
	inline CLValue stackPop()               { CLValue v = *(stk.end()-1); stk.pop_back(); return v; }
	inline CLValue &stackGet()              { return *(stk.end()-1); }
	inline void stackDup(int offset)        { stk.push_back(*(stk.rbegin()+offset)); }

	struct CallInfo
	{
		CallInfo(CLValue func, CLValue self, std::vector<CLValue> &locals) 
			: ip(0), func(func), self(self), locals(locals) {}
		CallInfo()
			: ip(0) {}
		~CallInfo() {}

		unsigned ip;                 // instruction pointer
		CLValue func;                // current function
		CLValue self;                // 'self' context
		std::vector<CLValue> locals; // local variables, including function arguments
	}; 
	std::vector<CallInfo> callstack;

	inline void callstackPush(CLValue &fn, CLValue &self, std::vector<CLValue> &locals)
	{
		callstack.push_back(CallInfo(fn, self, locals)); 
	}
	inline void callstackPop()                            { callstack.pop_back(); }
	inline CallInfo &callstackTop()                       { return *(callstack.end()-1); }

	void op_mcall();
	void op_ret();

	CLValue result; // yield result or null if RUNNING, return result if DONE

	bool inside_run_method; // prevents the run() method from being called recursively


private:
	// from CLObject /////////////////////////////////////////////

	// access values by key (= integers) ..
	virtual void set(CLValue &key, CLValue &val);
	virtual bool get(CLValue &key, CLValue &val);

	// iteration support:
	virtual CLValue begin();
	virtual CLValue next(CLValue iterator, CLValue &key, CLValue &value);

	// clone
	virtual CLValue clone();

	// to string
	virtual std::string toString();

        // from CLCollectable ////////////////////////////////////////
	void gc_mark();

	// debug info ////////////////////////////////////////////////
	int linenum;
	std::string filename;

	//////////////////////////////////////////////////////////////

public:
	static void save(class CLSerializer &S, CLThread *thread);
	static CLThread *load(class CLSerializer &S);
};

#endif

