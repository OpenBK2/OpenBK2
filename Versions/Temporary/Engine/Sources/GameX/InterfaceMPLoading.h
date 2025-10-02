#pragma once

#include "InterfaceLoadingSingle.h"

// Special "2D-interface" (not a descendant of CInterfaceScreenBase)
class CInterfaceMPLoading2D : public CInterfaceLoadingBase
{
	OBJECT_NOCOPY_METHODS( CInterfaceMPLoading2D );
public:

	struct SPlayer
	{
		wstring wszName;
		const NDb::STexture *pSideListItemIcon;
		const NDb::STexture *pSideMinimapIcon;
		int nLevel;
		wstring wszRank;
		int nPlayerIndex;
		int nTeam;
		CVec2 vMinimapPos;
		
		int operator&( IBinSaver &saver ) { NI_ASSERT( 0, "Should not be serialized" ); return 0; }
		
		SPlayer() :
			pSideListItemIcon( 0 ),
			pSideMinimapIcon( 0 ),
			nLevel( 0 ),
			nTeam( 0 ),
			vMinimapPos( VNULL2 )
		{
		}
	};

	struct SParams
	{
		CVec2 vMapAISize;
		const NDb::STexture *pMapPicture;
		const NDb::STexture *pMinimap;
		wstring wszMapName;
		wstring wszGameType;
		vector<SPlayer> players;

		int operator&( IBinSaver &saver ) { NI_ASSERT( 0, "Should not be serialized" ); return 0; }
		
		SParams() :
			pMapPicture( 0 ),
			pMinimap( 0 )
		{
		}
	};

protected:
	~CInterfaceMPLoading2D();

	void MakeInterior( const SParams &params );
public:
	CInterfaceMPLoading2D();
	CInterfaceMPLoading2D( const SParams &params );
};


