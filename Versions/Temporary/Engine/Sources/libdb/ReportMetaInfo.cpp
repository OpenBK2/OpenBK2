#include "stdafx.h"
#include "StructMetaInfo.h"
#include "ReportMetaInfo.h"
#include "System/XmlSaver.h"

namespace NDb
{
namespace NMetaInfo
{

// ************************************************************************************************************************ //
// **
// ** automagic
// **
// **
// **
// ************************************************************************************************************************ //

struct SMetaInfo
{
	list< CPtr<SStructMetaInfo> > structsStack;
	CMetaInfoMap knownTypes;
};
static SMetaInfo *s_pMetaInfo = 0;
static struct SMetaInfoAutoMagic
{
	~SMetaInfoAutoMagic()
	{
		if ( s_pMetaInfo )
			delete s_pMetaInfo;
		s_pMetaInfo = 0;
	}
} aMetaInfoAutoMagic;

bool CreateFullMetaInfoCopy( CMetaInfoMap *pRes )
{
	if ( s_pMetaInfo == 0 || s_pMetaInfo->knownTypes.empty() )
	{
		pRes->clear();
		return true;
	}
	//
	for ( CMetaInfoMap::const_iterator it = s_pMetaInfo->knownTypes.begin(); it != s_pMetaInfo->knownTypes.end(); ++it )
	{
		SStructMetaInfo *pCopy = new SStructMetaInfo();
		it->second->MakeDeepCopy( pCopy );
		(*pRes)[it->first] = pCopy;
	}
	//
	return pRes->size() == s_pMetaInfo->knownTypes.size();
}

void DropMetaInfo()
{
	if ( s_pMetaInfo )
		delete s_pMetaInfo;
	s_pMetaInfo = 0;
}

void StartRegister()
{
	if ( s_pMetaInfo == 0 )
		s_pMetaInfo = new SMetaInfo;
}

// ************************************************************************************************************************ //
// **
// ** meta-info reporting structs & functions
// **
// **
// **
// ************************************************************************************************************************ //

void StartMetaInfoReport( const string &szTypeName, const int nTypeID, const int nStructSize )
{
	StartRegister();
	if ( s_pMetaInfo->structsStack.empty() )
	{
		SStructMetaInfo *pInfo = new SStructMetaInfo();
		pInfo->nStructSize = nStructSize;
		s_pMetaInfo->knownTypes[szTypeName] = pInfo;
		s_pMetaInfo->structsStack.push_back( pInfo );
	}
	//	DebugTrace( "Type \"%s\" report started", szTypeName.c_str() );
}

void AddOnStack( SStructMetaInfo *pInfo )
{
	StartRegister();
	s_pMetaInfo->structsStack.push_back( pInfo );
}

void FinishMetaInfoReport()
{
	s_pMetaInfo->structsStack.pop_back();
	//	DebugTrace( "Report finished" );
}

void ReportMetaInfo( const string &szName, int nPtrShift, int nSizeof, NTypeDef::ETypeType eType )
{
	s_pMetaInfo->structsStack.back()->AddField( szName, nPtrShift, nSizeof, eType );
	//	DebugTrace( "\tField \"%s\" of type %d with shift %d and size %d", szName.c_str(), eType, nPtrShift, nSizeof );
}

void ReportMetaInfo( const string &szName, int nPtrShift, int nSizeof, NTypeDef::ETypeType eType, 
										int nContainedSize, NTypeDef::ETypeType eContainedType )
{
	//	DebugTrace( "\tArray-Field \"%s\" of type %d (contained type %d of size %d) with shift %d and size %d", 
	//		          szName.c_str(), eType, eContainedType, nContainedSize, nPtrShift, nSizeof );
	SStructMetaInfo *pStruct = s_pMetaInfo->structsStack.back();
	pStruct->AddField( szName, nPtrShift, nSizeof, eType, nContainedSize, eContainedType );
}

// ************************************************************************************************************************ //
// **
// ** 
// **
// **
// **
// ************************************************************************************************************************ //

STerminalClassReporter::STerminalClassReporter( CResource *pRes, IXmlSaver &_saver )
	: saver( _saver )
{
	saver.PushCurrentObject( pRes->GetDBID() );
}

STerminalClassReporter::~STerminalClassReporter()
{
	saver.PopCurrentObject();
}

}
}
