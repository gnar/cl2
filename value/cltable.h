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

#ifndef CLTABLE_H
#define CLTABLE_H

#include <string>

#include "value/clobject.h"
#include "value/clvalue.h"

class CLTable : public CLObject
{
public:
	CLTable();
	virtual ~CLTable();

	// set/get parent table
	void setParent(CLValue parent) { this->parent = parent; }
	CLValue getParent() { return this->parent; } 

	// get/set/remove slots
	virtual bool get(CLValue &key, CLValue &value);
	virtual void set(CLValue &key, CLValue &value);
	bool remove(CLValue &key);

	// clone
	virtual CLValue clone();

	// to string..
	virtual std::string toString();
	
	void clear();
	size_t slotsUsed() { return fill; }
	void reserve(size_t min_size);

	// iteration support:
	CLValue begin();
	CLValue next(CLValue iterator, CLValue &key, CLValue &value);

	// load/save
	static void save(CLSerializer &S, CLTable *table);
	static CLTable *load(CLSerializer &S);

private:
	static const size_t MIN_SIZE = 4;

	typedef unsigned int HashKey_t;

	struct Slot
	{
		Slot() : key(), value(), next(0) {}
		~Slot() {}

		CLValue key;
		CLValue value;
		Slot *next;
	};

	Slot *slots; // variable sized slot array
	size_t size; // 'slots' array size
	size_t reserved; // 'size' will never go below this value
	size_t fill; // number of slots filled
	Slot *free_slot; // always points to the first free slot (counting from the top of 'slots' array)
	CLValue parent; // table parent (must be of type CL_TABLE)

	// hash function
	static HashKey_t Hash(CLValue &key);

	// get slot for a given hash-key
	inline Slot *GetSlot(HashKey_t hash) { return &slots[hash % size]; }

	// check if a slot is free
	inline bool IsSlotFree(Slot *slot) { return slot->key.isFalse(); }

	// find slot with equal key in slot chain beginning at 's'
	Slot *FindSlot(CLValue &key, Slot *s);

	// autoresize based on 'fill' and 'size'
	void Resize(); 

	// resize table (but not below 'reserved')
	void Resize(size_t new_size);

	// GC
	virtual void gc_mark();

};

#endif

