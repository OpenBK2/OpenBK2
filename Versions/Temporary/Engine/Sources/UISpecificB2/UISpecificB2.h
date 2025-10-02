#pragma once

#include "UISpecificB2_export.h"


#include "../ui/commandparam.h"
#include "../ui/dbuserinterface.h"
#include "../UI/UI.h"
#include "../3DMotor/Gfx.h"
#include "../Stats_B2_M1/DBNotifications.h"
#include "../Stats_B2_M1/AITypes.h"
#include "../Stats_B2_M1/DBAnimB2.h"
#include "../Misc/Progress.h"

UISPECIFICB2_EXPORT IWindow* AddWindowCopy( IWindow *pParent, const struct NDb::SUIDesc *pDesc );
UISPECIFICB2_EXPORT IWindow* AddWindowCopy( IWindow *pParent, const IWindow *pSample );

struct SMiniMapUnitInfo
{
	WORD x;
	WORD y;
	float z;
	BYTE player;
	BYTE radius; // tiles	

	SMiniMapUnitInfo() : radius( 0 )  { }
	SMiniMapUnitInfo( const WORD _x, const WORD _y, const float _z, const BYTE _player, BYTE _radius ) 
		: x( _x ), y( _y ), z( _z ), player( _player ), radius( _radius ) { }
};

interface IMiniMap : virtual public IWindow
{
	struct SMarker
	{
		ZDATA
		CVec2 vPos;
		CDBPtr<NDb::STexture> pTexture;
		CVec2 vTexturePoint;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&vPos); f.Add(3,&pTexture); f.Add(4,&vTexturePoint); return 0; }
	};
	
	struct SFigure
	{
		ZDATA
		NDb::EMinimapFigureType eType;
		CVec2 vPos;
		float fSize;
		float fAngle;
		NGfx::SPixel8888 color;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&eType); f.Add(3,&vPos); f.Add(4,&fSize); f.Add(5,&fAngle); f.Add(6,&color); return 0; }
	};

	virtual void LoadMap( const int nWidth, const int nHeight, const int nWarFogLevel ) = 0;
	// Set some initial params for loading screen
	virtual void SetLoadingMapParams( int nWidth, int nHeight ) = 0;
	virtual CVec2 GetAIToScreen( const CVec2 &vPos ) const = 0;
	virtual void SetUnits( const vector< SMiniMapUnitInfo > &vUnits ) = 0;
	virtual void SetViewport( const vector< CVec2 > &vPoints ) = 0;
	virtual void SetWarFog( const CArray2D<BYTE> *pWarFogInfo ) = 0;
	virtual void SetMarkers( const vector<SMarker> &markers ) = 0;
	virtual void SetFigures( const vector<SFigure> &figures ) = 0;
	virtual void SetMaterial( CDBPtr< NDb::SMaterial > pMaterial ) = 0;
	virtual void SetTexture( const NDb::STexture *pTexture ) = 0;
	virtual void SetPlayerColor( const int nPlayer, const NGfx::SPixel8888 &color ) = 0;
	virtual void SetAdditionalScale( float fAdditionalScale ) = 0;

	virtual void SetRangeInfo( const int nUnitID, const SShootAreas &rangeInfo ) = 0;
	virtual void RemoveAllRangeInfo() = 0;
	virtual void SetNortDirectionTexture( const NDb::STexture *pTexture ) = 0;
};

interface IPotentialLines : virtual public IWindow
{
	virtual void SetParams( const string &szMask, const string &szDiffColourMap, const CVec2 &vMainStrike, const DWORD _dwBorderColour1, const DWORD _dwBorderColour2 ) = 0;

	virtual void ClearNodes() = 0;
	virtual void SetNode( int nX, int nY, int nEndOffsetX, int nEndOffsetY, float fValue ) = 0;		// If such node exists, it is altered

	virtual void ClearArrows() = 0;
	virtual void AddArrow( const vector<CVec2> &arrowTraj, float fArrowWidth, const NDb::STexture *pArrowTexture, DWORD dwArrowColour ) = 0;
};

interface ISelection : virtual public IWindow
{
};

// Note: no real texture, use color with alpha only
interface IWindowRoundProgressBar : virtual public IProgressBar
{
	// draw only: fStartAngle <= fFinishAngle
	virtual void SetAngles( float fStartAngle, float fFinishAngle ) = 0;
};

interface IWindow3DControl : virtual public IWindow
{
	struct SParam
	{
		ZDATA
		CVec2 vPos;
		CVec2 vSize;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&vPos); f.Add(3,&vSize); return 0; }
	};
	
	struct SObject
	{
		ZDATA
		int nID;
		CDBPtr<NDb::SModel> pModel;
		CVec2 vPos;
		CVec2 vSize;
		CDBPtr<NDb::SAnimB2> pAnim;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&nID); f.Add(3,&pModel); f.Add(4,&vPos); f.Add(5,&vSize); f.Add(6,&pAnim); return 0; }
		
		bool operator==( const SObject &object ) const
		{
			return nID == object.nID && pModel == object.pModel &&
				vPos == object.vPos && vSize == object.vSize && pAnim == object.pAnim;
		}
	};

	virtual void SetObjects( const vector<SObject> &objects ) = 0;
	virtual SParam GetDBObjectParam( int nIndex ) const = 0;
	virtual void SetBaseID3D( int nID ) = 0;
};

interface IWindowFrameSequence : virtual public IWindow
{
	virtual void Run( bool bRun ) = 0;
	virtual void Reset() = 0;
};

interface IProgressHookB2 : public IProgressHook
{
	virtual void RunFinishScreen() = 0;
};


