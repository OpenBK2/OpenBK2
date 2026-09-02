#include "stdafx.h"
#include <fmt/format.h>
#include "ObjectFilter.h"
#include "Misc/StrProc.h"
#include "System/XmlSaver.h"

static void NormalizeFileName( std::string *pFileName )
{
	NStr::ToLowerASCII( pFileName );
	NStr::ReplaceAllChars( pFileName, '\\', '/' );
}

bool CObjectFilter::SEntry::Match( const std::string &szFullName, const std::string &szClassTypeName ) const
{
	if ( szClassType != szClassTypeName )
		return false;
	if ( matches.empty() )
		return true;
	//
	for ( std::vector<std::string>::const_iterator it = matches.begin(); it != matches.end(); ++it )
	{
		if ( szFullName.compare(0, it->size(), *it) == 0 )
			return true;
	}
	return false;
}

bool CObjectFilter::Match( const std::string &_szFullName, const std::string &szClassTypeName ) const
{
	if ( entries.empty() )
		return false;
	//
	bool bResult = false;
	std::string szFullName = _szFullName;
	NormalizeFileName( &szFullName );
	//
	for ( std::vector<SEntry>::const_iterator it = entries.begin(); it != entries.end(); ++it )
	{
		if ( it->eOpType == SEntry::OPERATION_UNION && it->Match(szFullName, szClassTypeName) != false )
			bResult = true;
		else if ( it->eOpType == SEntry::OPERATION_INTERSECTION && 
			        (bResult == false || it->Match(szFullName, szClassTypeName) == false) )
			return false;
		else if ( it->eOpType == SEntry::OPERATION_DIFFERENCE && 
			        (bResult == false || it->Match(szFullName, szClassTypeName) != false) )
			return false;
	}
	return bResult;
}

int CObjectFilter::SEntry::operator&( IXmlSaver &saver )
{
	if ( saver.IsReading() )
	{
		std::string szOpType;
		saver.Add( "Operation", &szOpType );
		if ( szOpType == "union" )
			eOpType = SEntry::OPERATION_UNION;
		else if ( szOpType == "intersection" )
			eOpType = SEntry::OPERATION_INTERSECTION;
		else if ( szOpType == "difference" )
			eOpType = SEntry::OPERATION_DIFFERENCE;
		else
		{
			NI_ASSERT( false, fmt::format("Unknown operation type \"{}\" (valid are: union, intersection, difference)", szOpType) );
		}
	}
	else
	{
		std::string szOpType = "UNKNOWN";
		switch ( eOpType )
		{
		case SEntry::OPERATION_UNION:
			szOpType = "union";
			break;
		case SEntry::OPERATION_INTERSECTION:
			szOpType = "intersection";
			break;
		case SEntry::OPERATION_DIFFERENCE:
			szOpType = "difference";
			break;
		}
		saver.Add( "Operation", &szOpType );
	}
	saver.Add( "ClassType", &szClassType );
	saver.Add( "Matches", &matches );
	//
	if ( saver.IsReading() )
	{
		for ( std::vector<std::string>::iterator it = matches.begin(); it != matches.end(); ++it )
			NormalizeFileName( &(*it) );
	}
	return 0;
}

int CObjectFilter::operator&( IXmlSaver &saver )
{
	saver.Add( "Name", &wszName );
	saver.Add( "Entries", &entries );
	return 0;
}


