#pragma once

#include "System/VFSOperations.h"
// The templates below name CreateXmlSaver and CStringManager in bodies that do
// not depend on the resource type, so their declarations have to be visible
// here. MSVC looked them up only on instantiation, by which point every caller
// happened to have included both; GCC looks them up at the definition.
#include "System/XmlSaver.h"
#include "MapEditorLib/StringManager.h"

#include "MapEditorLib_export.h"

#include <cstdint>

struct SFileStreamHolder
{
	CDataStream *pStream;

	SFileStreamHolder() : pStream( 0 ) { }
	~SFileStreamHolder()
	{
		delete pStream;
	}
};


MAPEDITORLIB_EXPORT void OpenStreamHolder( SFileStreamHolder *pStreamHolder, const std::string &rszTextPath );
MAPEDITORLIB_EXPORT void CreateStreamHolder( SFileStreamHolder *pStreamHolder, const std::string &rszTextPath );


// Each overload that does the work comes before the one that adds a file
// extension and forwards to it. The forwarding call names the resource type, so
// GCC defers it to instantiation, but at that point it only finds an overload
// declared later by argument-dependent lookup, which misses it for any resource
// type outside the global namespace.
//
// LoadBINResource, SaveBINResource and their TypedSuper forms are gone: nothing
// called them, and the TypedSuper ones could never have compiled, since
// IBinSaver has no AddTypedSuper.

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
bool LoadXMLResource( const std::string &rszResourceFileName, const std::string &rszExtention, const std::string &rszChunkLabel, TResource &rResource )
{
	std::string szResourceFileName = rszResourceFileName;
	CStringManager::ExtendFileExtention( &szResourceFileName, rszExtention );
	return LoadXMLResource( szResourceFileName, rszChunkLabel, rResource );
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


// Legacy
template<class TResource>
bool LoadTypedSuperXMLResource( const std::string &rszResourceFileName, const std::string &rszExtention, TResource &rResource )
{
	std::string szResourceFileName = rszResourceFileName;
	CStringManager::ExtendFileExtention( &szResourceFileName, rszExtention );
	return LoadTypedSuperXMLResource( szResourceFileName, rResource );
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
bool SaveXMLResource( const std::string &rszResourceFileName, const std::string &rszExtention, const std::string &rszChunkLabel, TResource &rResource )
{
	std::string szResourceFileName = rszResourceFileName;
	CStringManager::ExtendFileExtention( &szResourceFileName, rszExtention );
	return SaveXMLResource( szResourceFileName, rszChunkLabel, rResource );
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
bool SaveTypedSuperXMLResource( const std::string &rszResourceFileName, const std::string &rszExtention, TResource &rResource )
{
	std::string szResourceFileName = rszResourceFileName;
	CStringManager::ExtendFileExtention( &szResourceFileName, rszExtention );
	return SaveTypedSuperXMLResource( szResourceFileName, rResource );
}


//bool CheckLatestBINResource( const std::string &rszResourceFileName, const std::string &rszXMLExtention, const std::string &rszBINExtention );


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
MAPEDITORLIB_EXPORT bool NormalizePath( std::string *pszPath, bool bFile, bool bExists, bool bReturnAbsolutePath, const std::string &rszPathPrefix, bool *pbAbsolutePath );
MAPEDITORLIB_EXPORT bool IsValidFileName( const std::string &rszFileName, bool bAbsolutePath );



struct SEnumFilesInDataStorageParameter
{
	std::list<std::string> fileNameList;
	std::string szPath;
	std::string szExtention;

	int nPathLength;
	int nExtentionLength;
};


typedef std::unordered_map<std::string, std::unordered_map<std::string, unsigned> > CEnumFolderMap;
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


// The narrow text in these is std::string, UTF-8; the CString overloads went
// with MFC, and the std::string ones that only converted to them are these now.
MAPEDITORLIB_EXPORT void Unicode2MBSC( std::string *pszText, const std::wstring &rwszText, int nCodePage );
// Exported, like the conversion it is the other half of. It was not, which no
// caller outside MapEditorLib had noticed until one wanted to convert the other
// way; nothing about what it does changed.
MAPEDITORLIB_EXPORT void MBSC2Unicode( std::wstring *pwszText, const std::string &rszText, int nCodePage );


MAPEDITORLIB_EXPORT void File2String( std::string *pszText, bool *pbUnicode, const std::vector<uint8_t> &rBuffer, int nCodePage, bool bRemove_0D );
MAPEDITORLIB_EXPORT void File2String( std::string *pszText, bool *pbUnicode, const std::string &rszTextPath, int nCodePage, bool bRemove_0D );
MAPEDITORLIB_EXPORT void File2String( std::wstring *pwszText, const std::vector<uint8_t> &rBuffer, bool bRemove_0D );
MAPEDITORLIB_EXPORT void File2String( std::wstring *pwszText, const std::string &rszTextPath, bool bRemove_0D );


MAPEDITORLIB_EXPORT void String2File( std::vector<uint8_t> *pBuffer, const std::string &rszText, bool bUnicode, int nCodePage, bool bAdd_0D );
MAPEDITORLIB_EXPORT void String2File( const std::string &rszText, bool bUnicode, const std::string &rszTextPath, int nCodePage, bool bAdd_0D );
MAPEDITORLIB_EXPORT void String2File( std::vector<uint8_t> *pBuffer, const std::wstring &rwszText, bool bAdd_0D );
MAPEDITORLIB_EXPORT void String2File( const std::wstring &rwszText, const std::string &rszTextPath, bool bAdd_0D );


