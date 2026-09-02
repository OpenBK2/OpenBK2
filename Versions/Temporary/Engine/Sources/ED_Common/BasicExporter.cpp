#include "stdafx.h"
#include <fmt/format.h>

#include "BasicExporter.h"
#include "MapEditorLib/CommonExporterMethods.h"

void CBasicExporter::Log( ELogOutputType eLogOutputType, const std::string &szText ) const
{
	NLog::GetLogger()->Log( eLogOutputType, szText );
}

bool CBasicExporter::LoadExporterSettings() const
{
	if ( textMapSettings.IsEmpty() )
	{
		if ( szObjectTypeName.empty() ) 
			return false;
		SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
		pUserData->SerializeSettings( textMapSettings, szObjectTypeName, SUserData::EXPORTER_SETINGS, SUserData::ST_LOAD	);
		if ( textMapSettings.IsEmpty() )
		{
			std::string szText = fmt::format("Can't get {} exporter settings,\n"
				"check UserData.xml in \"ObjectTypeDataMap\" section.\n",
				szObjectTypeName.c_str()
				);
			Log( LT_ERROR, szText );
			return false;
		}
	}
	return true;
}

const char *CBasicExporter::GetTextTemplate( const char *pszTemplateName ) const
{
	if ( LoadExporterSettings() )
		return textMapSettings.GetText( pszTemplateName );
	else
		return "";
}

bool CBasicExporter::ExecuteMayaScript( const std::string &szScript )
{
	if ( StartupMayaProcess( pMayaProcess ) )
	{
		if ( pMayaProcess->TransactCommand( szScript, "0" ) == false )
		{
			Log( LT_ERROR, "Export from Maya failed\n" );
			return false;
		}
	}
	return true;
}

bool CBasicExporter::StartExport( const std::string &rszObjectTypeName, bool bForce )
{
	pMayaProcess = CInteractiveMaya::Get();
	szObjectTypeName = rszObjectTypeName;
	return true;
}

void CBasicExporter::FinishExport( const std::string &rszObjectTypeName, bool bForce )
{
	pMayaProcess = 0;
}


