#pragma once
#include "Misc/StrProc.h"
#include "ObjectHeader.h"
#include "System/XMLSAXParser.h"

namespace NDb
{

class CObjectHeaderXmlSaxVisitor : public NLXML::IXmlSaxVisitor
{
	OBJECT_BASIC_METHODS( CObjectHeaderXmlSaxVisitor );
	//
	// Constructors still enable parsing and select the supplied destination header.
	bool bNeedMoreChunks = false;
	STypeObjectHeader *pHeader = nullptr;

public:
	CObjectHeaderXmlSaxVisitor()
		: bNeedMoreChunks( true )
		, pHeader( 0 )
	{
	}

	CObjectHeaderXmlSaxVisitor( STypeObjectHeader *_pHeader )
		: bNeedMoreChunks( true )
		, pHeader( _pHeader )
	{
	}

	//
	bool
	VisitHeader( const std::string &szVersion, const std::string &szEncoding, const std::string &szStandalone ) override
	{
		return true;
	}

	bool VisitComment( const std::string &szText ) override
	{
		return bNeedMoreChunks;
	}

	bool VisitAttribute( const std::string &szName, const std::string &szValue ) override
	{
		return true;
	}

	bool VisitChunkStart( const std::string &szName ) override
	{
		if ( bNeedMoreChunks )
		{
			pHeader->szClassTypeName = szName;
		}
		bNeedMoreChunks = false;
		return true;
	}

	bool VisitText( const std::string &szText ) override
	{
		return bNeedMoreChunks;
	}

	// finish visiting in the case of header end
	bool VisitChunkFinish( const std::string &szName ) override
	{
		return bNeedMoreChunks;
	}
};

} // namespace NDb
