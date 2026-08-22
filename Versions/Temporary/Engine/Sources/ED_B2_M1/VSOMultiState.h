#pragma once

#include "CommandHandlerDefines.h"

#include "RoadState.h"
#include "RiverState.h"
#include "CragState.h"
#include "LakeState.h"
#include "CoastState.h"

#include <cstdint>

//Mapinfo object state edit parameters
#define MIVSOSEP_POINT_NUMBER	0x00000001
#define MIVSOSEP_STATS_TYPE		0x00000002
#define MIVSOSEP_WIDTH				0x00000004
#define MIVSOSEP_OPACITY			0x00000008
#define MIVSOSEP_HEIGHT				0x00000010
#define MIVSOSEP_THUMBNAILS		0x00000020
#define MIVSOSEP_ALL					0xFFFFFFFF


class CVSOMultiState : public CMultiInputState, public ICommandHandler
{
	friend class CMultiInputState;
	friend class CVSOState;

	enum EInputStates
	{
		IS_ROAD		= 0,
		IS_RIVER	= 1,
		IS_CRAG		= 2,
		IS_LAKE		= 3,
		IS_COAST	= 4,
		IS_COUNT	= 5,
	};

public:
	struct SEditParameters
	{
		enum EPointNumber
		{
			PN_SINGLE		= 0,
			PN_MULTI		= 1,
			PN_ALL			= 2,
		};
		
		enum EStatsType
		{
			STATS_DEFAULT	= 0,
			STATS_CUSTOM	= 1,
		};
		//
		unsigned nFlags;
		EPointNumber ePointNumber;
		EStatsType eStatsType;
		float fWidth;
		float fOpacity;
		float fHeight;
		bool bThumbnails;
		//
		SEditParameters()
			: nFlags( 0 ),
				ePointNumber( PN_SINGLE ),
				eStatsType( STATS_CUSTOM ),
				fWidth( CVSOManager::DEFAULT_WIDTH ),
				fOpacity( CVSOManager::DEFAULT_OPACITY ),
				fHeight( CVSOManager::DEFAULT_HEIGHT ),
				bThumbnails( true ) {}
		SEditParameters( const SEditParameters &rEditParameters )
			: nFlags( rEditParameters.nFlags ),
				ePointNumber( rEditParameters.ePointNumber ),
				eStatsType( rEditParameters.eStatsType ),
				fWidth( rEditParameters.fWidth ),
				fOpacity( rEditParameters.fOpacity ),
				fHeight( rEditParameters.fHeight ),
				bThumbnails( rEditParameters.bThumbnails ) {}
		SEditParameters& operator=( const SEditParameters &rEditParameters )
		{
			if( &rEditParameters != this )
			{
				nFlags = rEditParameters.nFlags;
				ePointNumber = rEditParameters.ePointNumber;
				eStatsType = rEditParameters.eStatsType;
				fWidth = rEditParameters.fWidth;
				fOpacity = rEditParameters.fOpacity;
				fHeight = rEditParameters.fHeight;
				bThumbnails = rEditParameters.bThumbnails;
			}
			return *this;
		}
		int operator&( IXmlSaver &xs );
	};

	SEditParameters* GetEditParameters();
	class CMapInfoEditor* pMapInfoEditor;

	void UpdateEditParameters( unsigned nFlags );

	//конструкторы и операторы присваивания
	CVSOMultiState( CMapInfoEditor* _pMapInfoEditor = 0 )
	{
		pMapInfoEditor = _pMapInfoEditor;
		int nStateIndex = INVALID_INPUT_STATE_INDEX;
		
		CRoadState *pRoadState = new CRoadState( this );
		nStateIndex = AddInputState( pRoadState );
		NI_ASSERT( nStateIndex == IS_ROAD, StrFmt( "CVSOMultiState(): Wrong state number: %d (%d)", nStateIndex, IS_ROAD ) );

		CRiverState *pRiverState = new CRiverState( this );
		nStateIndex = AddInputState( pRiverState );
		NI_ASSERT( nStateIndex == IS_RIVER, StrFmt( "CVSOMultiState(): Wrong state number: %d, (%d)", nStateIndex, IS_RIVER ) );

		CCragState *pCragState = new CCragState( this );
		nStateIndex = AddInputState( pCragState );
		NI_ASSERT( nStateIndex == IS_CRAG, StrFmt( "CVSOMultiState(): Wrong state number: %d, (%d)", nStateIndex, IS_CRAG ) );

		CLakeState *pLakeState = new CLakeState( this );
		nStateIndex = AddInputState( pLakeState );
		NI_ASSERT( nStateIndex == IS_LAKE, StrFmt( "CVSOMultiState(): Wrong state number: %d, (%d)", nStateIndex, IS_LAKE ) );
		
		CCoastState *pCoastState = new CCoastState( this );
		nStateIndex = AddInputState( pCoastState );
		NI_ASSERT( nStateIndex == IS_COAST, StrFmt( "CVSOMultiState(): Wrong state number: %d, (%d)", nStateIndex, IS_COAST ) );

		SetActiveInputState( IS_ROAD, false, false );
		Singleton<ICommandHandlerContainer>()->Set( CHID_MAPINFO_VSO_MULTI_STATE, this );
	}
	//
	~CVSOMultiState()
	{
		Singleton<ICommandHandlerContainer>()->Remove( CHID_MAPINFO_VSO_MULTI_STATE );
	}

	void SwitchState( const string &rszObjectTypeName );
	//IInputState interface
	virtual void Enter();
	virtual void Leave();

public:
	bool PickOtherVSO( unsigned nFlags, const CTPoint<int> &rMousePoint, const CVec3 &rvPos );

	//ICommandHandler
	bool HandleCommand( unsigned nCommandID, uint32_t dwData );
	bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck );
};



