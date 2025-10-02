#pragma once


namespace NMapInfoEditor
{
	struct SObjectSelectionPart
	{
		CVec3 vPosition;
		float fDirection;
		//
		CVec3 vBackupPosition;
		float fBackupDirection;
		bool bBackupCreated;

		SObjectSelectionPart()
			:	vPosition( VNULL3 ),
				fDirection( 0.0f ),
				vBackupPosition( VNULL3 ),
				fBackupDirection( 0.0f ),
				bBackupCreated( false ) {}
		SObjectSelectionPart( const SObjectSelectionPart &rObjectSelectionPart )
			: vPosition( rObjectSelectionPart.vPosition ),
				fDirection( rObjectSelectionPart.fDirection ),
				vBackupPosition( rObjectSelectionPart.vBackupPosition ),
				fBackupDirection( rObjectSelectionPart.fBackupDirection ),
				bBackupCreated( rObjectSelectionPart.bBackupCreated ) {}
		SObjectSelectionPart& operator=( const SObjectSelectionPart &rObjectSelectionPart )
		{
			if( &rObjectSelectionPart != this )
			{
				vPosition =  rObjectSelectionPart.vPosition;
				fDirection =  rObjectSelectionPart.fDirection;
				vBackupPosition = rObjectSelectionPart.vBackupPosition;
				fBackupDirection = rObjectSelectionPart.fBackupDirection;
				bBackupCreated = rObjectSelectionPart.bBackupCreated;
			}
			return *this;
		}	

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		void BackupPosition();
		bool RollbackPosition();
	};
	typedef hash_map<UINT, SObjectSelectionPart> CObjectSelectionPartMap;

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct SObjectSelection
	{
		enum ESelectionType
		{
			ST_UNKNOWN		= 0,
			ST_CENTER			= 1,
			ST_DIRECTION	= 2,
			ST_OUT				= 3,
			ST_COUNT			= 4,
		};
		//
		CObjectSelectionPartMap objectSelectionPartMap;
		CVec3 vPosition;
		float fDirection;
		//
		CVec3 vBackupPosition;
		float fBackupDirection;
		bool bBackupCreated;
		//
		ESelectionType eSelectionType;
		CVec3 vPositionDifference;
		float fDirectionDifference;
		//
		SObjectSelection()
			: vPosition( VNULL3 ),
				fDirection( 0.0f ),
				eSelectionType( ST_UNKNOWN ),
				vBackupPosition( VNULL3 ),
				fBackupDirection( 0.0f ),
				bBackupCreated( false ),
				vPositionDifference( VNULL3 ),
				fDirectionDifference( 0.0f ) {}
		SObjectSelection( const SObjectSelection &rObjectSelection )
			: objectSelectionPartMap( rObjectSelection.objectSelectionPartMap ),
				vPosition( rObjectSelection.vPosition ),
				fDirection( rObjectSelection.fDirection ),
				vBackupPosition( rObjectSelection.vBackupPosition ),
				fBackupDirection( rObjectSelection.fBackupDirection ),
				bBackupCreated( rObjectSelection.bBackupCreated ),
				eSelectionType( rObjectSelection.eSelectionType ),
				vPositionDifference( rObjectSelection.vPositionDifference ),
				fDirectionDifference( rObjectSelection.fDirectionDifference ) {}
		SObjectSelection& operator=( const SObjectSelection &rObjectSelection )
		{
			if( &rObjectSelection != this )
			{
				objectSelectionPartMap = rObjectSelection.objectSelectionPartMap;
				vPosition = rObjectSelection.vPosition;
				fDirection = rObjectSelection.fDirection;
				eSelectionType = rObjectSelection.eSelectionType;
				vPositionDifference = rObjectSelection.vPositionDifference;
				fDirectionDifference = rObjectSelection.fDirectionDifference;
				vBackupPosition = rObjectSelection.vBackupPosition;
				fBackupDirection = rObjectSelection.fBackupDirection;
				bBackupCreated = rObjectSelection.bBackupCreated;
			}
			return *this;
		}

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		inline bool IsEmpty() { return objectSelectionPartMap.empty(); }

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		void ResetPickSelection();
		void Clear();
		void BackupPosition();
		bool RollbackPosition();
		void MakeAbsolute();
		void MakeRelative();
		//
		void Trace() const;
	};
};


