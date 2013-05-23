#include "vm/clthread.h"
#include "vm/clcontext.h"
#include "vm/clmodule.h"
#include "vm/clmathmodule.h"

#include "value/clfunction.h"
#include "value/clexternalfunction.h"
#include "value/clstring.h"
#include "value/cltable.h"
#include "value/clarray.h"

#include <assert.h>

#include <algorithm>
#include <iostream>
#include <sstream>

using namespace std;

CLThread::CLThread()
	: do_yield(true), state(CLThread::UNINITIALIZED), result(CLValue::Null()), inside_run_method(0), 
	  linenum(-1), filename("<input>")
{
	// register thread in context
	CLContext::inst().registerThread(CLValue(this));
}

CLThread::~CLThread()
{
	kill();

	// unregister thread in context
	CLContext::inst().unregisterThread(CLValue(this));
}

void CLThread::runtimeError(std::string err, bool fatal)
{
	if (fatal) {
		clog << filename << "(" << linenum << "): Fatal runtime error; " << err << endl;
		clog << "=> Killed thread." << endl;
		kill();
	} else {
		clog << filename << "(" << linenum << "): Runtime error; " << err << endl;
	}
}

bool CLThread::isRunning()
{
	return state == RUNNING;
}

bool CLThread::isInitialized()
{
	return state != UNINITIALIZED;
}


void CLThread::enableYield(bool yes)
{
	do_yield = yes;
}

void CLThread::init(CLValue fn, std::vector<CLValue> args, CLValue self)
{
	// TODO: Proper error handling
	assert(state == UNINITIALIZED);

	// fake function call

	// push function & self value
	stackPush(fn);
	stackPush(self.isNull() ? CLContext::inst().getRootTable() : self);

	// push function arguments and argc
	for (size_t i=0; i<args.size(); ++i) stackPush(args[i]);
	stackPush(CLValue(static_cast<int>(args.size())));

	state = RUNNING;

	op_mcall();
}

void CLThread::suspend()
{
	if (state == RUNNING)
	{
		state = SUSPENDED;
	}
}

void CLThread::resume()
{
	if (state == SUSPENDED)
	{
		state = RUNNING;
	}
}

