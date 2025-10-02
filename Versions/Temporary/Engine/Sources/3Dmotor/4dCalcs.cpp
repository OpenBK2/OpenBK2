#include "StdAfx.h"
#include "4dCalcs.h"

CVec3 Get3DDir( const CVec4 &_v1, const CVec4 &_v2 )
{
	CVec4 v1(_v1), v2(_v2);
	if ( v1.w < 0 )
		v1 = -v1;
	if ( v2.w < 0 )
		v2 = -v2;
	return CVec3( 
		v1.x * v2.w - v2.x * v1.w,  
		v1.y * v2.w - v2.y * v1.w,  
		v1.z * v2.w - v2.z * v1.w );
}

CVec4 AddHomogen( const CVec4 &_v1, const CVec4 &_v2 )
{
	return CVec4( 
		_v1.x * _v2.w + _v2.x * _v1.w, 
		_v1.y * _v2.w + _v2.y * _v1.w, 
		_v1.z * _v2.w + _v2.z * _v1.w, 
		_v1.w * _v2.w );
}

