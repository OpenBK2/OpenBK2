#if !defined(__STORE_INPUT_STATE_TEMPLATE__)
#define __STORE_INPUT_STATE_TEMPLATE__
#pragma once

#include "../MapEditorLib/StoreInputState.h"

class CMapInfoStoreInputState : public CStoreInputState
{
	struct SEventInfo : public SInputStateEventInfo
	{
		bool isValid;
		CVec3 vTerrainPos;
		CTPoint<int> visTilePos;
		CTPoint<int> aiTilePos;
		CTPoint<int> gridPos;
		//
		SEventInfo() : isValid( false ), vTerrainPos( VNULL3 ), visTilePos( 0, 0 ), aiTilePos( 0, 0 ), gridPos( 0, 0 ) {}
		SEventInfo( const SEventInfo &rTileEventInfo )	: SInputStateEventInfo( rTileEventInfo ), isValid( rTileEventInfo.isValid ), vTerrainPos( rTileEventInfo.vTerrainPos ), visTilePos( rTileEventInfo.visTilePos ), aiTilePos( rTileEventInfo.aiTilePos ), gridPos( rTileEventInfo.gridPos ) {}
		SEventInfo( const SInputStateEventInfo &rInputStateEventInfo )	: SInputStateEventInfo( rInputStateEventInfo ), isValid( false ), vTerrainPos( VNULL3 ), visTilePos( 0, 0 ), aiTilePos( 0, 0 ), gridPos( 0, 0 ) {}
		SEventInfo& operator=( const SEventInfo &rTileEventInfo )
		{
			if( &rTileEventInfo != this )
			{
				SInputStateEventInfo::operator=( rTileEventInfo );
				isValid = rTileEventInfo.isValid;
				vTerrainPos = rTileEventInfo.vTerrainPos;
				visTilePos = rTileEventInfo.visTilePos;
				aiTilePos = rTileEventInfo.aiTilePos;
				gridPos = rTileEventInfo.gridPos;
			}
			return *this;
		}
		inline bool IsValid() { return isValid; }
	};
	//

protected:
	//CStoreInputState
	void OnInputStateEvent( const SInputStateEventInfo &rInputStateEventInfo );

public:
	bool bFixInvalidPos;
	SEventInfo eventInfoList[ISE_COUNT];
	SEventInfo lastEventInfo;
	//
	CTPoint<int> visSize;

	CMapInfoStoreInputState() : bFixInvalidPos( false ), visSize( 0, 0 ) {}
	inline void SetSizes( const CTPoint<int> &rVisSize, bool _bFixInvalidPos ) { visSize = rVisSize; bFixInvalidPos = _bFixInvalidPos; }
};

#endif // !defined(__STORE_INPUT_STATE_TEMPLATE__)

