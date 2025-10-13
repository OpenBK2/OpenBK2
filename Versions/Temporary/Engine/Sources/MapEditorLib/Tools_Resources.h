#pragma once

#include "System/VFSOperations.h"


#include <cstdint>
// Legacy
template<class TResource>
bool LoadXMLResource( const std::string &rszResourceFileName, const std::string &rszExtention, const std::string &rszChunkLabel, TResource &rResource )
{
	std::string szResourceFileName = rszResourceFileName;
	CStringManager::ExtendFileExtention( &szResourceFileName, rszExtention );
	return LoadXMLResource( szResourceFileName, rszChunkLabel, rResource );
}

// New Legacy

struct SFileStreamHolder
{
	CDataStream *pStream;

	SFileStreamHolder() : pStream( 0 ) { }
	~SFileStreamHolder()
	{
		delete pStream;
	}
};


void OpenStreamHolder( SFileStreamHolder *pStreamHolder, const std::string &rszTextPath );
void CreateStreamHolder( SFileStreamHolder *pStreamHolder, const std::string &rszTextPath );


template<class TResource>
bool LoadXMLResource( const std::string &rszResourceFileName, const std::string &rszChunkLabel, TResource &rResource )
{
	SFileStreamHolder streamHolder;
	OpenStreamHolder( &streamHolder, rszResourceFileName );
	if ( streamHolder.pStream && streamHolder.pStream->IsOk() )
	{
		if ( CPtr<IXmlSaver> pSaver = CreateXmlSaver( streamHolder.pStream, SAVER_MODE_READ ) )
		{
			pSaver->Add( rszChunkLabel.c_str(), &rResource );
			return true;
		}
	}
	return false;
}


// Legacy
template<class TResource>
bool LoadTypedSuperXMLResource( const std::string &rszResourceFileName, const std::string &rszExtention, TResource &rResource )
{
	std::string szResourceFileName = rszResourceFileName;
	CStringManager::ExtendFileExtention( &szResourceFileName, rszExtention );
	return LoadTypedSuperXMLResource( szResourceFileName, rResource );
}


template<class TResource>
bool LoadTypedSuperXMLResource( const std::string &rszResourceFileName, TResource &rResource )
{
	SFileStreamHolder streamHolder;
	OpenStreamHolder( &streamHolder, rszResourceFileName );
	if ( streamHolder.pStream && streamHolder.pStream->IsOk() )
	{
		if ( CPtr<IXmlSaver> pSaver = CreateXmlSaver( streamHolder.pStream, SAVER_MODE_READ ) )
		{
			pSaver->AddTypedSuper( &rResource );
			return true;
		}
	}
	return false;
}


template<class TResource>
bool LoadBINResource( const std::string &rszResourceFileName, const std::string &rszExtention, int nChunkNumber, TResource &rResource )
{
	std::string szResourceFileName = rszResourceFileName;
	CStringManager::ExtendFileExtention( &szResourceFileName, rszExtention );
	return LoadBINResource( szResourceFileName, nChunkNumber, rResource );
}


template<class TResource>
bool LoadBINResource( const std::string &rszResourceFileName, int nChunkNumber, TResource &rResource )
{
	SFileStreamHolder streamHolder;
	OpenStreamHolder( &streamHolder, rszResourceFileName );
	if ( streamHolder.pStream && streamHolder.pStream->IsOk() )
	{
		if ( CPtr<IBinSaver> pSaver = CreateBinSaver( streamHolder.pStream, SAVER_MODE_READ ) )
		{
			pSaver->Add( nChunkNumber, &rResource );
			return true;
		}
	}
	return false;
}


template<class TResource>
bool LoadTypedSuperBINResource( const std::string &rszResourceFileName, const std::string &rszExtention, TResource &rResource )
{
	std::string szResourceFileName = rszResourceFileName;
	CStringManager::ExtendFileExtention( &szResourceFileName, rszExtention );
	return LoadTypedSuperBINResource( szResourceFileName, rResource );
}