void CLThread::run(int timeout)
{
	// this function is not reentrant.
	if (inside_run_method) assert(0); // TODO: Proper handling
	inside_run_method = true;

	CallInfo *ci = 0;
	CLFunction *fn = 0;
	std::vector<CLInstruction> *code = 0;

	result.setNull();

redo:
	if (state != RUNNING) goto done; // reason for this: op_ret, op_mcall might kill/exit/suspend thread

	ci   = &callstackTop();
	fn   = GET_FUNCTION(ci->func);
	code = &fn->code;

	for (;;)
	{
		// timeout?
		if ((timeout != -1) && (0 == timeout--)) goto done;

		// fetch instruction
		CLInstruction *inst = &(*code)[ci->ip];

		// increase instruction pointer
		++(ci->ip);

		switch (inst->op)
		{
			// No operation
			case OP_NOP: break;
			
			// Push constants to stack/pop stack/duplicate stack
			case OP_PUSH0:       stackPush(CLValue()); break;                                       // push null
			case OP_PUSHROOT:    stackPush(CLContext::inst().getRootTable()); break;                // push root table
			case OP_PUSHSELF:    stackPush(ci->self); break;                                        // push self
			case OP_PUSHCONST:   stackPush(fn->constants[inst->arg]); break;                        // push constant
			case OP_PUSHEXTFUNC: stackPush(CLValue(new CLExternalFunction(inst->arg_str))); break;  // push external function
			case OP_PUSHI:       stackPush(CLValue(inst->arg)); break;                              // push integer
			case OP_PUSHF:       stackPush(CLValue(inst->arg_float)); break;                        // push float	
			case OP_PUSHS:       stackPush(CLValue(new CLString(inst->arg_str))); break;            // push string

			case OP_POP: for (int i=0; i<inst->arg; ++i) stackPop(); break;                         // discard <arg> values from stack

			case OP_DUP: stackDup(inst->arg); break;                                                // duplicate value at offset i

			// Local variables
			case OP_PUSHL: stackPush(ci->locals[inst->arg]); break;                                   // push local variable
			case OP_POPL: ci->locals[inst->arg] = stackPop(); break;                                  // pop to local variable
			case OP_ADDL: ci->locals.resize(ci->locals.size() + inst->arg, CLValue(12345678)); break; // add n local variables
			case OP_DELL: ci->locals.erase(ci->locals.end() - inst->arg, ci->locals.end()); break;    // del n local variables

			// Operations
#define BINARY_OP(m) {\
	CLValue op2 = stackPop();\
	CLValue op1 = stackPop();\
	stackPush(op1.op##m(op2));\
}

#define UNARY_OP(m) {\
	stackPush(stackPop().op##m());\
}

			case OP_NEG:    UNARY_OP(_neg);     break; // unary -

			case OP_ADD:    BINARY_OP(_add);    break; // operator +
			case OP_SUB:    BINARY_OP(_sub);    break; // operator -
			case OP_MUL:    BINARY_OP(_mul);    break; // operator *
			case OP_DIV:    BINARY_OP(_div);    break; // operator /

			case OP_SHL:    BINARY_OP(_shl);    break; // operator <<
			case OP_SHR:    BINARY_OP(_shr);    break; // operator >>
			case OP_MODULO: BINARY_OP(_modulo); break; // operator %
			case OP_BITOR:  BINARY_OP(_bitor);  break; // operator |
			case OP_BITAND: BINARY_OP(_bitand); break; // operator & 
			case OP_BITXOR: BINARY_OP(_bitxor); break;

			case OP_AND:    BINARY_OP(_booland);break; // boolean and
			case OP_OR:     BINARY_OP(_boolor); break; // boolean or
			case OP_NOT:    UNARY_OP (_boolnot);break; // boolean not

			case OP_EQ:     BINARY_OP(_eq);     break; // ==
			case OP_NEQ:    stackPush(stackPop().op_eq(stackPop()).op_boolnot()); break; // !=
			case OP_LT:     BINARY_OP(_lt);     break; // <
			case OP_GT:     BINARY_OP(_gt);     break; // >
			case OP_LE:     BINARY_OP(_le);     break; // <=
			case OP_GE:     BINARY_OP(_ge);     break; // >=
#undef UNARY_OP
#undef BINARY_OP

			// Table/Array constructor
			case OP_NEWTABLE: stackPush(CLValue(new CLTable())); break; // create new table on stack
			case OP_NEWARRAY: stackPush(CLValue(new CLArray())); break; // create new array on stack

			// Get/Set/Iterator operations
			case OP_TABSET: 
			{ 
				CLValue v = stackPop(); CLValue k = stackPop(); CLValue t = stackPop();
				if (t.type & CL_RAW_ISOBJECT)
				{
					t.set(k, v); // GET_OBJECT(t)->set(k, v);
				} else {
					runtimeError(std::string("Can't set property '") + k.toString() + "' of non-object '" + t.toString() + "'");
				}
				stackPush(v);
				break;
			}

			case OP_TABGET: 
			{
				CLValue k = stackPop();
				CLValue t = stackPop();
				if (t.type & CL_RAW_ISOBJECT)
				{
					stackPush(t.get(k));
				} else {
					runtimeError(std::string("Can't get property '") + k.toString() + "' of non-object '" + t.toString() + "'");
					stackPush(CLValue::Null());
				}
				break;
			}

			case OP_TABGET2: 
			{
				CLValue k = stackPop();
				CLValue t = stackPop();

				if (t.type & CL_RAW_ISOBJECT)
				{
					stackPush(t.get(k));
				} else {
					runtimeError(std::string("Can't get property '") + k.toString() + "' of non-object '" + t.toString() + "'");
					stackPush(CLValue::Null());
				}

				stackPush(t);
				break;
			}

			case OP_TABIT:
			{
				CLValue t = stackPop();
				stackPush(t);
				if (t.type & CL_RAW_ISOBJECT)
				{
					stackPush(GET_OBJECT(t)->begin());
				} else {
					runtimeError(std::string("Can't iterate over '") + t.toString() + "'");
					stackPush(CLValue::Null());
				}
				break;
			}

			case OP_TABNEXT:
			{
				CLValue it = stackPop();
				CLValue t = stackPop();
				CLValue key, val;
				if (t.type & CL_RAW_ISOBJECT)
				{
					it = GET_OBJECT(t)->next(it, key, val);
				} else {
					runtimeError(std::string("Can't iterate over '") + t.toString() + "'");
					it = CLValue::Null();
				}
				stackPush(t);
				stackPush(it);	
				stackPush(val);
				stackPush(key);
				break;
			}

			// Clone operator
			case OP_CLONE: stackPush(stackPop().clone()); break;

			// Branches
			case OP_JMP:  ci->ip = inst->arg; break;
			case OP_JMPT: if (stackPop().isTrue()) ci->ip = inst->arg; break;
			case OP_JMPF: if (stackPop().isFalse()) ci->ip = inst->arg; break;

			// Function call/return/yield
			case OP_MCALL: op_mcall(); goto redo;
			case OP_RET: op_ret(); goto redo; 
			case OP_YIELD: 
				result = stackPop(); 
				if (do_yield) goto done;
				result.setNull();
				break;

			// Debug info
			case OP_FILE:
				this->filename = inst->arg_str;
				break;

			case OP_LINE:
				this->linenum = inst->arg;
				break;
		}
	}

done:
	inside_run_method = false;
}

