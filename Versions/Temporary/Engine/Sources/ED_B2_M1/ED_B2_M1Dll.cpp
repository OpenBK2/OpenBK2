#include "stdafx.h"
//
#include "vendor/granny/include/granny.h"
#include "MapEditorLib/Interface_Editor.h"
#include "MapEditorLib/Interface_ChildFrame.h"
#include "MapEditorLib/MapEditorModule.h"
#include "MapEditorLib/InteractiveMayaExportTool.h"
#include "ED_Common/UIScene.h"
#include "ED_Common/TempAttributesTool.h"
#include "MapObjectDataExtractor.h"
#include "SpotDataExtractor.h"
#include "TileDataExtractor.h"
#include "VSODataExtractor.h"
#include "VSOWindow.h"
#include "HeightWindowV3.h"
#include "MapObjectWindow.h"

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
			const std::string szObjectFilterFileName = Singleton<IUserDataContainer>()->Get()->constUserData.szStartFolder + "Editor\\Filters.xml";
			{
				CFileStream stream( szObjectFilterFileName, CFileStream::WIN_READ_ONLY );
				if( stream.IsOk() )
				{
					Singleton<IObjectFilterCollector>()->Load( &stream );
				}
			}
			const std::string szDataExtractorFileName = Singleton<IUserDataContainer>()->Get()->constUserData.szStartFolder + "Editor\\Extractors.xml";
			{
				CFileStream stream( szDataExtractorFileName, CFileStream::WIN_READ_ONLY );
				if( stream.IsOk() )
					Singleton<IObjectCollector>()->Load( &stream );
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
	Singleton<IObjectCollector>()->RegisterDataExtractor( CMapObjectWindow::MAPOBJECT_EXTRACTOR_TYPE, new CMapObjectDataExtractor() );
	Singleton<IObjectCollector>()->RegisterDataExtractor( CMapObjectWindow::SPOT_EXTRACTOR_TYPE, new CSpotDataExtractor() );
	Singleton<IObjectCollector>()->RegisterDataExtractor( CHeightWindowV3::EXTRACTOR_TYPE, new CTileDataExtractor() );
	Singleton<IObjectCollector>()->RegisterDataExtractor( CVSOWindow::EXTRACTOR_TYPE, new CVSODataExtractor() );
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

HINSTANCE theEDB2M1Instance;
#ifdef NIVAL_DLL
BOOL WINAPI DllMain( HANDLE hInst, ULONG ul_reason_for_call, LPVOID lpReserved )
{
  if ( ul_reason_for_call == DLL_PROCESS_ATTACH )
	{
		// Для подключения ресурсов из DLL
		theEDB2M1Instance = (HINSTANCE)hInst;
	}
	return true;
}
#else
static struct SInitb2m1dll {
	SInitb2m1dll() { theEDB2M1Instance = GetModuleHandle( 0 ); }
} init;
#endif

static CEditorModuleB2M1 theEDB2Module;

IEditorModule* GetEditorModule1()
{
	return &theEDB2Module;
}


