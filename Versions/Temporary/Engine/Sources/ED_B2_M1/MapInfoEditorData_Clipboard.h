#if !defined(__MAPINFO_EDITOR_DATA__CLIPBOARD__)
#define __MAPINFO_EDITOR_DATA__CLIPBOARD__
#pragma once

#include "MapInfoEditorData_Consts.h"
#include "MapInfoEditorData_ObjectInfo.h"

namespace NMapInfoEditor
{
	typedef vector<CPtr<SObjectInfo> > CObjectClipboardPartList;
	struct SObjectClipboard
	{
		CObjectClipboardPartList objectClipboardPartlist;
		CVec3 vPosition;
		//
		CVec3 vBackupPosition;
		bool bBackupCreated;
		//
		CVec3 vPositionDifference;

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		SObjectClipboard()
			: vPosition( VNULL3 ),
				vBackupPosition( VNULL3 ),
				bBackupCreated( false ),
				vPositionDifference( VNULL3 ) {}
		SObjectClipboard( const SObjectClipboard &rObjectClipboard )
			: objectClipboardPartlist( rObjectClipboard.objectClipboardPartlist ),
				vPosition( rObjectClipboard.vPosition ),
				vBackupPosition( rObjectClipboard.vBackupPosition ),
				bBackupCreated( rObjectClipboard.bBackupCreated ),
				vPositionDifference( rObjectClipboard.vPositionDifference ) {}
		SObjectClipboard& operator=( const SObjectClipboard &rObjectClipboard )
		{
			if( &rObjectClipboard != this )
			{
				objectClipboardPartlist = rObjectClipboard.objectClipboardPartlist;
				vPosition = rObjectClipboard.vPosition;
				vPositionDifference = rObjectClipboard.vPositionDifference;
				vBackupPosition = rObjectClipboard.vBackupPosition;
				bBackupCreated = rObjectClipboard.bBackupCreated;
			}
			return *this;
		}

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		inline bool IsEmpty() { return objectClipboardPartlist.empty(); }
		inline void Clear() { objectClipboardPartlist.clear(); }
		inline int Size() { return objectClipboardPartlist.size(); }
		void Insert( const SObjectInfo *pObjectInfo );

		//
		void BackupPosition();
		bool RollbackPosition();
		void MakeAbsolute();
		void MakeRelative();
		//
		//
		void Trace() const;
	};
};

#endif // !defined(__MAPINFO_EDITOR_DATA__CLIPBOARD__)