void CLThread::kill()
{
	result.setNull();
	callstack.clear(); // empty callstack. !!
	stk.clear(); // empty stack
	state = DONE;
}

void CLThread::op_mcall()
{
	CLValue func, argc, self;	
	std::vector<CLValue> args;
	
	argc = stackPop(); assert(argc.type == CL_INTEGER); // TODO: Proper error handling
	args.resize(GET_INTEGER(argc));
	for (int i=0; i<GET_INTEGER(argc); ++i) args[GET_INTEGER(argc)-i-1] = stackPop();
	self = stackPop();
	func = stackPop();
	
	//cout << "argc.... : " << GET_INTEGER(argc) << endl;
	//cout << "func.... : " << GET_STRING(func.toString())->get() << endl;

	switch (func.type)
	{
		case CL_FUNCTION:
			// throw away arguments or create default null ones
			args.resize(GET_FUNCTION(func)->num_args, CLValue::Null()); 
			callstackPush(func, self, args);
			break;

		case CL_EXTERNALFUNCTION:
		{
			CLExternalFunctionPtr fn = GET_EXTERNALFUNCTION(func)->getExternalFunctionPtr();
			if (fn)
			{
				CLValue result = fn(*this, args, self);
				stackPush(result);
			} else {
				runtimeError(std::string("Could not resolve external function '") + func.toString() + "', ignoring call");
				stackPush(CLValue::Null());
			}
			break;
		}

		default:
			runtimeError(std::string("Ignoring call of uncallable value '") + func.toString() + "'");
			stackPush(CLValue::Null());
			break;
	}
}

void CLThread::op_ret()
{
#ifdef DEBUG
	if (!callstackTop().locals.empty())
	{
		cout << "Internal error: Undestroyed locals at function return" << callstack.back().locals.size() << endl;
	}
#endif
	callstackPop();

	if (callstack.empty()) // thread has finished?
	{
		// fetch result from stack
		result = stackPop();
#ifdef DEBUG
		if (!stk.empty())
		{
			cout << "Internal error: Stack not empty after function return:" << stk.size() << " items left" << endl;
		}
#endif
		state = DONE;
	}
}

