#pragma once

#include "InterfaceLoadingSingle.h"

#include <cstdint>
#include <string>
#include <vector>

// Special "2D-interface" (not a descendant of CInterfaceScreenBase)
class CInterfaceMPLoading2D : public CInterfaceLoadingBase
{
	OBJECT_NOCOPY_METHODS( CInterfaceMPLoading2D );

public:
	struct SPlayer
	{
		std::wstring wszName;
		const NDb::STexture *pSideListItemIcon;
		const NDb::STexture *pSideMinimapIcon;
		int nLevel;
		std::wstring wszRank;
		int nPlayerIndex;
		int nTeam;
		uint32_t dwColour;
		CVec2 vMinimapPos;

		int operator&( IBinSaver &saver )
		{
			NI_ASSERT( 0, "Should not be serialized" );
			return 0;
		}

		SPlayer()
			: pSideListItemIcon( 0 )
			, pSideMinimapIcon( 0 )
			, nLevel( 0 )
			, nPlayerIndex( -1 )
			, nTeam( 0 )
			, dwColour( 0xffffffff )
			, vMinimapPos( VNULL2 )
		{
		}
	};

	struct SParams
	{
		CVec2 vMapAISize;
		const NDb::STexture *pMapPicture;
		const NDb::STexture *pMinimap;
		std::wstring wszMapName;
		std::wstring wszGameType;
		std::vector<SPlayer> players;
		bool bCooperative;

		int operator&( IBinSaver &saver )
		{
			NI_ASSERT( 0, "Should not be serialized" );
			return 0;
		}

		SParams()
			: vMapAISize( VNULL2 )
			, pMapPicture( 0 )
			, pMinimap( 0 )
			, bCooperative( false )
		{
		}
	};

protected:
	~CInterfaceMPLoading2D() override;

	void MakeInterior( const SParams &params );

public:
	CInterfaceMPLoading2D();
	CInterfaceMPLoading2D( const SParams &params );
};
