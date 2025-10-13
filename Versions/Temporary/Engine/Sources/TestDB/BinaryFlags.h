#pragma once

#include "libdb/Variant.h"

#include <cstdint>

namespace NDb
{
	class CBinaryFlags
	{
		uint32_t flags[2];
	public:
		CBinaryFlags() {}
		CBinaryFlags( uint32_t _flags[2] ) { memcpy(flags, _flags, 8); }
		//
		operator CVariant() { return CVariant(this, sizeof(*this)); }
		operator const CVariant() const { return CVariant(this, sizeof(*this)); }
	};
}
