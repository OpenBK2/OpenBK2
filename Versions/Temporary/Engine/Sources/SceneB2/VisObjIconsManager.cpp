#include "StdAfx.h"

#include "../3DMotor/G2DView.h"
#include "../3DLib/Transform.h"
#include "Camera.h"
#include "VisObjIconsManager.h"

//#include <VTuneAPI.h>
//#pragma comment (lib, "vtuneapi.lib")

const int DEF_HPBAR_PARTS_NUM = 3;
const int DEF_ICONS_GAP_X = 2;
const int DEF_ICONS_ABOVE_Y = 2;
const int DEF_ICONS_BELOW_Y = 2;
const int DEF_HP_COLOR_NUM = 4 + 8; // user, friend, enemy, neutral + mp colors
const int DEF_HP_GAP_Y = 2;
const int DEF_ICON_GROUP_GAP_X = -20;
const int DEF_ICON_GROUP_GAP_Y = 0;
const int DEF_ICON_LEVELUP_GAP_Y = -20;
const int DEF_ICON_DAMAGED_BUILDING_GAP_Y = 2;
const int MAX_SMALL_HITBARS = 6;

void CVisObjIconsManager::Init( const NDb::SVisObjIconsSet *pIconsSet )
{
	pTexture = pIconsSet->pTexture;
	NI_ASSERT( pTexture, "Texture is not set for icons" );
	if ( !pTexture )
		return;

	// initialize HPBar
	nHPBarColorsNum = DEF_HP_COLOR_NUM; //max( pIconsSet->hpBarColors.size(), 1 );
	hpBarTexCoords.resize( DEF_HPBAR_PARTS_NUM + nHPBarColorsNum );
	CTRect<float> nullRect( VNULL2, VNULL2 );
	fill( hpBarTexCoords.begin(), hpBarTexCoords.end(), nullRect );
	NI_ASSERT( pIconsSet->hpBarBorders.size() >= DEF_HPBAR_PARTS_NUM, "HPBar contains too small number of parts" );
	if ( pIconsSet->hpBarBorders.size() >= DEF_HPBAR_PARTS_NUM )
	{
		for ( int i = 0; i < DEF_HPBAR_PARTS_NUM; ++i )
			hpBarTexCoords[i] = pIconsSet->hpBarBorders[i];
	}

	// CRAP - adapted for new color system
	NI_ASSERT( pIconsSet->hpBarColors.size() >= 2, "Colors is not set for HPBar" );
	if ( pIconsSet->hpBarColors.size() >= 2 )
	{
		CTRect<float> rcDelta;
		rcDelta.x1 = pIconsSet->hpBarColors[1].x1 - pIconsSet->hpBarColors[0].x1;
		rcDelta.y1 = pIconsSet->hpBarColors[1].y1 - pIconsSet->hpBarColors[0].y1;
		rcDelta.x2 = pIconsSet->hpBarColors[1].x2 - pIconsSet->hpBarColors[0].x2;
		rcDelta.y2 = pIconsSet->hpBarColors[1].y2 - pIconsSet->hpBarColors[0].y2;
		for ( int i = DEF_HPBAR_PARTS_NUM; i < hpBarTexCoords.size(); ++i )
		{
			hpBarTexCoords[i].x1 = pIconsSet->hpBarColors[0].x1 + rcDelta.x1 * (i - DEF_HPBAR_PARTS_NUM );
			hpBarTexCoords[i].y1 = pIconsSet->hpBarColors[0].y1 + rcDelta.y1 * (i - DEF_HPBAR_PARTS_NUM );
			hpBarTexCoords[i].x2 = pIconsSet->hpBarColors[0].x2 + rcDelta.x2 * (i - DEF_HPBAR_PARTS_NUM );
			hpBarTexCoords[i].y2 = pIconsSet->hpBarColors[0].y2 + rcDelta.y2 * (i - DEF_HPBAR_PARTS_NUM );
		}
	}
	
	hpBarRanges = pIconsSet->hpBarRanges;

	// initialize icons
	singleIcons.resize( pIconsSet->icons.size() );
	for ( int i = 0; i < pIconsSet->icons.size(); ++i )
	{
		singleIcons[i].nType = pIconsSet->icons[i].eType;
		singleIcons[i].nPriority = pIconsSet->icons[i].nPriority;
		singleIcons[i].texCoords = pIconsSet->icons[i].rctexCoords;
	}

	sort( singleIcons.begin(), singleIcons.end() );

	for ( int i = 0; i < singleIcons.size(); ++i )
		type2VectorHash[singleIcons[i].nType] = i;

	rectLayout.rects.reserve( 256 );

	fHPColOffset = max( ( ( hpBarTexCoords[0].x2 - hpBarTexCoords[0].x1 ) -
		( hpBarTexCoords[DEF_HPBAR_PARTS_NUM].x2 - hpBarTexCoords[DEF_HPBAR_PARTS_NUM].x1 ) ), 1.0f );
	fHPColExpand = max( ( hpBarTexCoords[0].x2 - hpBarTexCoords[0].x1 - fHPColOffset ) * 2, 0.0f );

	bNeedUpdate = true;
}

