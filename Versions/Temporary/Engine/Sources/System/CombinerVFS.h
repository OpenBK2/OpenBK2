#pragma once

#include "VFS.h"

namespace NVFS
{
class CCombinerVFS : public ICombinerVFS
{
	OBJECT_NOCOPY_METHODS( CCombinerVFS )
	//
	std::vector<CObj<IVFS>> vfses;

public:
	CCombinerVFS() {}

	CCombinerVFS( IVFS *pVFS )
	{
		if ( pVFS )
		{
			vfses.push_back( pVFS );
		}
	}

	//
	CDataStream *OpenFile( const std::string &szPath ) override;
	bool DoesFileExist( const std::string &szPath ) override;
	bool GetFileStats( SFileStats *pStats, const std::string &szPath ) override;
	void GetAllFileNames( std::vector<std::string> *pFileNames, const std::string &rszFolder ) override;

	//
	const std::vector<CObj<IVFS>> &GetVFSList() const override
	{
		return vfses;
	}

	void SetVFSList( const std::vector<CObj<IVFS>> &vfsList ) override;
};

} // namespace NVFS
