#pragma once

#include "System_export.h"

#include "FileTime.h"

#include <cstdint>

namespace NVFS
{

//! file stats descriptor.
struct SFileStats
{
	//! file name
	const char *pszName;
	//! file size
	int nSize;
	//! права доступа (на чтение (1) или на запись (2))
	uint32_t dwAccess;
	//! modification time, packed as in FileTime.h so that a file on disk and
	//! an entry in an archive can be compared directly
	SWin32Time mtime;
};

//! Virtual file system.
struct IVFS : public CObjectBase
{
	//! Open file to read data
	virtual CDataStream* OpenFile( const std::string &szPath ) = 0;
	//! Check does file exist
	virtual bool DoesFileExist( const std::string &szPath ) = 0;
	//! Retrieve file stats. \return Returns false in the case file doesn't exist
	virtual bool GetFileStats( SFileStats *pStats, const std::string &szPath ) = 0;
	//! Retrieve all files list from the storage
	virtual void GetAllFileNames( std::vector<std::string> *pFileNames, const std::string &rszFolder ) = 0;
};

struct ICombinerVFS : public IVFS
{
	virtual const std::vector< CObj<IVFS> > &GetVFSList() const = 0;
	virtual void SetVFSList( const std::vector< CObj<IVFS> > &vfsList ) = 0;
};
SYSTEM_EXPORT ICombinerVFS *CreateCombinerVFS( IVFS *pVFS );

struct IFileCreator : public CObjectBase
{
	//! Open existing file (with truncation) to write or create new one if does not exist
	virtual CDataStream* CreateFile( const std::string &szPath ) = 0;
	//! Delete existing file
	virtual bool RemoveFile( const std::string &szPath ) = 0;
};

}

