#pragma once

#include "libdb/Variant.h"

#include <cstdint>
#include <cstring>

namespace NDb
{
	// TestType.cll's Flags: a hexbinary of numBytes = 8 renamed to this type. The
	// database reads and writes it as an 8-byte blob, which is what the CVariant
	// conversions hand it.
	class CBinaryFlags
	{
		uint32_t flags[2];
	public:
		CBinaryFlags() { flags[0] = flags[1] = 0; }
		CBinaryFlags( const uint32_t _flags[2] ) { memcpy( flags, _flags, sizeof( flags ) ); }
		//
		operator CVariant() const { return CVariant( this, sizeof( *this ) ); }
	};
}
