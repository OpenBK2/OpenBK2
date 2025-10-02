#pragma once

// для индексации клеток 
struct SIntThree
{
	int x;
	int y;
	int z;
	
	SIntThree() {  }

	SIntThree( const int _x, const int _y, const int _z) : x( _x ), y( _y ), z( _z ) {  }
	SIntThree( const CVec3 &v ) : x( v.x ), y( v.y ), z( v.z ) {  }

	bool operator==(const SIntThree &p)const
	{
		return 0 == memcmp( this, &p, sizeof(*this) );
	}
	bool operator!=( const SIntThree &p ) const
	{
		return !operator==(p);
	}
};


