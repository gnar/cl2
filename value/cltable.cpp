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

#include "value/cltable.h"
#include "value/clstring.h"

#include "serialize/clserializer.h"

#include <assert.h>
#include <string>
#include <sstream>

CLTable::CLTable() : slots(0), reserved(0)
{
	clear();
}

CLTable::~CLTable()
{
	delete [] slots;
}

void CLTable::clear()
{
	delete [] slots;

	size = MIN_SIZE;
	if (size < reserved) size = reserved;

	fill = 0;
	slots = new Slot[size];
	free_slot = &slots[size-1];
}

void CLTable::reserve(size_t reserve_size)
{
	reserved = MIN_SIZE;
	while (reserved < reserve_size) reserved *= 2;

	Resize();
}


CLTable::HashKey_t CLTable::Hash(CLValue &key)
{
	switch (key.type)
	{
		case CL_STRING:  return (HashKey_t)(GET_STRING(key)->hash());
		case CL_INTEGER: return (HashKey_t)(GET_INTEGER(key));
		default: return 0;
	}
}

void CLTable::Resize()
{
	size_t new_size = size;

	if (fill >= size - size/4) // using more than 3/4 of slots?
		new_size = size * 2;
	else if (fill < size/4) // using less than 1/4 of slots?
		new_size = size / 2;

	Resize(new_size);
}

void CLTable::Resize(size_t new_size)
{
	if (new_size < MIN_SIZE) new_size = MIN_SIZE;
	if (new_size < reserved) new_size = reserved;

	if (new_size == size) return;

	size_t old_size = size;
	Slot *old_slots = slots;

	size = new_size;
	fill = 0;
	slots = new Slot[size];
	free_slot = &slots[size-1];

	for (size_t i=0; i<old_size; ++i)
	{
		if (!IsSlotFree(&old_slots[i]))
		{
			set(old_slots[i].key, old_slots[i].value);
		}
	}

	delete [] old_slots;
}

CLTable::Slot *CLTable::FindSlot(CLValue &key, CLTable::Slot *s)
{
	while (s)
	{
		if (key.op_eq(s->key).isTrue()) return s;
		s = s->next;
	}
	return 0;
}


bool CLTable::get(CLValue &key, CLValue &value)
{
	// special key: "parent"
	if ((key.type == CL_STRING) && (GET_STRING(key)->get() == "parent"))
	{
		value = parent;
		return true;
	}

	Slot *found = FindSlot(key, GetSlot(Hash(key)));
	if (found)
	{
		value = found->value;
		return true;
	} else {
		// not found? look in parent table..
		if (parent.type == CL_TABLE) return GET_TABLE(parent)->get(key, value);
	}

	return false; // remove compiler warning
}


void CLTable::set(CLValue &key, CLValue &value)
{
	// special keys: "parent", null
	if ((key.type == CL_STRING) && (GET_STRING(key)->get() == "parent"))
	{
		parent = value;
		return;
	} else if (key.isNull()) {
		return;
	}

	// remove slot if new value = null
	if (value.isNull())
	{
		remove(key);
		return;
	}

	HashKey_t hash = Hash(key);
	Slot *main_slot = GetSlot(hash);

	// I. Does the key already exist in this table? -> Just update the value
	Slot *found = FindSlot(key, main_slot);
	if (found)
	{
		found->value = value;
		return;
	}

	// II. Insert key/value pair into table
	// Is the main slot free?
	if (IsSlotFree(main_slot))
	{
		main_slot->key = key;
		main_slot->value = value;
		main_slot->next = 0;
	} else {
		// Collision!
	
		// Get main slot of colliding slot (which is in 'main_slot')
		Slot *coll_main_slot = GetSlot(Hash(main_slot->key));

		// If both main slots are the same, put new data into any free slot, and keep both in the same chain
		if (main_slot == coll_main_slot)
		{
			free_slot->next = main_slot->next;
			main_slot->next = free_slot;
			free_slot->key = key;
			free_slot->value = value;
		} else {
		//      Else, move contents of the colliding slot (in 'main_slot') into a free slot, and store new data (in 'main_slot')
			
			// find previous of colliding slot (begin search at its main position)
			Slot *coll_prev = coll_main_slot;
			while (coll_prev->next != main_slot) coll_prev = coll_prev->next;

			// move colliding node into free slot
			*free_slot = *main_slot;

			// repair chain for colliding slot
			coll_prev->next = free_slot;
			free_slot->next = main_slot->next;

			// now main_slot is free
			main_slot->key = key;
			main_slot->value = value;
			main_slot->next = 0;
		}
	}

	// III. Resize table if necessary, and keep free_slot free.
	++fill;
	if (fill == size) 
	{
		Resize();
	} else {
		// Correct 'free_slot'
		while (!IsSlotFree(free_slot))
		{
			assert(free_slot != &slots[0]);
			--free_slot;
		}
	}
}

