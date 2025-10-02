#include "StdAfx.h"

#include "CampaignExporter.h"

#include "ExporterMethods.h"
#include "../MapEditorLib/ExporterFactory.h"
#include "../MapEditorLib/ManipulatorManager.h"

REGISTER_EXPORTER_IN_DLL( Campaign, CCampaignExporter )

EXPORT_RESULT CCampaignExporter::ExportObject( IManipulator* pManipulator,
																							const string &rszObjectTypeName,
																							const string &rszObjectName,
																							bool bForce,
																							EXPORT_TYPE exportType )
{
	if ( ( exportType != ET_BEFORE_REF ) && ( exportType != ET_NO_REF ) )
		return ER_SUCCESS;
	// intro movie
	string szIntroMovie;
	if ( CManipulatorManager::GetValue(&szIntroMovie, pManipulator, "IntroMovie") != false && !szIntroMovie.empty() )
	{
		if ( ExportFilesList( szIntroMovie, bForce, "Movies" ) == false )
			return ER_FAIL;
	}
	// outro movie
	string szOutroMovie;
	if ( CManipulatorManager::GetValue(&szIntroMovie, pManipulator, "OutroMovie") != false && !szOutroMovie.empty() )
	{
		if ( ExportFilesList( szOutroMovie, bForce, "Movies" ) == false )
			return ER_FAIL;
	}
	//
	return ER_SUCCESS;
}


