#pragma once
#include "libdb/Variant.h"
namespace NDb
{
	class CBinaryFlags
	{
		DWORD flags[2];
	public:
		CBinaryFlags() {}
		CBinaryFlags( DWORD _flags[2] ) { memcpy(flags, _flags, 8); }
		//
		operator CVariant() { return CVariant(this, sizeof(*this)); }
		operator const CVariant() const { return CVariant(this, sizeof(*this)); }
	};
}
