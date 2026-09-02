#include "stdafx.h"

#include "TempAttributesTool.h"

#include "vendor/granny/include/granny.h"
#include "MapEditorLib/ManipulatorManager.h"
#include "MapEditorLib/Interface_Exporter.h"
#include "MapEditorLib/CommonExporterMethods.h"
#include "MapEditorLib/TextMapSettings.h"
#include "MapEditorLib/Interface_Logger.h"
#include "System/FilePath.h"
#include "System/FileUtils.h"
#include "Misc/StrProc.h"

namespace NMEGeomAttribs
{

class CTempAttributesTool : public IExportTool
{
	OBJECT_NOCOPY_METHODS( CTempAttributesTool );
	//
	struct SDesc
	{
		std::string szRootMesh;
		std::string szRootJoint;
		granny_file *pFile;
		granny_file_info *pFileInfo;
		//
		SDesc(): pFile( 0 ), pFileInfo( 0 ) {}
		~SDesc() { if ( pFile ) GrannyFreeFile( pFile ); }
	};
	//
	std::string szAttribsExportTemplate;
	std::string szAttribsExportSettings;
	//
	typedef std::list<SDesc> CDescsList;
	typedef std::unordered_map<std::string, CDescsList> CDescsMap; 
	CDescsMap descsMap;
	// load export settings and script template
	bool LoadSettings()
	{
		if ( !szAttribsExportTemplate.empty() && !szAttribsExportSettings.empty() )
			return true;
		//
		CTextMapSettings textMapSettings;
		SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
		pUserData->SerializeSettings( textMapSettings, "Geometry", SUserData::EXPORTER_SETINGS, SUserData::ST_LOAD	);
		if ( textMapSettings.IsEmpty() )
		{
			NLog::GetLogger()->Log( LT_ERROR, "Can't get 'Geometry' exporter settings\n" );
			return false;
		}
		szAttribsExportTemplate = textMapSettings.GetText( "ExportAttribs" );
		szAttribsExportSettings = GetGrannyExportSettingsFileName( "Attribs" );
		return !szAttribsExportTemplate.empty() && !szAttribsExportSettings.empty();
	}
	// export granny with desired root mesh and root joint and add 
	bool AddGrannyFile( SDesc *pDesc, const std::string &szFileName, const std::string &szRootMesh, const std::string &szRootJoint )
	{
		if ( LoadSettings() == false )
			return false;
		// form script to export temp file
		std::string szDstFileName = NFile::GetTempFileName() + ".gr2";
		NStr::ReplaceAllChars( &szDstFileName, '\\', '/' );
		std::string szSrcFileName = szFileName;
		NStr::ReplaceAllChars( &szSrcFileName, '\\', '/' );
		std::string szScript = StrFmt( szAttribsExportTemplate.c_str(), szDstFileName.c_str(), szSrcFileName.c_str(),
			                        szRootMesh.c_str(), szRootJoint.c_str(), szAttribsExportSettings.c_str() );
		szScript += ";\n";
		NFile::CreatePath( NFile::GetFilePath(szDstFileName) );
		if ( ExecuteMayaScript(szScript) == false )
			return false;
		//
		if ( WaitForFile(szDstFileName, 10000) == false )
			return false;
		// read granny file
		pDesc->pFile = GrannyReadEntireFile( szDstFileName.c_str() );
		::DeleteFile( szDstFileName.c_str() );	// delete temp file
		if ( pDesc->pFile == 0 )
			return false;
		pDesc->pFileInfo = GrannyGetFileInfo( pDesc->pFile );
		if ( pDesc->pFileInfo == 0 )
			return false;
		//
		pDesc->szRootMesh = szRootMesh;
		pDesc->szRootJoint = szRootJoint;
		return true;
	}
public:
	void StartExportTool() {}
	void FinishExportTool() { descsMap.clear(); }
	//
	granny_file_info *GetAttribs( const std::string &_szFileName, const std::string &szRootMesh, const std::string &szRootJoint )
	{
		if ( szRootMesh.empty() && szRootJoint.empty() )
			return 0;
		std::string szFileName;
		NStr::ToLower( &szFileName, _szFileName );
		NStr::ReplaceAllChars( &szFileName, '\\', '/' );
		CDescsMap::iterator posList = descsMap.find( szFileName );
		if ( posList == descsMap.end() )
		{
			CDescsList &lst = descsMap[szFileName];
			CDescsList::iterator pos = lst.insert( lst.end(), SDesc() );
			if ( AddGrannyFile(&(*pos), szFileName, szRootMesh, szRootJoint) == false )
			{
				lst.erase( pos );
				return 0;
			}
			return GetAttribs( szFileName, szRootMesh, szRootJoint );
		}
		else
		{
			for ( CDescsList::iterator itDesc = posList->second.begin(); itDesc != posList->second.end(); ++itDesc )
			{
				const bool bJoint = itDesc->szRootJoint == szRootJoint;
				const bool bMesh = itDesc->szRootMesh == szRootMesh;
				// exact
				if ( bJoint && bMesh )
					return itDesc->pFileInfo;
				// mesh == joint and one of them are matched
				if ( (szRootMesh == szRootJoint) && (bJoint || bMesh) )
					return itDesc->pFileInfo;
				// mesh or joint are empty and opposite matched
				if ( (szRootMesh.empty() && bJoint) || (szRootJoint.empty() && bMesh) )
					return itDesc->pFileInfo;
			}
			// create new entry
			CDescsList::iterator pos = posList->second.insert( posList->second.end(), SDesc() );
			if ( AddGrannyFile(&(*pos), szFileName, szRootMesh, szRootJoint) == false )
			{
				posList->second.erase( pos );
				return 0;
			}
			return GetAttribs( szFileName, szRootMesh, szRootJoint );
		}
	}
};

static CObj<CTempAttributesTool> pTempAttributesTool;
IExportTool *GetOrCreateTempAttributesExportTool() 
{ 
	if ( pTempAttributesTool == 0 )
		pTempAttributesTool = new CTempAttributesTool(); 
	return pTempAttributesTool; 
}
void DestroyTempAttributesExportTool() { pTempAttributesTool = 0; }

granny_file_info *GetAttribsByVisObj( IManipulator *pMan )
{
	if ( pMan == 0 )
		return 0;
	int nNumSeasons = -1;
	CManipulatorManager::GetValue( &nNumSeasons, pMan, "Models" );
	if ( nNumSeasons <= 0 )
		return 0;
	// try summer model
	for ( int i = 0; i < nNumSeasons; ++i )
	{
		std::string szSeason;
		CManipulatorManager::GetValue( &szSeason, pMan, StrFmt("Models.[%d].Season", i) );
		if ( szSeason == "SEASON_SUMMER" )
		{
			CPtr<IManipulator> pModelMan = 
				CManipulatorManager::CreateManipulatorFromReference( StrFmt( "Models.[%d].Model", i ), pMan, 0, 0, 0 );
			return GetAttribsByModel( pModelMan );
		}
	}
	// try any model
	CPtr<IManipulator> pModelMan = 
		CManipulatorManager::CreateManipulatorFromReference( "Models.[0].Model", pMan, 0, 0, 0 );
	return GetAttribsByModel( pModelMan );
}

granny_file_info *GetAttribsByModel( IManipulator *pMan )
{
	if ( pMan == 0 )
		return 0;
	CPtr<IManipulator> pGeomMan = 
		CManipulatorManager::CreateManipulatorFromReference( "Geometry", pMan, 0, 0, 0 );
	return GetAttribsByGeometry( pGeomMan );
}

granny_file_info *GetAttribsBySkeleton( IManipulator *pMan )
{
	if ( pMan == 0 )
		return 0;
	std::string szSrcFileName;
	CManipulatorManager::GetValue( &szSrcFileName, pMan, "SrcName" );
	std::string szRootJoint;
	CManipulatorManager::GetValue( &szRootJoint, pMan, "RootJoint" );
	if ( szSrcFileName.empty() || szRootJoint.empty() )
		return 0;
	//
	const SUserData *pUD = Singleton<IUserDataContainer>()->Get();
	return GetAttribs( pUD->constUserData.szExportSourceFolder + szSrcFileName, "", szRootJoint );
}

granny_file_info *GetAttribsByGeometry( IManipulator *pMan )
{
	if ( pMan == 0 )
		return 0;
	std::string szSrcFileName;
	CManipulatorManager::GetValue( &szSrcFileName, pMan, "SrcName" );
	std::string szRootMesh;
	CManipulatorManager::GetValue( &szRootMesh, pMan, "RootMesh" );
	std::string szRootJoint;
	CManipulatorManager::GetValue( &szRootJoint, pMan, "RootJoint" );
	if ( szSrcFileName.empty() || szRootJoint.empty() || szRootMesh.empty() )
		return 0;
	//
	const SUserData *pUD = Singleton<IUserDataContainer>()->Get();
	return GetAttribs( pUD->constUserData.szExportSourceFolder + szSrcFileName, szRootMesh, szRootJoint );
}

granny_file_info *GetAttribs( const std::string &szFileName, const std::string &szRootMesh, const std::string &szRootJoint )
{
	if ( szFileName.empty() || (pTempAttributesTool == 0) )
		return 0;
	//
	return pTempAttributesTool->GetAttribs( szFileName, szRootMesh, szRootJoint );
}

void GetAttributesFromBone( void *pDstData, granny_bone *pBone, const char **ppszAttribNames, const int nNumAttribs )
{
	std::vector<granny_data_type_definition> gdtd( nNumAttribs + 1 );
	memset( &(gdtd[0]), 0, sizeof(gdtd[0]) * gdtd.size() );

	for ( int i = 0; i < nNumAttribs; ++i )
	{
		gdtd[i].Type = GrannyReal32Member;
		gdtd[i].Name = (char*)( ppszAttribNames[i] );
	}
	gdtd[nNumAttribs].Type = GrannyEndMember;
	//
	GrannyConvertSingleObject( pBone->ExtendedData.Type, pBone->ExtendedData.Object, &(gdtd[0]), pDstData, nullptr );
}

};