inline void CVisObjIconsManager::ProjectIcon( CVisObjIconsManager::SObjIcon &icon, const SHMatrix &matr, const CVec2 &vViewportSize )
{
	CVec4 vSrcPos;
	matr.RotateHVector( &vSrcPos, icon.vPos );

	//vSrcPos.x /= vSrcPos.w; // -1 to 1
	//vSrcPos.y /= vSrcPos.w; // -1 to 1
	//vSrcPos.z /= vSrcPos.w; // 0 to 1 if point is placed inside clipping planes

	const float fW = 1.0f / vSrcPos.w;
	icon.fDepth = vSrcPos.z * fW;
	const float fOffsetX = ( vSrcPos.x * fW + 1.0f ) * 0.5f;
	const float fOffsetY = ( vSrcPos.y * fW + 1.0f ) * 0.5f;

	if ( ( fOffsetX < 0.0f ) || ( fOffsetX > 1.0f ) || ( fOffsetY < 0.0f ) || ( fOffsetY > 1.0f ) || ( icon.fDepth < 0.0f ) || fW < 0.0f )
		icon.fDepth = -1.0f; // for not processing after sorting
	else
	{
		icon.nScrOffsetX = Float2Int( fOffsetX * vViewportSize.x ) - ( icon.nHPBarBaseLength >> 1 );
		icon.nScrOffsetY = Float2Int( ( 1.0f - fOffsetY ) * vViewportSize.y );
	}
}

