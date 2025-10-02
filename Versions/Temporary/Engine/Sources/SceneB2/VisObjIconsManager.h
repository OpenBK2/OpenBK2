#pragma once

#include "../Stats_B2_M1/IconsSet.h"
#include "../3DMotor/RectLayout.h"
#include "SceneTypes.h"

namespace NGScene
{
	class I2DGameView;
}

class CVisObjIconsManager : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( CVisObjIconsManager )
public:
	struct SObjIcon
	{
		struct SHPBar
		{
			int nOffsetX;
			int nOffsetY;
			int fLength;
			int nColorIndex;
			float fValue;
			int nColorIndex2;
			float fValue2;
		};

		int nID;
		int nHPBarBaseLength;
		CVec3 vPos;
		int nScrOffsetX;
		int nScrOffsetY;
		float fDepth;
		DWORD nColor;
		float fAddHeight;
		vector<int> icons;
		vector<SHPBar> hpBars;
		int nIconGroup;
		int nIconLevelup;
		int nIconDamagedBuilding;
		//
		SObjIcon() {}
		SObjIcon( const int _nID, const float _fDepth ) : nID(_nID), fDepth(_fDepth) {}
		//
		bool operator < ( const SObjIcon &v ) const { return fDepth < v.fDepth; }
		bool IsEmpty() const { return icons.empty() && (nHPBarBaseLength <= 0); }
	};
	//
private:
	//
	struct SSingleIcon
	{
		int nType;
		int nPriority;
		CTRect<float> texCoords;
		//
		bool operator < ( const SSingleIcon &v ) const { return nPriority < v.nPriority; }
	};
	//
	hash_map<int, int> type2VectorHash; // hash from EIconTypeEnum to singleIcons index
	vector<CTRect<float> > hpBarTexCoords;
	vector<float> hpBarRanges;
	vector<SSingleIcon> singleIcons;
	//list<SObjIcon> objIcons; // icons data
	typedef hash_map<int, SObjIcon> CObjIconsHash;
	CObjIconsHash objIcons;
	CDBPtr<NDb::STexture> pTexture;
	CRectLayout rectLayout;
	int nHPBarColorsNum;
	float fHPColOffset;
	float fHPColExpand;
	CPtr<NGScene::I2DGameView> p2DView;
	bool bNeedUpdate;
	//
protected:
	void ProjectIcon( CVisObjIconsManager::SObjIcon &icon, const SHMatrix &matr, const CVec2 &vViewportSize );
	void RegenerateIconRects( CVisObjIconsManager::SObjIcon &icon );
	void RegenerateAllIconsRects();
	void UpdateIcon( SObjIcon &icon );
	//
public:
	void Init( const NDb::SVisObjIconsSet *pIconsSet );
	void SetIcon( const SSceneObjIconInfo &iconInfo, const CVec3 &vPos );
	void UpdateIcon( const int nID, const CVec3 &vPos );
	void RemoveIcon( const int nID );
	void UpdateAllIcons();
	void DrawIcons();
	void Attach2DView( NGScene::I2DGameView *_p2DView ) { p2DView = _p2DView; }
};


