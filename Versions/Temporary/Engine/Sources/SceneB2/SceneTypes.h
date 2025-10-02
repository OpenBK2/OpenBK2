#pragma once

// unit's icons and hitbars
struct SSceneObjIconInfo
{
	struct SHitbar
	{
		float fValue;
		int nColorIndex;
	};
	
	int nID;
	int nHPBarBaseLength;
	int nHPBarPassengerLength;
	float fAddHeight;
	float fAlpha;

	bool bIsMainHitbar;
	float fHPBarValue;
	int nHPBarColorIndex;
	float fHPBarAdditionalValue; // 2-color main hitbar
	int nHPBarAdditionalColorIndex;
	
	vector<NDb::SVisObjIconsSet::SVisObjIcon::EVisObjIconType> icons;
	vector<SHitbar> smallHitbars;
	NDb::SVisObjIconsSet::SVisObjIcon::EVisObjIconType eIconGroup;
	NDb::SVisObjIconsSet::SVisObjIcon::EVisObjIconType eIconLevelup;
	NDb::SVisObjIconsSet::SVisObjIcon::EVisObjIconType eIconDamagedBuilding;
	
	bool IsEmpty() const
	{
		return (nHPBarBaseLength == 0) || !bIsMainHitbar;
	}
	
	SSceneObjIconInfo( int _nID ) :
		nID( _nID ), 
		nHPBarBaseLength( 0 ),
		eIconGroup( NDb::SVisObjIconsSet::SVisObjIcon::VOIT_NONE ),
		eIconLevelup( NDb::SVisObjIconsSet::SVisObjIcon::VOIT_NONE ),
		eIconDamagedBuilding( NDb::SVisObjIconsSet::SVisObjIcon::VOIT_NONE )
	{
	}
};