inline void CVisObjIconsManager::RegenerateIconRects( CVisObjIconsManager::SObjIcon &icon )
{
	if ( icon.nColor > 0 )
	{
		CRectLayout::SRect rect;
		rect.sColor.dwColor = icon.nColor;

		const float fScrOffsetX = icon.nScrOffsetX;
		const float fScrOffsetY = icon.nScrOffsetY;
		const float fHPBarLength = (hpBarTexCoords[0].x2 - hpBarTexCoords[0].x1) + 
			icon.nHPBarBaseLength + (hpBarTexCoords[2].x2 - hpBarTexCoords[2].x1);
		const float fIconsAboveOffsetY = fScrOffsetY - DEF_ICONS_ABOVE_Y;
		const float fIconsBelowOffsetY = fScrOffsetY + (hpBarTexCoords[0].y2 - hpBarTexCoords[0].y1) + DEF_ICONS_BELOW_Y;

		// add common icons
		if ( !icon.icons.empty() )
		{
			float fIconsOffsetX = fScrOffsetX + fHPBarLength;

			float fIconsSizeX = 0.0f;
			for ( vector<int>::const_iterator it = icon.icons.begin(); it != icon.icons.end(); ++it )
			{
				const SSingleIcon &baseIcon = singleIcons[*it];
				fIconsSizeX += baseIcon.texCoords.x2 - baseIcon.texCoords.x1;
			}
			fIconsSizeX += DEF_ICONS_GAP_X * (icon.icons.size() - 1);
			if ( fIconsSizeX > fHPBarLength )
				fIconsOffsetX += (fIconsSizeX - fHPBarLength) * 0.5f;

			for ( vector<int>::const_iterator it = icon.icons.begin(); it != icon.icons.end(); ++it )
			{
				const SSingleIcon &baseIcon = singleIcons[*it];
				rect.fSizeX = baseIcon.texCoords.x2 - baseIcon.texCoords.x1;
				rect.fSizeY = baseIcon.texCoords.y2 - baseIcon.texCoords.y1;
				rect.fX = fIconsOffsetX - rect.fSizeX;
				rect.fY = fIconsBelowOffsetY;
				rect.sTex.rcTexRect = baseIcon.texCoords;
				rectLayout.rects.push_back( rect );
				fIconsOffsetX -= rect.fSizeX + DEF_ICONS_GAP_X;
			}
		}
		
		// group icon
		if ( icon.nIconGroup >= 0 )
		{
			const SSingleIcon &baseIcon = singleIcons[icon.nIconGroup];
			rect.fSizeX = baseIcon.texCoords.x2 - baseIcon.texCoords.x1;
			rect.fSizeY = baseIcon.texCoords.y2 - baseIcon.texCoords.y1;
			rect.fX = fScrOffsetX - rect.fSizeX - DEF_ICONS_GAP_X;
			rect.fY = fScrOffsetY;
			rect.sTex.rcTexRect = baseIcon.texCoords;
			rectLayout.rects.push_back( rect );
		}
		
		// levelup icon
		if ( icon.nIconLevelup >= 0 )
		{
			const SSingleIcon &baseIcon = singleIcons[icon.nIconLevelup];
			rect.fSizeX = baseIcon.texCoords.x2 - baseIcon.texCoords.x1;
			rect.fSizeY = baseIcon.texCoords.y2 - baseIcon.texCoords.y1;
			rect.fX = fScrOffsetX + fHPBarLength - rect.fSizeX;
			rect.fY = fIconsAboveOffsetY - rect.fSizeY;
			rect.sTex.rcTexRect = baseIcon.texCoords;
			rectLayout.rects.push_back( rect );
		}
		
		// damaged building icon
		if ( icon.nIconDamagedBuilding >= 0 )
		{
			const SSingleIcon &baseIcon = singleIcons[icon.nIconDamagedBuilding];
			rect.fSizeX = baseIcon.texCoords.x2 - baseIcon.texCoords.x1;
			rect.fSizeY = baseIcon.texCoords.y2 - baseIcon.texCoords.y1;
			rect.fX = fScrOffsetX + (fHPBarLength - rect.fSizeX) * 0.5f;
			rect.fY = fIconsAboveOffsetY - rect.fSizeY;
			rect.sTex.rcTexRect = baseIcon.texCoords;
			rectLayout.rects.push_back( rect );
		}

		// hp bar
		if ( icon.nHPBarBaseLength > 0 )
		{
			float fOffsetX = icon.nScrOffsetX;
			float fOffsetY = icon.nScrOffsetY;
			for ( int i = 0; i < icon.hpBars.size(); ++i )
			{
				const SObjIcon::SHPBar &hpBar = icon.hpBars[i];

				if ( hpBar.fLength > 0.0f )
				{
					// add HPBar border
					rect.fX = fOffsetX + hpBar.nOffsetX;
					rect.fY = fOffsetY + hpBar.nOffsetY;
					rect.fSizeX = hpBarTexCoords[0].x2 - hpBarTexCoords[0].x1;
					rect.fSizeY = hpBarTexCoords[0].y2 - hpBarTexCoords[0].y1;
					rect.sTex.rcTexRect = hpBarTexCoords[0];
					rectLayout.rects.push_back( rect );
					rect.fX += rect.fSizeX;
					rect.fSizeX = hpBar.fLength;
					rect.fSizeY = hpBarTexCoords[1].y2 - hpBarTexCoords[1].y1;
					rect.sTex.rcTexRect = hpBarTexCoords[1];
					rectLayout.rects.push_back( rect );
					rect.fX += rect.fSizeX;
					rect.fSizeX = hpBarTexCoords[2].x2 - hpBarTexCoords[2].x1;
					rect.fSizeY = hpBarTexCoords[2].y2 - hpBarTexCoords[2].y1;
					rect.sTex.rcTexRect = hpBarTexCoords[2];
					rectLayout.rects.push_back( rect );

					// add HPBar colors
					if ( hpBar.fValue > 0.0f )
					{
						rect.fX = fOffsetX + fHPColOffset;
						rect.fY = fOffsetY + fHPColOffset;
						const CTRect<float> &colRect = hpBarTexCoords[hpBar.nColorIndex];
						rect.fSizeX = ( hpBar.fLength + fHPColExpand ) * hpBar.fValue;
						rect.fSizeY = colRect.y2 - colRect.y1;
						rect.sTex.rcTexRect = colRect;
						rectLayout.rects.push_back( rect );
					}
					else
						rect.fSizeX = 0.0f;
					rect.fX = fOffsetX + fHPColOffset + rect.fSizeX;
					rect.fY = fOffsetY + fHPColOffset;
					const CTRect<float> &colRect = hpBarTexCoords[hpBar.nColorIndex2];
					rect.fSizeX = ( hpBar.fLength + fHPColExpand ) * hpBar.fValue2;
					rect.fSizeY = colRect.y2 - colRect.y1;
					rect.sTex.rcTexRect = colRect;
					rectLayout.rects.push_back( rect );

					fOffsetY -= hpBarTexCoords[0].y2 - hpBarTexCoords[0].y1 + DEF_HP_GAP_Y;
				}
			}
		}
	}
}

