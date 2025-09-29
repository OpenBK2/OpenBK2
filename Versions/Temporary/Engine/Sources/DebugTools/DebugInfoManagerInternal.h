#pragma once
#include "DebugInfoManager.h"
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace NDebugInfo
{
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
enum EDebugInfoUpdate
{
	DEBUG_INFO_UNKNOWN,
	DEBUG_INFO_MARKER,
	DEBUG_INFO_CIRCLE,
	DEBUG_INFO_SEGMENT,
	DEBUG_INFO_LINE,
	DEBUG_INFO_RECT,
	DEBUG_INFO_DELETE,
	DEBUG_INFO_DELETE_LINE,
};
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct SDebugInfoUpdate : public CObjectBase
{
  int nID;

	virtual const EDebugInfoUpdate GetDebugInfoUpdateID() const  { return DEBUG_INFO_UNKNOWN; }

	SDebugInfoUpdate() : nID( NDebugInfo::OBJECT_ID_GENERATE ) {}
	SDebugInfoUpdate( const int _nID ) : nID( _nID ) {}
};
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct SDebugInfoObject : public SDebugInfoUpdate
{
	EColor color;

	virtual const EDebugInfoUpdate GetDebugInfoUpdateID() const  { return DEBUG_INFO_UNKNOWN; }

	SDebugInfoObject() : SDebugInfoUpdate(), color( WHITE ) {}
	SDebugInfoObject( const int _nID, const EColor _color ) : SDebugInfoUpdate( _nID ), color( _color ) {}
};
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct SDebugInfoMarker : public SDebugInfoObject
{
	OBJECT_NOCOPY_METHODS( SDebugInfoMarker );
public:
	vector<SVector> tiles;

	virtual const EDebugInfoUpdate GetDebugInfoUpdateID() const  { return DEBUG_INFO_MARKER; }

	SDebugInfoMarker() : SDebugInfoObject() {}
	SDebugInfoMarker( const int nID, const vector<SVector> &_tiles, const EColor color ) : SDebugInfoObject( nID, color ), tiles( _tiles ) {}
};
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct SDebugInfoCircle : public SDebugInfoObject
{
	OBJECT_NOCOPY_METHODS( SDebugInfoCircle );
public:
	CCircle circle;

	virtual const EDebugInfoUpdate GetDebugInfoUpdateID() const  { return DEBUG_INFO_CIRCLE; }

	SDebugInfoCircle() : SDebugInfoObject() {}
	SDebugInfoCircle( const int nID, const CCircle _circle, const EColor color ) : SDebugInfoObject( nID, color ), circle( _circle ) {}
};
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct SDebugInfoSegment : public SDebugInfoObject
{
	OBJECT_NOCOPY_METHODS( SDebugInfoSegment );
public:
	CSegment segment;
	int nThickness;

	virtual const EDebugInfoUpdate GetDebugInfoUpdateID() const  { return DEBUG_INFO_SEGMENT; }

	SDebugInfoSegment() : SDebugInfoObject(), nThickness( -1 ) {}
	SDebugInfoSegment( const int nID, const CSegment &_segment, const int _nThickness, const EColor color ) : SDebugInfoObject( nID, color ),
		segment( _segment ), nThickness( _nThickness ) {}
};
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct SDebugInfoLine : public SDebugInfoUpdate
{
	OBJECT_NOCOPY_METHODS( SDebugInfoLine );
public:
	SArrowHead arrowStart;
	SArrowHead arrowEnd;
	CVec4 vColor;
	bool bDepthCheck;

	virtual const EDebugInfoUpdate GetDebugInfoUpdateID() const  { return DEBUG_INFO_LINE; }

	SDebugInfoLine() : vColor( CVec4( 255, 255, 255, 255 ) ), bDepthCheck( false ) {}
	SDebugInfoLine( const int nID, const NDebugInfo::SArrowHead &_arrowStart, const NDebugInfo::SArrowHead &_arrowEnd, const CVec4 &_vColor, const bool _bDepthCheck ) :
		SDebugInfoUpdate( nID ), arrowStart( _arrowStart ), arrowEnd( _arrowEnd ), vColor( _vColor ), bDepthCheck( _bDepthCheck ) {}
};
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct SDebugInfoRect : public SDebugInfoUpdate
{
	OBJECT_NOCOPY_METHODS( SDebugInfoRect );
public:
	SRect rect;
	CVec4 vColor;
	float fZ;
	bool bDepthCheck;

	virtual const EDebugInfoUpdate GetDebugInfoUpdateID() const  { return DEBUG_INFO_RECT; }

	SDebugInfoRect() : vColor( CVec4( 255, 255, 255, 255 ) ), fZ( 0.0f ), bDepthCheck( false ) { rect.InitRect( VNULL2, VNULL2, VNULL2, VNULL2 ); }
	SDebugInfoRect( const int nID, const SRect _rect, const float _fZ, const CVec4 &_vColor, const bool _bDepthCheck ) :
		SDebugInfoUpdate( nID ), rect( _rect ), fZ( _fZ ), vColor( _vColor ), bDepthCheck( _bDepthCheck ) {}
};
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct SDebugInfoDeleteObject : public SDebugInfoUpdate
{
	OBJECT_NOCOPY_METHODS( SDebugInfoDeleteObject );
public:
	virtual const EDebugInfoUpdate GetDebugInfoUpdateID() const  { return DEBUG_INFO_DELETE; }

	SDebugInfoDeleteObject() : SDebugInfoUpdate() {}
	SDebugInfoDeleteObject( const int nID ) : SDebugInfoUpdate( nID ) {}
};
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct SDebugInfoDeleteLine : public SDebugInfoUpdate
{
	OBJECT_NOCOPY_METHODS( SDebugInfoDeleteLine );
public:
	virtual const EDebugInfoUpdate GetDebugInfoUpdateID() const  { return DEBUG_INFO_DELETE_LINE; }

	SDebugInfoDeleteLine() : SDebugInfoUpdate() {}
	SDebugInfoDeleteLine( const int nID ) : SDebugInfoUpdate( nID ) {}
};
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CDebugInfoManager : public IDebugInfoManager
{
	OBJECT_NOCOPY_METHODS( CDebugInfoManager );

	int nCurrentID;
	int nRedAxisID, nGreenAxisID, nBlueAxisID;
	NDebugInfo::EColor currentColor;
	list< CPtr<NDebugInfo::SDebugInfoUpdate> > updates;

  const int GetID( const int nID );
	const int PushUpdate( NDebugInfo::SDebugInfoUpdate *pObject );
	const CVec4 Color2CVec4( const NDebugInfo::EColor color ) const;

public:
	CDebugInfoManager() { Reset(); }

	void Reset();

	int CreateMarker( const int nID, const vector<SVector> &tiles, const NDebugInfo::EColor eColor );
	int CreateCircle( const int nID, const CCircle &circle, const NDebugInfo::EColor eColor );
	int CreateSegment( const int nID, const CSegment &segment, const int nThickness, const NDebugInfo::EColor eColor );
	void DeleteObject( const int nID );

	int DrawLine( const int nID, const NDebugInfo::SArrowHead &arrowStart, const NDebugInfo::SArrowHead &arrowEnd, const CVec4 &vColor, const bool bDepthCheck );
	int DrawLine( const int nID, const NDebugInfo::SArrowHead &arrowStart, const NDebugInfo::SArrowHead &arrowEnd, const NDebugInfo::EColor eColor );
	int DrawLine( const int nID, const CVec2 &vStart, const CVec2 &vEnd, const bool bArrowEnd, const float fZ, const CVec4 &vColor );
	int DrawRect( const int nID, const SRect &rect, const float fZ, const CVec4 &vColor, const bool bDepthCheck );
	int DrawRect( const int nID, const SRect &rect, const float fZ, const NDebugInfo::EColor eColor );
	void RemoveLine( const int nID );

	//показать оси из точки (0; 0; 0), Красная - X, Зеленая - Y, Синяя - Z
	void ShowAxes( const bool bShow );

	//возвращает последовательно цвета: Красный, Синий, Зеленый, Красный ...
	NDebugInfo::EColor GetCycleColor();

	const NDebugInfo::SDebugInfoUpdate *GetUpdate() const;
	const bool PopUpdate();
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
