#include "stdafx.h"

#include "ED_B2_M1_export.h"

//
#include "vendor/granny/include/granny.h"
#include "MapEditorLib/Interface_Editor.h"
#include "MapEditorLib/Interface_ChildFrame.h"
#include "MapEditorLib/MapEditorModule.h"
#include "MapEditorLib/Resources.h"
#include "MapEditorLib/InteractiveMayaExportTool.h"
#include "ED_Common/UIScene.h"
#include "ED_Common/TempAttributesTool.h"
#include "System/FilePath.h"
#include <fmt/format.h>

#include "MapObjectDataExtractor.h"
#include "SpotDataExtractor.h"
#include "TileDataExtractor.h"
#include "VSODataExtractor.h"

namespace
{
	CObj<CInteractiveMayaExportTool> pInteractiveMayaExportTool;

	void PrintGrannyVersions()
	{
		ILogger *pLogger = NLog::GetLogger();
#define STRINGIZE_INNER(x)                    #x
#define STRINGIZE(x)                          STRINGIZE_INNER(x)
#define GRANNY_VERSION_STR(number, release)	  number " (" release ")"

		// Print out what version of the .h file we're using
		pLogger->Log( LT_NORMAL, "Compiled with " );
		pLogger->Log( LT_IMPORTANT, GRANNY_VERSION_STR(GrannyProductVersion, STRINGIZE(GrannyProductReleaseName)) );
		pLogger->Log( LT_NORMAL, " granny version (.h).\n" );

		// Print out what version of the .dll we're using
		pLogger->Log( LT_NORMAL, "Using granny2.dll of version " );
		pLogger->Log( LT_IMPORTANT, GrannyGetVersionString());
		pLogger->Log( LT_NORMAL, ".\n" );
#undef GRANNY_VERSION_STR

		if ( !GrannyVersionsMatch )
		{
			pLogger->Log( LT_ERROR, "WARNING: 'compiled with' and 'using' granny version mismatch.\n" );
		}
	}
	//
	void LoadFilters()
	{
		try
		{
			// JoinPath rather than a concatenation with the separator written out,
			// which is how "Editor\Filters.xml" got onto the end of an otherwise
			// good start folder and named one file that does not exist off
			// Windows. Without the filters every palette keyed on them is built
			// with an empty filter list, so the object palette offers nothing to
			// place: no units, no buildings, no terrain objects.
			const std::string szObjectFilterFileName =
					NFile::JoinPath( Singleton<IUserDataContainer>()->Get()->constUserData.szStartFolder, "Editor", "Filters.xml" );
			{
				CFileStream stream( szObjectFilterFileName, CFileStream::WIN_READ_ONLY );
				if( stream.IsOk() )
				{
					Singleton<IObjectFilterCollector>()->Load( &stream );
				}
				else
				{
					// Said out loud, because a palette with nothing in it is all
					// the report this used to make.
					NLog::GetLogger()->Log( LT_ERROR, fmt::format( "Can't read the object filters {}\n", szObjectFilterFileName ) );
				}
			}
			const std::string szDataExtractorFileName =
					NFile::JoinPath( Singleton<IUserDataContainer>()->Get()->constUserData.szStartFolder, "Editor", "Extractors.xml" );
			{
				CFileStream stream( szDataExtractorFileName, CFileStream::WIN_READ_ONLY );
				if( stream.IsOk() )
				{
					Singleton<IObjectCollector>()->Load( &stream );
				}
				else
				{
					NLog::GetLogger()->Log( LT_ERROR, fmt::format( "Can't read the data extractors {}\n", szDataExtractorFileName ) );
				}
			}
		}
		catch ( ... ) 
		{
			NLog::GetLogger()->Log( LT_ERROR, "Can't load object filters\n" );
		}
	}
}


class CEditorModuleB2M1 : public IEditorModule
{
	void ModuleStartup();
	void ModuleShutdown();
	void ModuleCreate();
	void ModuleDestroy();
	void ModuleCreateControls();
	void ModulePostCreateControls();
	void ModulePreDestroyControls();
	void ModuleDestroyControls();
	void ModulePostCreateMainFrame();
};

void CEditorModuleB2M1::ModuleStartup()
{
	// не существует еще <все> кроме Singleton<IUserDataContainer>()
	// This module's resources register themselves: its generated tables are
	// namespace-scope objects, constructed when the module loads.
}

void CEditorModuleB2M1::ModuleShutdown()
{
	// существует только Singleton<IUserDataContainer>()
}