CLValue CLTable::clone()
{
	CLTable *dst = new CLTable();
	CLTable *src = this;

	CLValue it = src->begin(), key, value;
	while (it.isTrue())
	{
		it = src->next(it, key, value);
		dst->set(key, value);
	}

	return CLValue(dst);
}

std::string CLTable::toString()
{
	std::stringstream ss;
	ss << "[";

	if (parent.isTrue())
	{
		ss << "parent=" << parent.toString() << ' ';
	}

	for (size_t i=0; i<size; ++i)
	{
		if (!IsSlotFree(&slots[i]))
		{
			ss << slots[i].key.toString() << "=" << slots[i].value.toString() << ' ';
		}
	}

	ss << "]";
	return ss.str();
}

bool CLTable::remove(CLValue &key)
{
	Slot *main_slot = GetSlot(Hash(key));
	Slot *slot = FindSlot(key, main_slot);

	if (!slot) return false; // no node to remove

	Slot *to_clear = slot;

	if (slot == main_slot)
	{
		// the main slot needs to be occupied, if there are chained slots
		if (slot->next != 0)
		{
			to_clear = main_slot->next; // the slot that is copied to main_slot and then cleared instead of 'slot'
			*main_slot = *to_clear;
		}
	} else {
		// find previous
		Slot *prev = main_slot;
		while (prev->next != slot) prev = prev->next;

		// repair chain
		prev->next = slot->next;
	}

	*to_clear = Slot();

	--fill;

	// correct 'free_slot'
	if (free_slot < to_clear) free_slot = to_clear;

	Resize();

	return true;
}

// GC
void CLTable::gc_mark()
{
	if (gc_isMarked()) return;
	gc_setMarked();

	// mark parent
	parent.markObject();

	// mark key/value pairs
	for (size_t i=0; i<size; ++i)
	{
		slots[i].key.markObject();
		slots[i].value.markObject();
	}
}

// iteration support
CLValue CLTable::begin()
{
	for (size_t i=0; i<size; ++i)
	{
		if (!IsSlotFree(&slots[i])) return CLValue((int)i);
	}

	return CLValue();
}

CLValue CLTable::next(CLValue iterator, CLValue &key, CLValue &value)
{
	size_t it = (size_t)GET_INTEGER(iterator);

	// load key/value
	key = slots[it].key;
	value = slots[it].value;

	// increment iterator
	++it;
	for (size_t i=it; i<size; ++i)
	{
		if (!IsSlotFree(&slots[i]))
		{
			return CLValue((int)i);
		}
	}

	return CLValue(); // end reached?
}

//static member
void CLTable::save(CLSerializer &S, CLTable *table)
{
	unsigned tmp;
	S.IO(tmp = table->size); // slot size
	S.IO(tmp = table->reserved); // reserved
	CLValue::save(S, table->getParent()); // parent

	for (size_t i=0; i<table->size; ++i) // slots
	{
		// write slot
		Slot *slot = &table->slots[i];

		CLValue::save(S, slot->key);
		CLValue::save(S, slot->value);

		// save next ptr
		int tmp = -1;
		if ((slot->next != 0) && (!table->IsSlotFree(slot)))
		{
			ptrdiff_t next = (slot->next - table->slots);
			assert(next > 0);
			assert((unsigned)next < table->size);
			tmp = static_cast<int>(next);
		}
		S.IO(tmp);
	}
}

//static member
CLTable *CLTable::load(CLSerializer &S)
{
	CLTable *table = new CLTable(); S.addPtr(table);

	unsigned tmp;
	size_t size, reserved;

	S.IO(tmp); size = tmp; // slot size
	S.IO(tmp); reserved = tmp; // reserved value
	table->setParent(CLValue::load(S)); // parent

	table->reserve(reserved > size ? reserved : size);
	assert(table->size == size); // (!!!)

	// load slots, & correct free_slot and fill
	table->fill = 0;
	table->free_slot = 0;

	for (size_t i=0; i<size; ++i)
	{
		Slot *slot = &table->slots[i];
		
		// load slot
		slot->key = CLValue::load(S);
		slot->value = CLValue::load(S);
		
		int tmp; S.IO(tmp);
		if (tmp == -1)
		{
			slot->next = 0;
		} else {
			slot->next = table->slots + (ptrdiff_t)tmp;
		}

		// correct free_slot & fill
		if (table->IsSlotFree(slot))
		{
			table->free_slot = slot;
		} else {
			++(table->fill);
		}
	}
	assert(table->free_slot);
	assert(table->fill < table->size);

	return table;
}

