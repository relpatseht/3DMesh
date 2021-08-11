#pragma once

#include <cstdint>
#include <malloc.h>
#include <memory>
#include "sanity.h"

struct BitSet
{
	uint64_t* bits;
	unsigned bitCount;
	unsigned qwordCount;
};

#define BitSet_Create(SetPtr, Count) 										 \
	{																		 \
		const unsigned qwordCount = ((Count)+63) >> 6;						 \
		(SetPtr)->bits = (uint64_t*)_malloca(sizeof(uint64_t) * qwordCount); \
		(SetPtr)->bitCount = (unsigned)Count;								 \
		(SetPtr)->qwordCount = qwordCount;                                   \
	}

static __forceinline void BitSet_Destroy(BitSet* set)
{
	_freea(set->bits);
	set->bitCount = 0;
	set->qwordCount = 0;
	set->bits = nullptr;
}

static __forceinline void BitSet_ClearAll(BitSet* set)
{
	memset(set, 0, sizeof(uint64_t) * set->qwordCount);
}

static __forceinline bool BitSet_IsSet(const BitSet& set, unsigned bit)
{
	sanity(bit < set.bitCount);
	return set.bits[bit >> 6] & (bit & 0x3F);
}

static __forceinline void BitSet_Set(BitSet* set, unsigned bit)
{
	sanity(bit < set->bitCount);
	set->bits[bit >> 6] |= 1ull << (bit & 0x3F);
}

static __forceinline void BitSet_Toggle(BitSet* set, unsigned bit)
{
	sanity(bit < set->bitCount);
	set->bits[bit >> 6] ^= 1ull << (bit & 0x3F);
}

static __forceinline void BitSet_Clear(BitSet* set, unsigned bit)
{
	sanity(bit < set->bitCount);
	set->bits[bit >> 6] &= ~(1ull << (bit & 0x3F));
}

struct ScopedBitSet : public BitSet
{
	~ScopedBitSet()
	{
		BitSet_Destroy(this);
	}
};
