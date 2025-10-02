#pragma once

#include "CommandHandlerDefines.h"

#include "SpotState.h"
#include "BridgeState.h"
#include "FenceState.h"
#include "EntrenchmentState.h"

//Mapinfo object state edit parameters
#define MIMOSEP_PLAYER_COUNT			0x00000001
#define MIMOSEP_PLAYER_INDEX			0x00000002
#define MIMOSEP_DIRECTION_TYPE		0x00000004
#define MIMOSEP_DIRECTION					0x00000008
#define MIMOSEP_THUMBNAILS				0x00000010
#define MIMOSEP_ALL								0xFFFFFFFF


class CMapObjectMultiState : public CMultiInputState, public ICommandHandler
{
	friend class CMultiInputState;
	friend class CMapInfoState;

	enum EInputStates
	{
		IS_SIMPLE_OBJECT	= 0,
		IS_BRIDGE					= 1,
		IS_SPOT						= 2,
		IS_FENCE					= 3,
		IS_ENTRENCHMENT		= 4,
		IS_COUNT					= 5,
	};

public:
	// Структура данных, с помощью которой конфигурационный диалог общается со стейтом
	struct SEditParameters
	{
		enum EDirectionType
		{
			DT_RANDOM	= 0,
			DT_CUSTOM	= 1,
		};

		typedef vector<string> CPlayerList;

		UINT nFlags;
		//
		EDirectionType eDirectionType;
		CPlayerList playerList;
		int nPlayerIndex;
		float fDirection;
		bool bThumbnails;
		SEditParameters()
			:	nFlags( MIMOSEP_ALL ),
				nPlayerIndex( 0 ),
				eDirectionType( DT_RANDOM ),
				fDirection( 0.0f  ),
				bThumbnails( true ) {}
		SEditParameters( const SEditParameters &rEditParameters )
			:	nFlags( rEditParameters.nFlags ),
				playerList( rEditParameters.playerList ),
				nPlayerIndex( rEditParameters.nPlayerIndex ),
				eDirectionType( rEditParameters.eDirectionType ),
				fDirection( rEditParameters.fDirection ),
				bThumbnails( rEditParameters.bThumbnails ) {}
		SEditParameters& operator=( const SEditParameters &rEditParameters )
		{
			if( &rEditParameters != this )
			{
				nFlags = rEditParameters.nFlags;
				playerList = rEditParameters.playerList;
				nPlayerIndex = rEditParameters.nPlayerIndex;
				eDirectionType = rEditParameters.eDirectionType;
				fDirection = rEditParameters.fDirection;
				bThumbnails = rEditParameters.bThumbnails;
			}
			return *this;
		}

		int operator&( IXmlSaver &xs );
	};

	SEditParameters* GetEditParameters();
	class CMapInfoEditor* pMapInfoEditor;

	void UpdateEditParameters( UINT nFlags );

	//конструкторы и операторы присваивания
	CMapObjectMultiState( CMapInfoEditor* _pMapInfoEditor = 0 )
	{
		pMapInfoEditor = _pMapInfoEditor;
		int nStateIndex = INVALID_INPUT_STATE_INDEX;
		
		CSimpleObjectState *pSimpleObjectState = new CSimpleObjectState( this );
		nStateIndex = AddInputState( pSimpleObjectState );
		NI_ASSERT( nStateIndex == IS_SIMPLE_OBJECT, StrFmt( "CMapObjectMultiState(): Wrong state number: %d (%d)", nStateIndex, IS_SIMPLE_OBJECT ) );

		CBridgeState *pBridgeState = new CBridgeState( this );
		nStateIndex = AddInputState( pBridgeState );
		NI_ASSERT( nStateIndex == IS_BRIDGE, StrFmt( "CMapObjectMultiState(): Wrong state number: %d, (%d)", nStateIndex, IS_BRIDGE ) );

		CSpotState *pSpotState = new CSpotState( this );
		nStateIndex = AddInputState( pSpotState );
		NI_ASSERT( nStateIndex == IS_SPOT, StrFmt( "CMapObjectMultiState(): Wrong state number: %d, (%d)", nStateIndex, IS_SPOT ) );

		CFenceState *pFenceState = new CFenceState( this );
		nStateIndex = AddInputState( pFenceState );
		NI_ASSERT( nStateIndex == IS_FENCE, StrFmt( "CMapObjectMultiState(): Wrong state number: %d, (%d)", nStateIndex, IS_FENCE ) );

		CEntrenchmentState *pEntrenchmentState = new CEntrenchmentState( this );
		nStateIndex = AddInputState( pEntrenchmentState );
		NI_ASSERT( nStateIndex == IS_ENTRENCHMENT, StrFmt( "CMapObjectMultiState(): Wrong state number: %d, (%d)", nStateIndex, IS_ENTRENCHMENT ) );
		
		SetActiveInputState( IS_SIMPLE_OBJECT, false, false );
		Singleton<ICommandHandlerContainer>()->Set( CHID_MAPINFO_MAPOBJECT_MULTI_STATE, this );
	}
	//
	~CMapObjectMultiState()
	{
		Singleton<ICommandHandlerContainer>()->Remove( CHID_MAPINFO_MAPOBJECT_MULTI_STATE );
	}

	void SwitchState( const string &rszObjectTypeName );
	//IInputState interface
	virtual void Enter();
	virtual void Leave();

public:
	//ICommandHandler
	bool HandleCommand( UINT nCommandID, DWORD dwData );
	bool UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck );
};



