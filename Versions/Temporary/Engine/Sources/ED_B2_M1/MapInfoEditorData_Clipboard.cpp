#include "StdAfx.h"

#include "MapInfoEditorData_Clipboard.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


namespace NMapInfoEditor
{
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SObjectClipboard::BackupPosition()
	{
		vBackupPosition = vPosition;
		bBackupCreated = true;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SObjectClipboard::RollbackPosition()
	{
		if ( bBackupCreated )
		{
			vPosition = vBackupPosition;
		}
		return bBackupCreated;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SObjectClipboard::MakeAbsolute()
	{
		for ( CObjectClipboardPartList::const_iterator itObjectClipboardPart = objectClipboardPartlist.begin(); itObjectClipboardPart != objectClipboardPartlist.end(); ++itObjectClipboardPart )
		{
			if ( ( *itObjectClipboardPart ) )
			{
				( *itObjectClipboardPart )->vPosition += vPosition;
			}
		}
		vPosition = VNULL3;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SObjectClipboard::MakeRelative()
	{
		vPosition = VNULL3;
		int nClipboardPartCount = 0;
		for ( CObjectClipboardPartList::const_iterator itObjectClipboardPart = objectClipboardPartlist.begin(); itObjectClipboardPart != objectClipboardPartlist.end(); ++itObjectClipboardPart )
		{
			if ( ( *itObjectClipboardPart ) )
			{
				vPosition += ( *itObjectClipboardPart )->vPosition;
				++nClipboardPartCount;
			}
		}
		if ( nClipboardPartCount > 0 )
		{
			vPosition /= ( nClipboardPartCount * 1.0f );
		}
		// установим относительные координаты
		for ( CObjectClipboardPartList::const_iterator itObjectClipboardPart = objectClipboardPartlist.begin(); itObjectClipboardPart != objectClipboardPartlist.end(); ++itObjectClipboardPart )
		{
			if ( ( *itObjectClipboardPart ) )
			{
				( *itObjectClipboardPart )->vPosition -= vPosition;
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SObjectClipboard::Insert( const SObjectInfo *pObjectInfo )
	{ 
		CObjectClipboardPartList::iterator itObjectClipboardPart = objectClipboardPartlist.insert( objectClipboardPartlist.end(), pObjectInfo->CallDuplicate() );
		if ( ( *itObjectClipboardPart ) )
		{
			( *itObjectClipboardPart )->CopySelf();
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SObjectClipboard::Trace() const
	{
		DebugTrace( "clipboard, begin" );
		for ( CObjectClipboardPartList::const_iterator itObjectClipboardPart = objectClipboardPartlist.begin(); itObjectClipboardPart != objectClipboardPartlist.end(); ++itObjectClipboardPart )
		{
			if ( ( *itObjectClipboardPart ) )
			{
				( *itObjectClipboardPart )->Trace();
			}
		}
		DebugTrace( "clipboard, end" );
		//
		DebugTrace( "clipboard, pos: ( %g, %g, %g )",
								vPosition.x, vPosition.y, vPosition.z );
		//
		DebugTrace( "clipboard, %s, backupPos: ( %g, %g, %g )",
								bBackupCreated ? "true" : "false", vBackupPosition.x, vBackupPosition.y, vBackupPosition.z );
		//
		DebugTrace( "clipboard, diffPos: ( %g, %g, %g )",
								vPositionDifference.x, vPositionDifference.y, vPositionDifference.z );
	}
};

// basement storage  

