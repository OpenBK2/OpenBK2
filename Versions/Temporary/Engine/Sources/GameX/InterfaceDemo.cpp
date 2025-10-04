#include "stdafx.h"
#include "InterfaceDemo.h"
#include "GameXClassIDs.h"
#include "System/VFSOperations.h"
#include "System/XmlSaver.h"
#include "Image/Targa.h"
#include "UI/BackgroundMutableTexture.h"

const char* DEMO_START_SEQUENCE_FILE_NAME = "Demo\\start_frames.xml";
const char* DEMO_FINAL_SEQUENCE_FILE_NAME = "Demo\\final_frames.xml";

static bool s_bExit = false;

// CInterfaceDemo::SFrame

int CInterfaceDemo::SFrame::operator&( IXmlSaver &saver )
{
	saver.Add( "FileName", &szFileName );
	saver.Add( "Delay", &fDelay );
	return 0;
}

// CInterfaceDemo

CInterfaceDemo::CInterfaceDemo() :
	CInterfaceScreenBase( "DemoScreen", "demo_screen" ),
	fDelay( 0.0f ),
	timeAbs( 0 ),
	bFinished( false ),
	nNextPicture( 0 ),
	bFinal( false )
{
}

bool CInterfaceDemo::Init()
{
	if ( !CInterfaceScreenBase::Init() )
		return false;

	AddScreen( this );

	return true;
}

bool CInterfaceDemo::Execute( const string &szSender, const string &szReaction )
{
	return false;
}

int CInterfaceDemo::Check( const string &szCheckName ) const
{
	return 0;
}

void CInterfaceDemo::MakeInterior()
{
	pMain = GetChildChecked<IWindow>( GetScreen(), "Main", true );
	
	LoadSequence();
	
	pPictureTexture = new CBackgroundMutableTexture();
	if ( pMain )
		pMain->SetBackground( pPictureTexture );
		
	NextPicture();
}

bool CInterfaceDemo::StepLocal( bool bAppActive )
{
	bool bResult = CInterfaceScreenBase::StepLocal( bAppActive );
	
	if ( bAppActive && !bFinished )
	{
		NTimer::STime timeAbsCur = Singleton<IGameTimer>()->GetAbsTime();
		if ( timeAbs != 0 )
		{
			fDelay -= (float)(timeAbsCur - timeAbs) * 0.001f;
			
			if ( fDelay <= 0.0f )
				NextPicture();
		}
		timeAbs = timeAbsCur;
	}
	
	return bResult;
}

void CInterfaceDemo::LoadSequence()
{
	CFileStream stream( NVFS::GetMainVFS(), bFinal ? DEMO_FINAL_SEQUENCE_FILE_NAME : DEMO_START_SEQUENCE_FILE_NAME );
	if ( stream.IsOk() )
	{
		CPtr<IXmlSaver> pSaver = CreateXmlSaver( &stream, SAVER_MODE_READ );
		pSaver->Add( "Frames", &frames );
	}
}

bool CInterfaceDemo::LoadImage( const string &szFileName )
{
	CFileStream stream( NVFS::GetMainVFS(), szFileName );
	bool bResult = stream.IsOk();
	if ( bResult )
	{
		CArray2D<DWORD> array;
		NImage::LoadTGAImage( array, &stream );
		pPictureTexture->Set( *reinterpret_cast< CArray2D<NGfx::SPixel8888>* >( &array ) );
	}
	return bResult;
}

void CInterfaceDemo::NextPicture()
{
	while ( nNextPicture < frames.size() ) 
	{
		SFrame &frame = frames[nNextPicture];
		nNextPicture++;
		bool bResult = LoadImage( frame.szFileName );
		if ( bResult )
		{
			fDelay = frame.fDelay;
			return;
		}
	}
	bFinished = true;
	if ( bFinal )
		NGlobal::ProcessCommand( L"final_exit" );
	else
		NGlobal::ProcessCommand( L"menu" );
}

void CInterfaceDemo::SetFinal( bool _bFinal )
{
	bFinal = _bFinal;

	MakeInterior();
}

// CICInterfaceDemo

void CICInterfaceDemo::PreCreate()
{
}

void CICInterfaceDemo::PostCreate( IInterface *pInterface )
{
	NMainLoop::PushInterface( pInterface );
	pInterface->SetFinal( bFinal );
}

void CICInterfaceDemo::Configure( const char *pszConfig )
{
	bFinal = false;
	if ( pszConfig != 0 && pszConfig[0] != 0 )
	{
		bFinal = true;
	}
}

void DemoScreen( const string &szID, const vector<wstring> &paramsSet, void *pContext )
{
	NMainLoop::Command( ML_COMMAND_DEMO_SCREEN, "" );
}

void DemoScreenFinal( const string &szID, const vector<wstring> &paramsSet, void *pContext )
{
	if ( !s_bExit )
	{
		NMainLoop::Command( ML_COMMAND_CLEAR_INTERFACES, "" );
		NMainLoop::Command( ML_COMMAND_DEMO_SCREEN, "final" );
	}
	s_bExit = true;
}

START_REGISTER(DemoCommands)
REGISTER_CMD( "demo_screen", DemoScreen );
REGISTER_CMD( "demo_screen_final", DemoScreenFinal );
FINISH_REGISTER

REGISTER_SAVELOAD_CLASS( ML_COMMAND_DEMO_SCREEN, CICInterfaceDemo )
REGISTER_SAVELOAD_CLASS( 0x17275AC1, CInterfaceDemo )


