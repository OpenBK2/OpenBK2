#pragma once
#include "ObjectHeader.h"
namespace NDb
{

struct SFullTypeHeader : public STypeObjectHeader
{
	std::string szFileName;
	//
	int operator&( IBinSaver &saver )
	{
		saver.Add( 1, static_cast<STypeObjectHeader*>(this) );
		saver.Add( 2, &szFileName );
		return 0;
	}
};

inline bool LoadIndexData( std::vector<SFullTypeHeader> *pRes, CDataStream *pStream )
{
	if ( pStream && pStream->IsOk() )
	{
		if ( CPtr<IBinSaver> pSaver = CreateBinSaver(pStream, SAVER_MODE_READ) )
			pSaver->Add( 1, pRes );
		else
			return false;

	}
	return true;
}

}

