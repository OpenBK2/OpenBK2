#pragma once
enum ELineDispos
{
	SAME,
	PARALLEL,
	INTERSECT,
	NOTINTERSECT
};
typedef std::list< std::vector<CVec2> > TPolygonsList;

bool IsPolygonInverse( const std::vector<CVec2> &vPolygon );
bool IsPointInPolygon( const std::vector<CVec2> &vPolygon, const CVec2 &vPoint );
ELineDispos IntersectLines( const CVec2 &vLine1Beg, const CVec2 &vLine1End, const CVec2 &vLine2Beg, const CVec2 &vLine2End, CVec2 *pvRes );
void ClipPolygon(	const TPolygonsList &vPolygon, const TPolygonsList &vClipPolygon, TPolygonsList *pvIntList, TPolygonsList *pvSubList );
void DumpPolyList( const TPolygonsList &polygonsList );