static vector<CVisObjIconsManager::SObjIcon> regenIcons( 256 );
void CVisObjIconsManager::RegenerateAllIconsRects()
{
	rectLayout.rects.resize( 0 );

	regenIcons.resize( 0 );
	for ( CObjIconsHash::const_iterator it = objIcons.begin(); it != objIcons.end(); ++it )
		regenIcons.push_back( it->second );

	if ( regenIcons.empty() )
		return;

	sort( regenIcons.begin(), regenIcons.end() ); // for depth ordering
	vector<SObjIcon>::iterator itIcon = regenIcons.end();
	do
	{
		--itIcon;
		if ( itIcon->fDepth < 0.0f )
			break;
		RegenerateIconRects( *itIcon );
	} while ( itIcon != regenIcons.begin() );
}

void CVisObjIconsManager::UpdateAllIcons()
{
	if ( !p2DView )
		return;

	const CVec2 vViewportSize = p2DView->GetViewportSize();
	ICamera *pCamera = Camera();
	const SHMatrix matr = pCamera->GetTransform().Get().forward;

	for ( CObjIconsHash::iterator it = objIcons.begin(); it != objIcons.end(); ++it )
		ProjectIcon( it->second, matr, vViewportSize );

	bNeedUpdate = true;
}

inline void CVisObjIconsManager::UpdateIcon( SObjIcon &icon )
{
	const bool bWasInside = icon.fDepth >= 0.0f;

	const CVec2 vViewportSize = p2DView->GetViewportSize();
	ICamera *pCamera = Camera();
	const SHMatrix matr = pCamera->GetTransform().Get().forward;
	ProjectIcon( icon, matr, vViewportSize );

	if ( ( icon.fDepth >= 0.0f ) || ( bWasInside ) )
		bNeedUpdate = true;
		//RegenerateAllIconsRects();
}