// Serialization /////////////////////////////////////////////

void CLThread::save(CLSerializer &S, CLThread *thread)
{
	assert(!thread->inside_run_method); //FIXME: Hmmm...
	unsigned int tmp;

	S.IO(tmp = thread->state); 			// ThreadState state
	CLValue::save(S, thread->result); 		// CLValue result
	CLValue::saveVector(S, thread->stk);		// std::vector<CLValue> stk;

	S.IO(tmp = thread->callstack.size());		// callstack.size()
	for (size_t i=0; i<thread->callstack.size(); ++i)	// struct CallInfo 
	{
		S.IO(tmp = thread->callstack[i].ip);
		CLValue::save(S, thread->callstack[i].func);
		CLValue::save(S, thread->callstack[i].self);
		CLValue::saveVector(S, thread->callstack[i].locals);
	}

	S.IO(tmp = thread->linenum);
	S.IO(thread->filename);
}

CLThread *CLThread::load(CLSerializer &S)
{
	unsigned int tmp;

	CLThread *thread = new CLThread(); S.addPtr(thread);

	S.IO(tmp); thread->state = ThreadState(tmp);	// ThreadState state
	thread->result = CLValue::load(S);		// CLValue result
	thread->stk = CLValue::loadVector(S);		// std::vector<CLValue> stk
	
	S.IO(tmp); thread->callstack.resize(tmp);	// callstack.size()
	for (size_t i=0; i<thread->callstack.size(); ++i)	// struct CallInfo 
	{
		S.IO(thread->callstack[i].ip);
		thread->callstack[i].func = CLValue::load(S);
		thread->callstack[i].self = CLValue::load(S);
		thread->callstack[i].locals = CLValue::loadVector(S);
	}

	S.IO(thread->linenum);
	S.IO(thread->filename);

	return thread;
}

// from CLObject /////////////////////////////////////////////

void CLThread::set(CLValue &key, CLValue &val)
{
}

bool CLThread::get(CLValue &key, CLValue &val)
{
	const std::string &s = GET_STRING(key)->get();

	if (s == "kill") {
		val = CLValue(new CLExternalFunction("sys_thread_kill")); return true;
	} else if (s == "isrunning") {
		val = CLValue(new CLExternalFunction("sys_thread_isrunning")); return true;
	} else if (s == "suspend") {
		val = CLValue(new CLExternalFunction("sys_thread_suspend")); return true;
	} else if (s == "resume") {
		val = CLValue(new CLExternalFunction("sys_thread_resume")); return true;
	} else if (s == "result") {
		val = this->result; return true;
	}

	return false;
}


// iteration support:
CLValue CLThread::begin()
{
	return CLValue::Null();
}

CLValue CLThread::next(CLValue iterator, CLValue &key, CLValue &value)
{
	assert(0); // can't happen
	return CLValue::Null();
}

// from CLCollectable ////////////////////////////////////////
void CLThread::gc_mark()
{
	if (gc_isMarked()) return;
	gc_setMarked();

	// mark result value
	result.markObject();

	// mark references in callstack
	size_t cs_size = callstack.size();
	for (size_t i=0; i<cs_size; ++i)
	{
		CallInfo &ci = callstack[i];
		ci.func.markObject();
		ci.self.markObject();
		for (size_t x=0; x<ci.locals.size(); ++x) ci.locals[x].markObject();
	}

	// mark stack
	for (size_t i=0; i<stk.size(); ++i)
	{
		stk[i].markObject();
	}
}

//////////////////////////////////////////////////////////////

CLValue CLThread::clone()
{
	cout << "Cloning threads not supported!" << endl;
	return CLValue::Null();
}

std::string CLThread::toString()
{
	std::stringstream ss; ss << "<thread@" << (void*)this << ">";
	return ss.str();
}


