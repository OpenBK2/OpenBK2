#pragma once
#include "System/XMLSAXParser.h"
#include "Misc/StrProc.h"
#include "ObjectHeader.h"

namespace NDb
{

class CObjectHeaderXmlSaxVisitor : public NLXML::IXmlSaxVisitor
{
	OBJECT_BASIC_METHODS( CObjectHeaderXmlSaxVisitor );
	//
	bool bNeedMoreChunks;
	STypeObjectHeader *pHeader;
public:
	CObjectHeaderXmlSaxVisitor(): bNeedMoreChunks( true ), pHeader( 0 ) {}
	CObjectHeaderXmlSaxVisitor( STypeObjectHeader *_pHeader ): bNeedMoreChunks( true ), pHeader( _pHeader ) {}
	//
	bool VisitHeader( const std::string &szVersion, const std::string &szEncoding, const std::string &szStandalone ) { return true; }
	bool VisitComment( const std::string &szText ) { return bNeedMoreChunks; }
	bool VisitAttribute( const std::string &szName, const std::string &szValue ) { return true; }
	bool VisitChunkStart( const std::string &szName )
	{
		if ( bNeedMoreChunks )
			pHeader->szClassTypeName = szName;
		bNeedMoreChunks = false;
		return true;
	}
	bool VisitText( const std::string &szText ) { return bNeedMoreChunks; }
	// finish visiting in the case of header end
	bool VisitChunkFinish( const std::string &szName ) { return bNeedMoreChunks; }
};

}

