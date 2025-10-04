#include "stdafx.h"

#include "BasicExporter.h"
#include "MapEditorLib/CommonExporterMethods.h"

void CBasicExporter::Log( ELogOutputType eLogOutputType, const string &szText ) const
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
			string szText = StrFmt("Can't get %s exporter settings,\n"
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

bool CBasicExporter::ExecuteMayaScript( const string &szScript )
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

bool CBasicExporter::StartExport( const string &rszObjectTypeName, bool bForce )
{
	pMayaProcess = CInteractiveMaya::Get();
	szObjectTypeName = rszObjectTypeName;
	return true;
}

void CBasicExporter::FinishExport( const string &rszObjectTypeName, bool bForce )
{
	pMayaProcess = 0;
}


