#include "stdafx.h"

#include "FileRead.h"
#include "FileNode.h"
#include "../Misc/StrProc.h"

namespace NLang
{

static string szFileName = "";
static CDataStream *pStream = 0;
static int nTypeNumber = 0;

static struct SFileStreamDestroyer
{
	~SFileStreamDestroyer()
	{
		if ( pStream )
			delete pStream;
	}
} fileStreamDestroyer;

const char* GetParsingFileName()
{
	return szFileName.c_str();
}

bool OpenFile( const string &_szFileName )
{
	szFileName = _szFileName;

	string szStreamFileName = szFileName;
	NStr::ReplaceAllChars( &szStreamFileName, '/', '\\' );

	NStr::ToLowerASCII( &szFileName );

	if ( pStream )
		delete pStream;

	pStream = new CFileStream( szStreamFileName, CFileStream::WIN_READ_ONLY );
	if ( pStream->IsOk() )
	{
		CFileNode *pRootFile = GetRootFile();
		pRootFile->AddInclude( szFileName );
		pRootFile->GetInclude( szFileName )->SetExist();
	}

	return pStream->IsOk();
}

int ReadData( char *pBuf, int nMaxSize )
{
	int nCurPos = pStream->GetPosition();
	if ( nCurPos == 0 )
	{
		int nSizeToRead = Min( pStream->GetSize(), nMaxSize - 1 );
		pBuf[0] = '\n';
		pStream->Read( pBuf + 1, nSizeToRead );
		return nSizeToRead + 1;
	}
	else
	{
		int nSizeToRead = Min( pStream->GetSize() - nCurPos, nMaxSize );
		pStream->Read( pBuf, nSizeToRead );
		return nSizeToRead;	
	}
}

}

