#pragma once

#include "System_export.h"


namespace NLXML
{

struct IXmlSaxVisitor : public CObjectBase
{
	virtual bool VisitHeader( const std::string &szVersion, const std::string &szEncoding, const std::string &szStandalone ) = 0;
	virtual bool VisitComment( const std::string &szText ) = 0;
	virtual bool VisitChunkStart( const std::string &szName ) = 0;
	virtual bool VisitAttribute( const std::string &szName, const std::string &szValue ) = 0;
	virtual bool VisitText( const std::string &szText ) = 0;
	virtual bool VisitChunkFinish( const std::string &szName ) = 0;
};

SYSTEM_EXPORT bool ParseXML( IXmlSaxVisitor *pVisitor, CDataStream *pStream );

}