template<class TResource>
bool LoadTypedSuperBINResource( const std::string &rszResourceFileName, TResource &rResource )
{
	SFileStreamHolder streamHolder;
	OpenStreamHolder( &streamHolder, rszResourceFileName );
	if ( streamHolder.pStream && streamHolder.pStream->IsOk() )
	{
		if ( CPtr<IBinSaver> pSaver = CreateBinSaver( streamHolder.pStream, SAVER_MODE_READ ) )
		{
			pSaver->AddTypedSuper( 1, &rResource );
			return true;
		}
	}
	return false;
}


template<class TResource>
bool SaveXMLResource( const std::string &rszResourceFileName, const std::string &rszExtention, const std::string &rszChunkLabel, TResource &rResource )
{
	std::string szResourceFileName = rszResourceFileName;
	CStringManager::ExtendFileExtention( &szResourceFileName, rszExtention );
	return SaveXMLResource( szResourceFileName, rszChunkLabel, rResource );
}


template<class TResource>
bool SaveXMLResource( const std::string &rszResourceFileName, const std::string &rszChunkLabel, TResource &rResource )
{
	SFileStreamHolder streamHolder;
	CreateStreamHolder( &streamHolder, rszResourceFileName );
	if ( streamHolder.pStream && streamHolder.pStream->IsOk() )
	{
		if ( CPtr<IXmlSaver> pSaver = CreateXmlSaver( streamHolder.pStream, SAVER_MODE_WRITE ) )
		{
			pSaver->Add( rszChunkLabel.c_str(), &rResource );
			return true;
		}
	}
	return false;
}


template<class TResource>
bool SaveTypedSuperXMLResource( const std::string &rszResourceFileName, const std::string &rszExtention, TResource &rResource )
{
	std::string szResourceFileName = rszResourceFileName;
	CStringManager::ExtendFileExtention( &szResourceFileName, rszExtention );
	return SaveTypedSuperXMLResource( szResourceFileName, rResource );
}


template<class TResource>
bool SaveTypedSuperXMLResource( const std::string &rszResourceFileName, TResource &rResource )
{
	SFileStreamHolder streamHolder;
	CreateStreamHolder( &streamHolder, rszResourceFileName );
	if ( streamHolder.pStream && streamHolder.pStream->IsOk() )
	{
		if ( CPtr<IXmlSaver> pSaver = CreateXmlSaver( streamHolder.pStream, SAVER_MODE_WRITE ) )
		{
			pSaver->AddTypedSuper( &rResource );
			return true;
		}
	}
	return false;
}


template<class TResource>
bool SaveBINResource( const std::string &rszResourceFileName, const std::string &rszExtention, int nChunkNumber, TResource &rResource )
{
	std::string szResourceFileName = rszResourceFileName;
	CStringManager::ExtendFileExtention( &szResourceFileName, rszExtention );
	return SaveBINResource( szResourceFileName, nChunkNumber, rResource );
}


template<class TResource>
bool SaveBINResource( const std::string &rszResourceFileName, int nChunkNumber, TResource &rResource )
{
	SFileStreamHolder streamHolder;
	CreateStreamHolder( &streamHolder, rszResourceFileName );
	if ( streamHolder.pStream && streamHolder.pStream->IsOk() )
	{
		if ( CPtr<IBinSaver> pSaver = CreateBinSaver( streamHolder.pStream, SAVER_MODE_WRITE ) )
		{
			pSaver->Add( nChunkNumber, &rResource );
			return true;
		}
	}
	return false;
}


template<class TResource>
bool SaveTypedSuperBINResource( const std::string &rszResourceFileName, const std::string &rszExtention, TResource &rResource )
{
	std::string szResourceFileName = rszResourceFileName;
	CStringManager::ExtendFileExtention( &szResourceFileName, rszExtention );
	return SaveTypedSuperBINResource( szResourceFileName, rResource );
}


template<class TResource>
bool SaveTypedSuperBINResource( const std::string &rszResourceFileName, TResource &rResource )
{
	SFileStreamHolder streamHolder;
	CreateStreamHolder( &streamHolder, rszResourceFileName );
	if ( streamHolder.pStream && streamHolder.pStream->IsOk() )
	{
		if ( CPtr<IBinSaver> pSaver = CreateBinSaver( streamHolder.pStream, SAVER_MODE_WRITE ) )
		{
			pSaver->AddTypedSuper( &rResource );
			return true;
		}
	}
	return false;
}


