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
	bool VisitHeader( const string &szVersion, const string &szEncoding, const string &szStandalone ) { return true; }
	bool VisitComment( const string &szText ) { return bNeedMoreChunks; }
	bool VisitAttribute( const string &szName, const string &szValue ) { return true; }
	bool VisitChunkStart( const string &szName )
	{
		if ( bNeedMoreChunks )
			pHeader->szClassTypeName = szName;
		bNeedMoreChunks = false;
		return true;
	}
	bool VisitText( const string &szText ) { return bNeedMoreChunks; }
	// finish visiting in the case of header end
	bool VisitChunkFinish( const string &szName ) { return bNeedMoreChunks; }
};

}

