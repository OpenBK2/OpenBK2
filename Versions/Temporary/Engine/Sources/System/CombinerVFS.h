#pragma once

#include "VFS.h"

namespace NVFS
{
class CCombinerVFS : public ICombinerVFS
{
	OBJECT_NOCOPY_METHODS( CCombinerVFS )
	//
	vector< CObj<IVFS> > vfses;
public:
	CCombinerVFS() {}
	CCombinerVFS( IVFS *pVFS ) { if ( pVFS ) vfses.push_back( pVFS ); }
	//
	CDataStream* OpenFile( const string &szPath );
	bool DoesFileExist( const string &szPath );
	bool GetFileStats( SFileStats *pStats, const string &szPath );
	void GetAllFileNames( vector<string> *pFileNames, const string &rszFolder );
	//
	const vector< CObj<IVFS> > &GetVFSList() const { return vfses; }
	void SetVFSList( const vector< CObj<IVFS> > &vfsList );
};

}

