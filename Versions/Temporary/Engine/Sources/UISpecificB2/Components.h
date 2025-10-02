#pragma once

#include "ui/commandparam.h"
#include "ui/dbuserinterface.h"
#include "3Dmotor/GPixelFormat.h"

class CTextureRoundSegmentVisitor
{
	ZDATA
	CDBPtr<NDb::STexture> pTexture;
	CTRect<float> rect;
	float fStartAngle;
	float fFinishAngle;
	vector<NGfx::SPixel8888> colors;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pTexture); f.Add(3,&rect); f.Add(4,&fStartAngle); f.Add(5,&fFinishAngle); f.Add(6,&colors); return 0; }
private:
	void DrawTriangle( struct IUIVisitor *pVisitor, const CVec2 &v1, const CVec2 &v2, const CVec2 &v3,
		const CVec2 &vTex1, const CVec2 &vTex2, const CVec2 &vTex3 );
	bool ClampAngles( float *pStart, float *pFinish, float fMin, float fMax );
public:
	CTextureRoundSegmentVisitor();
	
	void SetTexture( const NDb::STexture *pTexture );
	void SetColor( DWORD dwColor );
	void SetPlacement( const CTRect<float> &rect );
	// draw only: fStartAngle <= fFinishAngle
	void SetAngles( float fStartAngle, float fFinishAngle );
	
	void Visit( struct IUIVisitor *pVisitor );
};


