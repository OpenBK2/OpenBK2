#pragma once

#include "3Dmotor/RectLayout.h"
#include "SceneTypes.h"
#include "Stats_B2_M1/IconsSet.h"

#include <cstdint>

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
			// Defensive member defaults; explicit constructor values still take precedence.
			int nOffsetX = 0;
			int nOffsetY = 0;
			int fLength = 0;
			int nColorIndex = 0;
			float fValue = 0.0f;
			int nColorIndex2 = 0;
			float fValue2 = 0.0f;
		};

		// Defensive member defaults; explicit constructor values still take precedence.
		int nID = 0;
		int nHPBarBaseLength = 0;
		CVec3 vPos;
		int nScrOffsetX = 0;
		int nScrOffsetY = 0;
		float fDepth = 0.0f;
		uint32_t nColor = 0;
		float fAddHeight = 0.0f;
		std::vector<int> icons;
		std::vector<SHPBar> hpBars;
		int nIconGroup = 0;
		int nIconLevelup = 0;
		int nIconDamagedBuilding = 0;

		//
		SObjIcon() {}

		SObjIcon( const int _nID, const float _fDepth )
			: nID( _nID )
			, fDepth( _fDepth )
		{
		}

		//
		bool operator<( const SObjIcon &v ) const
		{
			return fDepth < v.fDepth;
		}

		bool IsEmpty() const
		{
			return icons.empty() && ( nHPBarBaseLength <= 0 );
		}
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
		bool operator<( const SSingleIcon &v ) const
		{
			return nPriority < v.nPriority;
		}
	};

	//
	std::unordered_map<int, int> type2VectorHash; // hash from EIconTypeEnum to singleIcons index
	std::vector<CTRect<float>> hpBarTexCoords;
	std::vector<float> hpBarRanges;
	std::vector<SSingleIcon> singleIcons;
	//list<SObjIcon> objIcons; // icons data
	typedef std::unordered_map<int, SObjIcon> CObjIconsHash;
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

	void Attach2DView( NGScene::I2DGameView *_p2DView )
	{
		p2DView = _p2DView;
	}
};