//bool CheckLatestBINResource( const string &rszResourceFileName, const string &rszXMLExtention, const string &rszBINExtention );


// возвращаемое значение:
// true - путь существует в указанном каталоге
// false - путь не существует в указанном каталоге
//
// параметры:
// pszPath							- начальное значение пути, также сюда записывается результат
// bFile								- ищем файл или каталог, если каталог, то на конец обязательно прибавляется '\\' (эсли необходимо)
// bExists							- вляет на значение возвращаемое функцией ( true / false )
// bReturnAbsolutePath	- в путь добавляется или отрезается указанный путь
// замечания:
// если путь уже был абсолютным и начало пути не совпадает с указанным, то путь не изменяется
// если путь уже был абсолютным и начало пути совпадает с указанным, то путь изменяется на относительный ( при bReturnAbsolutePath == false )
// если путь был относительным, к нему прибавляется указаанный путь ( при bReturnAbsolutePath == true )
bool NormalizePath( std::string *pszPath, bool bFile, bool bExists, bool bReturnAbsolutePath, const std::string &rszPathPrefix, bool *pbAbsolutePath );
bool IsValidFileName( const std::string &rszFileName, bool bAbsolutePath );



struct SEnumFilesInDataStorageParameter
{
	std::list<std::string> fileNameList;
	std::string szPath;
	std::string szExtention;

	int nPathLength;
	int nExtentionLength;
};


typedef std::unordered_map<std::string, std::unordered_map<std::string, UINT> > CEnumFolderMap;
struct SEnumFolderStructureParameter
{
	int nIgnoreFolderCount;
	CEnumFolderMap enumFolderMap;

	SEnumFolderStructureParameter() : nIgnoreFolderCount( 1 ) {}

	bool IsFolderRelative( const std::string &rszFolder, const std::string &rszRelativeFolder );
	void SetRelativeFolder( const std::string &rszFolder, const std::string &rszRelativeFolder );

	static bool IsFolderRelative( const CEnumFolderMap &rEnumFolderMap, const std::string &rszFolder, const std::string &rszRelativeFolder );
	static void SetRelativeFolder( CEnumFolderMap *pEnumFolderMap, const std::string &rszFolder, const std::string &rszRelativeFolder );
};


void EnumFilesInDataStorage( std::vector<SEnumFilesInDataStorageParameter> *pParameters, SEnumFolderStructureParameter *pEnumFolderStructureParameter = 0 );


bool ExecuteProcess( const std::string &rszCommand, const std::string &rszCmdLine, const std::string &rszDirectory, bool bWait );


void Unicode2MBSC( CString *pstrText, const std::wstring &rwszText, int nCodePage );
void MBSC2Unicode( std::wstring *pwszText, const CString &rstrText, int nCodePage );


void File2String( CString *pstrText, bool *pbUnicode, const std::vector<uint8_t> &rBuffer, int nCodePage, bool bRemove_0D );
void File2String( CString *pstrText, bool *pbUnicode, const std::string &rszTextPath, int nCodePage, bool bRemove_0D );
void File2String( std::string *pszText, bool *pbUnicode, const std::string &rszTextPath, int nCodePage, bool bRemove_0D );
void File2String( std::wstring *pwszText, const std::vector<uint8_t> &rBuffer, bool bRemove_0D );
void File2String( std::wstring *pwszText, const std::string &rszTextPath, bool bRemove_0D );


void String2File( std::vector<uint8_t> *pBuffer, const CString &rstrText, bool bUnicode, int nCodePage, bool bAdd_0D );
void String2File( const CString &rstrText, bool bUnicode, const std::string &rszTextPath, int nCodePage, bool bAdd_0D );
void String2File( const std::string &rszText, bool bUnicode, const std::string &rszTextPath, int nCodePage, bool bAdd_0D );
void String2File( std::vector<uint8_t> *pBuffer, const std::wstring &rwszText, bool bAdd_0D );
void String2File( const std::wstring &rwszText, const std::string &rszTextPath, bool bAdd_0D );