void CEditorModuleB2M1::ModuleCreate()
{
	// существует уже <все>
	// вызывается до создания MainFrame
	//
	Singleton<IObjectCollector>()->RegisterDataExtractor( new CMapObjectDataExtractor() );
	// The extractor types, which the palettes' filters are keyed on too. They
	// were constants on the MFC palettes (CMapObjectWindow, CHeightWindowV3,
	// CVSOWindow), which are gone.
	Singleton<IObjectCollector>()->RegisterDataExtractor( "MAPOBJECT", new CMapObjectDataExtractor() );
	Singleton<IObjectCollector>()->RegisterDataExtractor( "SPOT", new CSpotDataExtractor() );
	Singleton<IObjectCollector>()->RegisterDataExtractor( "TILE", new CTileDataExtractor() );
	Singleton<IObjectCollector>()->RegisterDataExtractor( "VSO", new CVSODataExtractor() );
	LoadFilters();
	//
	Singleton<IBuilderContainer>()->Create( "AnimB2" );
	Singleton<IBuilderContainer>()->Create( "AckSetRPGStats" );
	Singleton<IBuilderContainer>()->Create( "VisObj" );
	//
	Singleton<IExporterContainer>()->Create( "MapInfo" );
	//
	Singleton<IEditorContainer>()->Create( "Model" );
	Singleton<IEditorContainer>()->Create( "MapInfo" );
	Singleton<IEditorContainer>()->AddExtendObjectType( "Model", "Effect" );
	Singleton<IEditorContainer>()->AddExtendObjectType( "Model", "ComplexEffect" );
	Singleton<IEditorContainer>()->AddExtendObjectType( "Model", "Material" );
	Singleton<IEditorContainer>()->AddExtendObjectType( "Model", "Geometry" );
	Singleton<IEditorContainer>()->AddExtendObjectType( "Model", "VisObj" );
	Singleton<IEditorContainer>()->AddExtendObjectType( "Model", "MechUnitRPGStats" );
	Singleton<IEditorContainer>()->Create( "BuildingRPGStats" );
	Singleton<IEditorContainer>()->Create( "SquadRPGStats" );
	//
	NSingleton::RegisterSingleton( CreateUIScene(), IUIScene::tidTypeID );
	//
	pInteractiveMayaExportTool = new CInteractiveMayaExportTool;
	Singleton<IExporterContainer>()->RegisterExportTool( pInteractiveMayaExportTool );
	Singleton<IExporterContainer>()->RegisterExportTool( NMEGeomAttribs::GetOrCreateTempAttributesExportTool() );
}

void CEditorModuleB2M1::ModuleDestroy()
{
	// существует еще <все>
	// вызывается после разрушения MainFrame
	//
	Singleton<IExporterContainer>()->UnRegisterExportTool( NMEGeomAttribs::GetOrCreateTempAttributesExportTool() );
	NMEGeomAttribs::DestroyTempAttributesExportTool();
	//
	Singleton<IExporterContainer>()->UnRegisterExportTool( pInteractiveMayaExportTool );
	pInteractiveMayaExportTool = 0;
	//
	NSingleton::UnRegisterSingleton( IUIScene::tidTypeID );
}

void CEditorModuleB2M1::ModuleCreateControls()
{
	// существует уже <все>
	// вызывается после создания MainFrame, перед LoadBarState
}

void CEditorModuleB2M1::ModulePostCreateControls()
{
	// существует уже <все>
	// вызывается после создания MainFrame, после LoadBarState
	PrintGrannyVersions();
}

void CEditorModuleB2M1::ModulePreDestroyControls()
{
	// существует еще <все>
	// вызывается перед разрушением MainFrame, перед SaveBarState
}

void CEditorModuleB2M1::ModuleDestroyControls()
{
	// существует еще <все>
	// вызывается после создания MainFrame, после SaveBarState
}

void CEditorModuleB2M1::ModulePostCreateMainFrame()
{
	// вызывается после создания MainFrame и PostStorageInitialize()
	Singleton<IChildFrameContainer>()->Create( "__CHILD_FRAME_DX_SCENE_LABEL__" );
}

// theEDB2M1Instance and the DllMain that set it are gone, as ED_Common's are.
// Its comment said it was "для подключения ресурсов из DLL", and the last
// reader of it was the ::LoadMenu in ModelState that stopped finding anything
// when this module's resources became generated C++ tables.

static CEditorModuleB2M1 theEDB2Module;

ED_B2_M1_EXPORT IEditorModule* GetEditorModule1()
{
	return &theEDB2Module;
}


