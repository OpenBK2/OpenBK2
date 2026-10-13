#pragma once

CVec3 Get3DDir( const CVec4 &_v1, const CVec4 &_v2 );
CVec4 AddHomogen( const CVec4 &_v1, const CVec4 &_v2 );
inline CVec4 SubHomogen( const CVec4 &_v1, const CVec4 &_v2 ) { return AddHomogen( _v1, CVec4(-_v2.x, -_v2.y, -_v2.z, _v2.w ) ); }
inline CVec3 SafeUnhomogen( const CVec4 &v )
{
	if ( v.w == 0 )
		return CVec3( v.x, v.y, v.z ) * 1e6f;
	return CVec3( v.x / v.w, v.y / v.w, v.z / v.w );
}
inline CVec3 Unhomogen( const CVec4 &a ) { ASSERT( a.w != 0 ); return CVec3(a.x/a.w, a.y/a.w, a.z/a.w ); }
