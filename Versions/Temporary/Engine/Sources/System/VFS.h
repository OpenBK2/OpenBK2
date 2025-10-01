#pragma once

#include "System_export.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "FileTime.h"
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace NVFS
{
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//! file stats descriptor.
struct SFileStats
{
	//! file name
	const char *pszName;
	//! file size
	int nSize;
	//! права доступа (на чтение (1) или на запись (2))
	DWORD dwAccess;
	//! creation time
	SWin32Time ctime;
	//! modification time
	SWin32Time mtime;
	//! last access time
	SWin32Time atime;
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//! Virtual file system.
interface IVFS : public CObjectBase
{
	//! Open file to read data
	virtual CDataStream* OpenFile( const string &szPath ) = 0;
	//! Check does file exist
	virtual bool DoesFileExist( const string &szPath ) = 0;
	//! Retrieve file stats. \return Returns false in the case file doesn't exist
	virtual bool GetFileStats( SFileStats *pStats, const string &szPath ) = 0;
	//! Retrieve all files list from the storage
	virtual void GetAllFileNames( vector<string> *pFileNames, const string &rszFolder ) = 0;
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
interface ICombinerVFS : public IVFS
{
	virtual const vector< CObj<IVFS> > &GetVFSList() const = 0;
	virtual void SetVFSList( const vector< CObj<IVFS> > &vfsList ) = 0;
};
SYSTEM_EXPORT ICombinerVFS *CreateCombinerVFS( IVFS *pVFS );
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
interface IFileCreator : public CObjectBase
{
	//! Open existing file (with truncation) to write or create new one if does not exist
	virtual CDataStream* CreateFile( const string &szPath ) = 0;
	//! Delete existing file
	virtual bool RemoveFile( const string &szPath ) = 0;
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}
