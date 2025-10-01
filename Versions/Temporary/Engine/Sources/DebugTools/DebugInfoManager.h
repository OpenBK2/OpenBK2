#pragma once

#include "DebugTools_export.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace NDebugInfo
{
	enum
	{
		OBJECT_ID_GENERATE	= -1,
		OBJECT_ID_FORGET		= 0,
	};

	enum EColor
	{
		BLACK,
		BLUE,
		GREEN,
		RED,
		WHITE,
	};

	struct SArrowHead 
	{
		CVec3 vPosition;
		float fWidth;
		float fHeight;

		SArrowHead() : vPosition( VNULL3 ), fWidth( 0.0f ), fHeight( 0.0f ) {}
		SArrowHead( const CVec3 &_vPosition ) : vPosition( _vPosition ), fWidth( 0.0f ), fHeight( 0.0f ) {}
		SArrowHead( const CVec3 &_vPosition, const float _fWidth, const float _fHeight ) : vPosition( _vPosition ), fWidth( _fWidth ), fHeight( _fHeight ) {}
	};
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
interface IDebugInfoManager : public CObjectBase
{
	enum { tidTypeID = 0x31160C80 };

	virtual int CreateMarker( const int nID, const vector<SVector> &tiles, const NDebugInfo::EColor eColor ) = 0;
	virtual int CreateCircle( const int nID, const CCircle &circle, const NDebugInfo::EColor eColor ) = 0;
	virtual int CreateSegment( const int nID, const CSegment &segment, const int nThickness, const NDebugInfo::EColor eColor ) = 0;
	virtual void DeleteObject( const int nID ) = 0;
	
	//возвращает последовательно цвета: Красный, Синий, Зеленый, Красный ...
	virtual NDebugInfo::EColor GetCycleColor() = 0;

	virtual int DrawLine( const int nID, const NDebugInfo::SArrowHead &arrowStart, const NDebugInfo::SArrowHead &arrowEnd, const CVec4 &vColor, const bool bDepthCheck ) = 0;
	virtual int DrawLine( const int nID, const NDebugInfo::SArrowHead &arrowStart, const NDebugInfo::SArrowHead &arrowEnd, const NDebugInfo::EColor eColor ) = 0;
	virtual int DrawLine( const int nID, const CVec2 &vStart, const CVec2 &vEnd, const bool bArrowEnd, const float fZ, const CVec4 &vColor ) = 0;
	virtual int DrawRect( const int nID, const SRect &rect, const float fZ, const CVec4 &vColor, const bool bDepthCheck ) = 0;
	virtual int DrawRect( const int nID, const SRect &rect, const float fZ, const NDebugInfo::EColor eColor ) = 0;
	virtual void RemoveLine( const int nID ) = 0;
	//показать оси из точки (0; 0; 0), Красная - X, Зеленая - Y, Синяя - Z
	virtual void ShowAxes( const bool bShow ) = 0;
};
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DEBUGTOOLS_EXPORT IDebugInfoManager *CreateDebugInfoManager();
inline IDebugInfoManager *DebugInfoManager() { return Singleton<IDebugInfoManager>(); }
