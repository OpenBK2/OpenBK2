#include "stdafx.h"

#include "CombinerVFS.h"
#include "FilePath.h"

#include <algorithm>

namespace NVFS
{

CDataStream *CCombinerVFS::OpenFile( const std::string &szPath )
{
	for ( int i = vfses.size() - 1; i >= 0; --i )
	{
		if ( CDataStream *pFile = vfses[i]->OpenFile(szPath) )
			return pFile;
	}
	return 0;
}

bool CCombinerVFS::DoesFileExist( const std::string &szPath )
{
	for ( int i = vfses.size() - 1; i >= 0; --i )
	{
		if ( vfses[i]->DoesFileExist(szPath) )
			return true;
	}
	return false;
}

bool CCombinerVFS::GetFileStats( SFileStats *pStats, const std::string &szPath )
{
	for ( int i = vfses.size() - 1; i >= 0; --i )
	{
		if ( vfses[i]->GetFileStats(pStats, szPath) )
			return true;
	}
	return false;
}

void CCombinerVFS::GetAllFileNames( std::vector<std::string> *pFileNames, const std::string &rszFolder )
{
	// compose filenames from all VFSes
	typedef std::unordered_map<NFile::CFilePath, int> CHashSet;
	CHashSet hashset;
	for ( int i = vfses.size() - 1; i >= 0; --i )
	{
		std::vector<std::string> fileNames;
		vfses[i]->GetAllFileNames( &fileNames, rszFolder );
		for ( std::vector<std::string>::const_iterator it = fileNames.begin(); it != fileNames.end(); ++it )
			hashset[*it] = 1;
	}
	// sort before return
	pFileNames->clear();
	pFileNames->reserve( hashset.size() );
	for ( CHashSet::const_iterator it = hashset.begin(); it != hashset.end(); ++it )
		pFileNames->push_back( it->first );
	std::sort( pFileNames->begin(), pFileNames->end() );
}

void CCombinerVFS::SetVFSList( const std::vector< CObj<IVFS> > &vfsList )
{ 
	vfses.clear(); 
	vfses = vfsList; 
}

ICombinerVFS *CreateCombinerVFS( IVFS *pVFS )
{
	return new CCombinerVFS( pVFS );
}

}