void CVisObjIconsManager::SetIcon( const SSceneObjIconInfo &iconInfo, const CVec3 &vPos )
{
	CObjIconsHash::iterator itFind = objIcons.find( iconInfo.nID );
	if ( itFind == objIcons.end() ) // create new
	{
		itFind = objIcons.insert( pair<int,SObjIcon>( iconInfo.nID, SObjIcon( iconInfo.nID, -1 ) ) ).first;
	}

	SObjIcon &icon = itFind->second;

	// common
	icon.vPos.Set( vPos.x, vPos.y, vPos.z + iconInfo.fAddHeight );
	icon.fAddHeight = iconInfo.fAddHeight;
	const int nCol = ClampFast( Float2Int( iconInfo.fAlpha * 255.0f ), 0, 255 );
	icon.nColor = ( nCol << 24 ) | ( nCol << 16 ) | ( nCol << 8 ) | ( nCol );
	icon.nHPBarBaseLength = iconInfo.nHPBarBaseLength;

	icon.hpBars.resize( 1 + Min( iconInfo.smallHitbars.size(), MAX_SMALL_HITBARS ) );
	SObjIcon::SHPBar &hpBar = icon.hpBars[0];
	hpBar.nOffsetX = 0;
	hpBar.nOffsetY = 0;
	hpBar.fLength = icon.nHPBarBaseLength;
	hpBar.nColorIndex = DEF_HPBAR_PARTS_NUM + Clamp( iconInfo.nHPBarAdditionalColorIndex, 0, nHPBarColorsNum - 1 );
	hpBar.nColorIndex2 = DEF_HPBAR_PARTS_NUM + Clamp( iconInfo.nHPBarColorIndex, 0, nHPBarColorsNum - 1 );
	hpBar.fValue = ClampFast( iconInfo.fHPBarValue * iconInfo.fHPBarAdditionalValue, 0.0f, 1.0f );
	hpBar.fValue2 = ClampFast( iconInfo.fHPBarValue - hpBar.fValue, 0.0f, 1.0f );

	icon.icons.reserve( iconInfo.icons.size() + 1 );
	icon.icons.resize( 0 );
	for ( int i = 0; i < iconInfo.icons.size(); ++i )
	{
		const NDb::SVisObjIconsSet::SVisObjIcon::EVisObjIconType eIconType = iconInfo.icons[i];
		if ( eIconType != NDb::SVisObjIconsSet::SVisObjIcon::VOIT_NONE )
		{ 
			const int nIconType = type2VectorHash[eIconType];
			icon.icons.push_back( nIconType );
		}
	}
	icon.nIconGroup = -1;
	icon.nIconLevelup = -1;
	icon.nIconDamagedBuilding = -1;
	if ( iconInfo.eIconGroup != NDb::SVisObjIconsSet::SVisObjIcon::VOIT_NONE )
		icon.nIconGroup = type2VectorHash[iconInfo.eIconGroup];
	if ( iconInfo.eIconLevelup != NDb::SVisObjIconsSet::SVisObjIcon::VOIT_NONE )
		icon.nIconLevelup = type2VectorHash[iconInfo.eIconLevelup];
	if ( iconInfo.eIconDamagedBuilding != NDb::SVisObjIconsSet::SVisObjIcon::VOIT_NONE )
		icon.nIconDamagedBuilding = type2VectorHash[iconInfo.eIconDamagedBuilding];
	sort( icon.icons.begin(), icon.icons.end() ); // for priority ordering
	for ( int i = 0; i < icon.hpBars.size() - 1; ++i )
	{
		const SSceneObjIconInfo::SHitbar &hitbarInfo = iconInfo.smallHitbars[i];
		SObjIcon::SHPBar &hpBar = icon.hpBars[i + 1];
		hpBar.nOffsetX = 0;
		hpBar.nOffsetY = 0;
		hpBar.fLength = iconInfo.nHPBarPassengerLength;
		hpBar.nColorIndex = DEF_HPBAR_PARTS_NUM + Clamp( hitbarInfo.nColorIndex, 0, nHPBarColorsNum - 1 );
		hpBar.nColorIndex2 = hpBar.nColorIndex;
		hpBar.fValue = 0.0f;
		hpBar.fValue2 = ClampFast( hitbarInfo.fValue, 0.0f, 1.0f );
	}

	UpdateIcon( icon );
}

void CVisObjIconsManager::UpdateIcon( const int nID, const CVec3 &vPos )
{
	CObjIconsHash::iterator itIcon = objIcons.find( nID );
	if ( itIcon != objIcons.end() )
	{
		SObjIcon &icon = itIcon->second;
		icon.vPos.Set( vPos.x, vPos.y, vPos.z + icon.fAddHeight );
		UpdateIcon( icon );
	}
}

void CVisObjIconsManager::RemoveIcon( const int nID )
{
	CObjIconsHash::iterator itIcon = objIcons.find( nID );
	if ( itIcon != objIcons.end() )
	{
		objIcons.erase( itIcon );
		bNeedUpdate = true;
	}
}

void CVisObjIconsManager::DrawIcons()
{
	//VTResume();
	if ( p2DView )
	{
		if ( bNeedUpdate )
		{
			RegenerateAllIconsRects();
			bNeedUpdate = false;
		}

		if ( rectLayout.rects.empty() )
			return;

		CTRect<float> scrRect( CVec2( 0, 0 ), p2DView->GetViewportSize() );
		//p2DView->CreateDynamicClearRects( rectLayout, CTPoint<float>( 0, 0 ), scrRect );
		p2DView->CreateDynamicRects( pTexture, rectLayout, CTPoint<float>( 0, 0 ), scrRect );
	}
	//VTPause();
}

REGISTER_SAVELOAD_CLASS( 0x17146CC0, CVisObjIconsManager );


