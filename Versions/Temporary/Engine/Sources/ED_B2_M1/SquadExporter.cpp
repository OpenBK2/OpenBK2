#include "StdAfx.h"

#include "SquadExporter.h"

#include "../MapEditorLib/ExporterFactory.h"
#include "../MapEditorLib/ManipulatorManager.h"
#include "../MapEditorLib/Interface_Logger.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


REGISTER_EXPORTER_IN_DLL( SquadRPGStats, CSquadExporter )

EXPORT_RESULT CSquadExporter::CheckObject( IManipulator* pManipulator,
																					 const string &rszObjectTypeName,
																				 	 const string &rszObjectName,
																					 bool bExport,
																					 EXPORT_TYPE exportType )
{
	if ( exportType == ET_AFTER_REF ) 
		return ER_SUCCESS;
	//
	ILogger *pLogger = NLog::GetLogger();
	//
	int nNumFormations = 0;
	if ( CManipulatorManager::GetValue( &nNumFormations, pManipulator, "formations" ) == false )
	{
		pLogger->Log( LT_ERROR, StrFmt("Can't get number of formations for squad\n") );
		pLogger->Log( LT_ERROR, StrFmt("\tSquad: %s\n", rszObjectName.c_str()) );
		return ER_FAIL;
	}
	// check number of formations
	if ( nNumFormations == 0 ) 
	{
		pLogger->Log( LT_ERROR, StrFmt("Empty formations array for squad\n") );
		pLogger->Log( LT_ERROR, StrFmt("\tSquad: %s\n", rszObjectName.c_str()) );
		return ER_FAIL;
	}
	//
	vector<string> formationTypes;
	// check 'changesByEvent' for each formation and collect formation types
	for ( int i = 0; i < nNumFormations; ++i ) 
	{
		string szFormationMoveType;
		CManipulatorManager::GetValue( &szFormationMoveType, pManipulator, StrFmt("formations.[%d].type", i) );
		if ( !szFormationMoveType.empty() && szFormationMoveType != " " ) 
			formationTypes.push_back( szFormationMoveType );
		//
		int nNumChangesByEvent = 0;
		if ( CManipulatorManager::GetValue( &nNumChangesByEvent, pManipulator, StrFmt("formations.[%d].changesByEvent", i) ) == false )
		{
			pLogger->Log( LT_ERROR, StrFmt("Can't get number of 'changesByEvent' for formation in squad\n") );
			pLogger->Log( LT_ERROR, StrFmt("\tSquad: %s\n", rszObjectName.c_str()) );
			pLogger->Log( LT_ERROR, StrFmt("\tFormation: %d\n", i) );
			return ER_FAIL;
		}
		//
		for ( int j = 0; j < nNumChangesByEvent; ++j ) 
		{
			int nFormation = -1;
			CManipulatorManager::GetValue( &nFormation, pManipulator, StrFmt("formations.[%d].changesByEvent.[%d]", i, j) );
			if ( nFormation < -1 || nFormation >= nNumFormations ) 
			{
				pLogger->Log( LT_ERROR, StrFmt("Invalid 'changesByEvent' - no formation to change to\n") );
				pLogger->Log( LT_ERROR, StrFmt("\tSquad: %s\n", rszObjectName.c_str()) );
				pLogger->Log( LT_ERROR, StrFmt("\tFormation: %d\n", i) );
				pLogger->Log( LT_ERROR, StrFmt("\tchangesByEvent: %d\n", j) );
				pLogger->Log( LT_ERROR, StrFmt("\tValue: %d\n", nFormation) );
				return ER_FAIL;
			}
		}
		/*
		if ( nNumChangesByEvent == 0 ) 
		{
			pLogger->Log( LT_ERROR, StrFmt("Formation must have at least one 'changesByEvent' (-1)\n") );
			pLogger->Log( LT_ERROR, StrFmt("\tObject: %s\n", rszObjectName.c_str()) );
			pLogger->Log( LT_ERROR, StrFmt("\tFormation: %d\n", i) );
		}
		*/
	}
	// check formation types uniqueness
	for ( vector<string>::const_iterator it1 = formationTypes.begin(); it1 != formationTypes.end(); ++it1 ) 
	{
		for ( vector<string>::const_iterator it2 = it1 + 1; it2 != formationTypes.end(); ++it2 ) 
		{
			if ( (*it1) == (*it2) )
			{
				pLogger->Log( LT_ERROR, StrFmt("Non-unique formation type\n") );
				pLogger->Log( LT_ERROR, StrFmt("\tObject: %s\n", rszObjectName.c_str()) );
				pLogger->Log( LT_ERROR, StrFmt("\tFormation type: %s\n", it1->c_str()) );
				return ER_FAIL;
			}
		}
	}
	//
	return ER_SUCCESS;
}


