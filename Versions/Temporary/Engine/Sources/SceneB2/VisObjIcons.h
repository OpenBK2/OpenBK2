#pragma once

#include "3Dmotor/GView.h"
#include "Stats_B2_M1/IconsSet.h"
#include "B2_M1_Terrain/PatchHolder.h"

struct ICamera;
class CCSBound;

class CVisObjIconInfo : public CPtrFuncBase<NGScene::CObjectInfo>
{
	OBJECT_BASIC_METHODS( CVisObjIconInfo );

	CPtr<ICamera> pCamera;
	bool bUpdate;
protected:
	void Recalc();
	bool NeedUpdate();
	void OrientToViewer();
	//
	CVisObjIconInfo() {}
public:
	NMeshData::SMeshData data;
	CVec3 vCenter;
	vector<CVec2> iconsSizesMin;
	vector<CVec2> iconsSizesMax;
	//
	CVisObjIconInfo( ICamera *_pCamera ) : pCamera( _pCamera ), bUpdate( true ) {}
	//
	void ForceUpdate() { bUpdate = true; }
	//
	int operator&( IBinSaver &saver )
	{
		saver.Add( 1, &pCamera );
		saver.Add( 2, &bUpdate );
		saver.Add( 3, &data );
		saver.Add( 4, &vCenter );
		saver.Add( 5, &iconsSizesMin );
		saver.Add( 6, &iconsSizesMax );
		return 0;
	}
};

typedef NMeshData::SPatchHolder<CVisObjIconInfo> CVisObjIconHolder;

struct SVisObjIcons
{
	struct SPresentIcon
	{
		NDb::SIconsSet::SIconType::EIconTypeEnum eType;
		float fValue;
		BYTE cAlpha;
		//
		SPresentIcon() {}
		SPresentIcon( const NDb::SIconsSet::SIconType::EIconTypeEnum _eType, const float _fValue, const BYTE _cAlpha ) :
			eType( _eType ), fValue( _fValue ), cAlpha( _cAlpha ) {}
		//
		int operator &( IBinSaver &saver )
		{
			saver.Add( 1, &eType );
			saver.Add( 2, &fValue );
			saver.Add( 3, &cAlpha );
			return 0;
		}
	};

	CObj<CCSBound> pBound;
	CVisObjIconHolder iconHolder;
	vector<SPresentIcon> icons;
	float fIconHalfWidth;
	float fIconAddHeight;
	//
	void CreateIcons( NGScene::IGameView *pGameView, const NDb::SIconsSet *pIconSet, const CVec3 &vPos );
	void MoveIcons( const CVec3 &vPos, const float fObjHeight );
	//
	int operator&( IBinSaver &saver )
	{
		saver.Add( 1, &pBound );
		//saver.Add( 2, &iconHolder );
		saver.Add( 3, &icons );
		saver.Add( 4, &fIconHalfWidth );
		saver.Add( 5, &fIconAddHeight );
		return 0;
	}
};

inline bool operator == ( const SVisObjIcons::SPresentIcon &a, const SVisObjIcons::SPresentIcon &b )
{
	return ( a.eType == b.eType );
}


