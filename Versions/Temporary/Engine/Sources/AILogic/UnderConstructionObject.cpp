#include "StdAfx.h"
#include ".\underconstructionobject.h"
#include "NewUpdater.h"
#include "EntrenchmentCreation.h"
#include "ComplexObstacleCreation.h"
#include "StaticObject.h"
#include "Diplomacy.h"
#include "..\Stats_B2_m1\ActionCommand.h"

extern CEventUpdater updater;
extern CDiplomacy theDipl;
CUnderConstructionObject theUnderConstructionObject;

void CUnderConstructionObject::SendClearUpdate()
{
	CPtr<SAIObjectsUnderConstructionUpdate> pNewUpdate = new SAIObjectsUnderConstructionUpdate( false );
	updater.AddUpdate( pNewUpdate, ACTION_NOTIFY_OBJECTS_UNDER_CONSTRUCTION, 0, 0 );
}

void CUnderConstructionObject::Clear() 
{ 
	//objects.clear();
}
#ifndef _FINALRELEASE
#include "..\System\CheckSumLog.h"

class CSimpleChecksumLog : public ICheckSumLog
{ 
	OBJECT_BASIC_METHODS( CSimpleChecksumLog );
	hash_map<int, unsigned long> entries1;
	bool bEntries2;
public:
	CSimpleChecksumLog() : bEntries2( false ) {  }
	void SetEntries2()
	{
		bEntries2 = true;
	}
	virtual bool AddChecksumLog( const int nGameTime, const unsigned long ulChecksum, const int nEntry )
	{
		if ( bEntries2 )
		{
			const unsigned long entry = entries1[nEntry];
			if ( ulChecksum == entry )
			{

			}
			else
			{
				NI_ASSERT( ulChecksum == entry, "differ" );
				return false;
			}
		}
		else
			entries1[nEntry] = ulChecksum;
		return true;
	}
};
bool bShowUnderConstruction = false;
#endif


void CUnderConstructionObject::ShowUnderConstruction( EActionCommand eCommand, const CVec2 &vStart, const CVec2 &vFinish, bool bFinished, CAILogic *pAI )
{
	if ( bFinished ) 
	{
		SendClearUpdate();
		return;
	}
	
	CExistingObjectModifyAI prohibited;
	switch ( eCommand )
	{
	case ACTION_COMMAND_BUILD_FENCE_BEGIN:
		// create CFenceCreation, PreCreate, create update and add it to updater
		{
			CPtr<CComplexObstacleCreation> pFence = new CComplexObstacleCreation( theDipl.GetMyNumber(), false );

			pFence->PreCreate( vStart, vFinish, false );
			CPtr<SAIObjectsUnderConstructionUpdate> pUpdate = new SAIObjectsUnderConstructionUpdate( true );
			pFence->CreateObjects( pUpdate );
			updater.AddUpdate( pUpdate, ACTION_NOTIFY_OBJECTS_UNDER_CONSTRUCTION, 0, 0 );
		}

		break;
	case ACTION_COMMAND_ENTRENCH_BEGIN:
		// create CFenceCreation, PreCreate, create update and add it to updater
		{
			CPtr<CEntrenchmentCreation> pFence = new CEntrenchmentCreation( theDipl.GetMyNumber(), false );

			pFence->PreCreate( vStart, vFinish, false );
			CPtr<SAIObjectsUnderConstructionUpdate> pUpdate = new SAIObjectsUnderConstructionUpdate( true );
			pFence->CreateObjects( pUpdate );
			updater.AddUpdate( pUpdate, ACTION_NOTIFY_OBJECTS_UNDER_CONSTRUCTION, 0, 0 );

		}

		break;
	case ACTION_COMMAND_LAND_MINE:

		break;
	default:
		NI_ASSERT( false, StrFmt( "wrong build command %i", eCommand ) );
	}
}

int CUnderConstructionObject::operator&( IBinSaver &saver )
{
	return 0;
}

