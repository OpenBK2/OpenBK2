#include "StdAfx.h"

#include "GameRootExporter.h"

#include "ExporterMethods.h"
#include "../MapEditorLib/ExporterFactory.h"
#include "../MapEditorLib/ManipulatorManager.h"
#include "XMLExport.h"

REGISTER_EXPORTER_IN_DLL( GameRoot, CGameRootExporter )

EXPORT_RESULT CGameRootExporter::ExportObject( IManipulator* pManipulator,
																								const string &rszObjectTypeName,
																								const string &rszObjectName,
																								bool bForce,
																								EXPORT_TYPE exportType )
{
//	// CRAP{ to export old database to new xml
//	if ( exportType == ET_NO_REF && bForce )
//	{
//		NXMLExport::DumpAllObjects();
//		return ER_SUCCESS;
//	}
//	// CRAP}
	//
	if ( ( exportType != ET_BEFORE_REF ) && ( exportType != ET_NO_REF ) )
		return ER_SUCCESS;
	//
	string szIntroMovie;
	if ( CManipulatorManager::GetValue(&szIntroMovie, pManipulator, "IntroMovie") != false && !szIntroMovie.empty() )
	{
		if ( ExportFilesList( szIntroMovie, bForce, "Movies" ) == false )
			return ER_FAIL;
	}
	//
	return ER_SUCCESS;
}

