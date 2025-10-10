#pragma once

struct STilesHash
{
	int operator()( const SVector &tile ) const 
	{ 
		NI_ASSERT( tile.x >=0 && tile.y >= 0 && tile.x <= 4095 && tile.y <= 4095, StrFmt( "Can't hash tile ( %d, %d )\n", tile.x, tile.y ) );
		return ( tile.x << 12 ) | tile.y;
	}
};

struct SVec2Hash
{
	int operator()( const CVec2 &pos) const 
	{ 
		return ( (int(pos.x)<<16) | int(pos.y) );
	}
};

struct SVec2Equ
{
	bool operator()( const CVec2 &v1, const CVec2 &v2 ) const
	{
		return fabs2( v1.x - v2.x, v1.y - v2.y ) < 0.0000001f;
	}
};

struct SEnumHash
{
	template <class T>
		int operator()( const T a ) const { return int( a ); }
};

struct SGUIDHash
{
	int operator()( const GUID a ) const 
	{ 
		DWORD *__s = (DWORD*)(&a);
		DWORD __h = *__s; 
		for ( int i = 1; i < 4; ++i)
			__h ^= __s[i];
		return __h;
	}
};


