#include "StdAfx.h"
#include "../System/XMLSAXParser.h"
#include "../System/VFSOperations.h"
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace NTest
{
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct SXmlTestData
{
	enum EType
	{
		UNKNOWN,
		CHUNK_START,
		CHUNK_FINISH,
		ATTRIBUTE,
		TEXT,
		COMMENT
	};
	EType eType;
	string szParam1;
	string szParam2;
};
static const SXmlTestData testData[] = 
{
	{ SXmlTestData::COMMENT, " edited with XMLSPY v2004 rel. 3 U (http://www.xmlspy.com) by [REDACTED] ([REDACTED]) ", "" },
	{ SXmlTestData::CHUNK_START, "Data", "" },
	{ SXmlTestData::ATTRIBUTE, "attr", "wer" },
	{ SXmlTestData::ATTRIBUTE, "zxc", "as \" we" },
	{ SXmlTestData::ATTRIBUTE, "test", "just a test" },
	{ SXmlTestData::CHUNK_START, "as", "" },
	{ SXmlTestData::ATTRIBUTE, "attr1", "val1" },
	{ SXmlTestData::ATTRIBUTE, "attr2", "val2" },
	{ SXmlTestData::CHUNK_FINISH, "as", "" },
	{ SXmlTestData::CHUNK_START, "a", "" },
	{ SXmlTestData::ATTRIBUTE, "as", "we" },
	{ SXmlTestData::ATTRIBUTE, "as1", "we1" },
	{ SXmlTestData::CHUNK_START, "NewText", "" },
	{ SXmlTestData::TEXT, "  this is a text with syschars & < > \" ' ©!", "" },
	{ SXmlTestData::CHUNK_FINISH, "NewText", "" },
	{ SXmlTestData::CHUNK_START, "NewText2", "" },
	{ SXmlTestData::CHUNK_FINISH, "NewText2", "" },
	{ SXmlTestData::CHUNK_FINISH, "a", "" },
	{ SXmlTestData::CHUNK_START, "a", "" },
	{ SXmlTestData::CHUNK_START, "NewText", "" },
	{ SXmlTestData::TEXT, "  this is a text with syschars & < > \" ' ©!", "" },
	{ SXmlTestData::CHUNK_FINISH, "NewText", "" },
	{ SXmlTestData::CHUNK_START, "NewText2", "" },
	{ SXmlTestData::CHUNK_FINISH, "NewText2", "" },
	{ SXmlTestData::CHUNK_FINISH, "a", "" },
	{ SXmlTestData::CHUNK_START, "a", "" },
	{ SXmlTestData::CHUNK_FINISH, "a", "" },
	{ SXmlTestData::COMMENT, "\r\n\t<b as=\"we\" as1=\"we1\">\r\n\t\t<NewText>  this is a text with syschars & < > \" ' !</NewText>\r\n\t\t<NewText2>   </NewText2>\r\n& < > ' \"\r\n\t</b>\r\n    ", "" },
	{ SXmlTestData::CHUNK_FINISH, "Data", "" },
};
// test visitor
class CTestXmlSaxVisitor : public NLXML::IXmlSaxVisitor
{
	OBJECT_BASIC_METHODS( CTestXmlSaxVisitor );
	int nCurrPos;
	bool bTestResult;
public:
	CTestXmlSaxVisitor(): nCurrPos( -1 ), bTestResult( true ) {}
	//
	bool VisitHeader( const string &szVersion, const string &szEncoding, const string &szStandalone )
	{
		if ( nCurrPos != -1 || szVersion != "1.0" || szEncoding != "UTF-8" || szStandalone != "" )
		{
			bTestResult = false;
			DebugTrace( "Header parse failed!" );
			return false;
		}
		++nCurrPos;
		return true;
	}
	bool VisitComment( const string &szText )
	{
		if ( testData[nCurrPos].eType != SXmlTestData::COMMENT )
		{
			bTestResult = false;
			DebugTrace( "Test data is not as expected (comment)" );
			return false;
		}
		if ( szText != testData[nCurrPos].szParam1 )
		{
			DebugTrace( "comment text mismatch!" );
			bTestResult = false;
			return false;
		}
		++nCurrPos;
		return true;
	}
	bool VisitChunkStart( const string &szName )
	{
		if ( testData[nCurrPos].eType != SXmlTestData::CHUNK_START )
		{
			bTestResult = false;
			DebugTrace( "Test data is not as expected (chunk start)" );
			return false;
		}
		if ( szName != testData[nCurrPos].szParam1 )
		{
			DebugTrace( "chunk name mismatch!" );
			bTestResult = false;
			return false;
		}
		++nCurrPos;
		return true;
	}
	bool VisitAttribute( const string &szName, const string &szValue )
	{
		if ( testData[nCurrPos].eType != SXmlTestData::ATTRIBUTE )
		{
			bTestResult = false;
			DebugTrace( "Test data is not as expected (attribute)" );
			return false;
		}
		if ( szName != testData[nCurrPos].szParam1 || szValue != testData[nCurrPos].szParam2 )
		{
			DebugTrace( "attribute mismatch!" );
			bTestResult = false;
			return false;
		}
		++nCurrPos;
		return true;
	}
	bool VisitText( const string &szText )
	{
		if ( testData[nCurrPos].eType != SXmlTestData::TEXT )
		{
			bTestResult = false;
			DebugTrace( "Test data is not as expected (text)" );
			return false;
		}
		if ( szText != testData[nCurrPos].szParam1 )
		{
			DebugTrace( "text mismatch!" );
			bTestResult = false;
			return false;
		}
		++nCurrPos;
		return true;
	}
	bool VisitChunkFinish( const string &szName )
	{
		if ( testData[nCurrPos].eType != SXmlTestData::CHUNK_FINISH )
		{
			bTestResult = false;
			DebugTrace( "Test data is not as expected (chunk finish)" );
			return false;
		}
		if ( szName != testData[nCurrPos].szParam1 )
		{
			DebugTrace( "chunk name mismatch!" );
			bTestResult = false;
			return false;
		}
		++nCurrPos;
		return true;
	}
	bool HasTestPassed() const { return bTestResult; }
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// this is a trace-visitor
class CXmlSaxTraceVisitor : public NLXML::IXmlSaxVisitor
{
	OBJECT_NOCOPY_METHODS( CXmlSaxTraceVisitor );
	vector<string> szChunkNames;
	string GetLeadSpaces() const
	{
		string szBuff;
		szBuff.resize( szChunkNames.size() );
		if ( !szBuff.empty() )
			memset( (void*)szBuff.c_str(), '\t', szChunkNames.size() );
		return szBuff;
	}
public:
	bool VisitHeader( const string &szVersion, const string &szEncoding, const string &szStandalone )
	{
		DebugTrace( "Header: \"%s\", \"%s\", \"%s\"", szVersion.c_str(), szEncoding.c_str(), szStandalone.c_str() );
		return true;
	}
	bool VisitComment( const string &szText )
	{
		DebugTrace( "%sComment: \"%s\"", GetLeadSpaces().c_str(), szText.c_str() );
		return true;
	}
	bool VisitChunkStart( const string &szName )
	{
		DebugTrace( "%sChunk start: \"%s\"", GetLeadSpaces().c_str(), szName.c_str() );
		szChunkNames.push_back( szName );
		return true;
	}
	bool VisitAttribute( const string &szName, const string &szValue )
	{
		DebugTrace( "%sAttribute: %s = \"%s\"", GetLeadSpaces().c_str(), szName.c_str(), szValue.c_str() );
		return true;
	}
	bool VisitText( const string &szText )
	{
		DebugTrace( "%sText: \"%s\"", GetLeadSpaces().c_str(), szText.c_str() );
		return true;
	}
	bool VisitChunkFinish( const string &szName )
	{
		NI_VERIFY( !szChunkNames.empty() && szChunkNames.back() == szName, "Wrong finish chunk!", return false );
		szChunkNames.pop_back();
		DebugTrace( "%sChunk finish: \"%s\"", GetLeadSpaces().c_str(), szName.c_str() );
		return true;
	}
};
class CXmlSaxEmptyVisitor : public NLXML::IXmlSaxVisitor
{
	OBJECT_NOCOPY_METHODS( CXmlSaxEmptyVisitor );
	int nNumHeaders;
	int nNumChunkStarts;
	int nNumChunkFinished;
	int nNumAttributes;
	int nNumComments;
	int nNumTextes;
public:
	CXmlSaxEmptyVisitor()
	{
		nNumHeaders = 0;
		nNumChunkStarts = 0;
		nNumChunkFinished = 0;
		nNumAttributes = 0;
		nNumComments = 0;
		nNumTextes = 0;
	}
	//
	bool VisitHeader( const string &szVersion, const string &szEncoding, const string &szStandalone )
	{
		++nNumHeaders;
		return true;
	}
	bool VisitComment( const string &szText )
	{
		++nNumComments;
		return true;
	}
	bool VisitChunkStart( const string &szName )
	{
		++nNumChunkStarts;
		return true;
	}
	bool VisitAttribute( const string &szName, const string &szValue )
	{
		++nNumAttributes;
		return true;
	}
	bool VisitText( const string &szText )
	{
		++nNumTextes;
		return true;
	}
	bool VisitChunkFinish( const string &szName )
	{
		++nNumChunkFinished;
		return true;
	}
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool TestXmlSaxParser( const char *pszTestFileName )
{
	CFileStream stream( NVFS::GetMainVFS(), pszTestFileName );
	NI_VERIFY( stream.IsOk(), StrFmt("Can't open test file \"%s\" to perform XML SAX parser test!", pszTestFileName), return false );
	CObj<CTestXmlSaxVisitor> pVisitor = new CTestXmlSaxVisitor();
	NLXML::ParseXML( pVisitor, &stream );
	return pVisitor->HasTestPassed();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}
