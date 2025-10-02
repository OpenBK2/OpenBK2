#include "StdAfx.h"

#include "MapInfoEditorData_Selection.h"

namespace NMapInfoEditor
{
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SObjectSelectionPart::BackupPosition()
	{
		vBackupPosition = vPosition;
		fBackupDirection = fDirection;
		bBackupCreated = true;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SObjectSelectionPart::RollbackPosition()
	{
		if ( bBackupCreated )
		{
			vPosition = vBackupPosition;
			fDirection = fBackupDirection;
		}
		return bBackupCreated;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SObjectSelection::ResetPickSelection()
	{
		eSelectionType = ST_UNKNOWN;
		vPositionDifference = VNULL3;
		fDirectionDifference = 0.0f;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SObjectSelection::Clear()
	{
		objectSelectionPartMap.clear();
		vPosition = VNULL3;
		fDirection = 0.0f;
		vBackupPosition = VNULL3;
		fBackupDirection = 0.0f;
		bBackupCreated = false;
		eSelectionType = ST_UNKNOWN;
		vPositionDifference = VNULL3;
		fDirectionDifference = 0.0f;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SObjectSelection::BackupPosition()
	{
		vBackupPosition = vPosition;
		fBackupDirection = fDirection;
		bBackupCreated = true;
		//
		for ( CObjectSelectionPartMap::iterator itObjectSelectionPart = objectSelectionPartMap.begin(); itObjectSelectionPart != objectSelectionPartMap.end(); ++itObjectSelectionPart )
		{
			itObjectSelectionPart->second.BackupPosition();
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SObjectSelection::RollbackPosition()
	{
		if ( bBackupCreated )
		{
			vPosition = vBackupPosition;
			fDirection = fBackupDirection;

			for ( CObjectSelectionPartMap::iterator itObjectSelectionPart = objectSelectionPartMap.begin(); itObjectSelectionPart != objectSelectionPartMap.end(); ++itObjectSelectionPart )
			{
				itObjectSelectionPart->second.RollbackPosition();
			}
		}
		return bBackupCreated;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SObjectSelection::MakeAbsolute()
	{
		for ( CObjectSelectionPartMap::iterator itObjectSelectionPart = objectSelectionPartMap.begin(); itObjectSelectionPart != objectSelectionPartMap.end(); ++itObjectSelectionPart )
		{
			itObjectSelectionPart->second.vPosition += vPosition;
			itObjectSelectionPart->second.fDirection += fDirection;
		}
		vPosition = VNULL3;
		fDirection = 0.0f;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SObjectSelection::MakeRelative()
	{
		vPosition = VNULL3;
		fDirection = 0.0f;
		int nSelectionCount = 0;
		for ( CObjectSelectionPartMap::const_iterator itObjectSelectionPart = objectSelectionPartMap.begin(); itObjectSelectionPart != objectSelectionPartMap.end(); ++itObjectSelectionPart )
		{
			vPosition += itObjectSelectionPart->second.vPosition;
			fDirection += itObjectSelectionPart->second.fDirection;
			++nSelectionCount;
		}
		if ( nSelectionCount > 0 )
		{
			vPosition /= ( nSelectionCount * 1.0f );
			fDirection /= ( nSelectionCount * 1.0f );
			// установим относительные координаты
			for ( CObjectSelectionPartMap::iterator itObjectSelectionPart = objectSelectionPartMap.begin(); itObjectSelectionPart != objectSelectionPartMap.end(); ++itObjectSelectionPart )
			{
				itObjectSelectionPart->second.vPosition -= vPosition;
				itObjectSelectionPart->second.fDirection -= fDirection;
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SObjectSelection::Trace() const
	{
		DebugTrace( "selection, begin" );
		for ( CObjectSelectionPartMap::const_iterator itObjectSelectionPart = objectSelectionPartMap.begin(); itObjectSelectionPart != objectSelectionPartMap.end(); ++itObjectSelectionPart )
		{
			DebugTrace( "ID: %d, pos: ( %g, %g, %g ), dir: %g",
									itObjectSelectionPart->first,
									itObjectSelectionPart->second.vPosition.x,
									itObjectSelectionPart->second.vPosition.y,
									itObjectSelectionPart->second.vPosition.z,
									itObjectSelectionPart->second.fDirection );
		}
		DebugTrace( "selection, end" );
		//
		DebugTrace( "selection, pos: ( %g, %g, %g ), dir: %g",
								vPosition.x, vPosition.y, vPosition.z, fDirection );
		//
		DebugTrace( "selection, %s, backupPos: ( %g, %g, %g ), backupDir: %g",
								bBackupCreated ? "true" : "false", vBackupPosition.x, vBackupPosition.y, vBackupPosition.z, fBackupDirection );
		//
		DebugTrace( "selection, type: %d, diffPos: ( %g, %g, %g ), diffDir: %g",
								eSelectionType, vPositionDifference.x, vPositionDifference.y, vPositionDifference.z, fDirectionDifference );
	}
};

// basement storage  


