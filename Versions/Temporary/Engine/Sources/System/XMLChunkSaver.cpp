#include "StdAfx.h"

#include "FilePath.h"
#include "XMLChunkSaver.h"
#include "XmlReader.h"
#include "XmlUtils.h"
#include "../Misc/StrProc.h"

typedef IXmlSaver::chunk_id chunk_id;
static const int START_CHUNK_LEVELS = 50;

IXmlSaver *CreateXmlSaver( CDataStream *pStream, ESaverMode mode )
{
	if ( ( mode == SAVER_MODE_READ ) && ( pStream == 0 || pStream->GetSize() == 0 ) )
		return 0;
	if ( mode == SAVER_MODE_WRITE && pStream == 0 ) 
		return 0;
	return new CXMLChunkSaver( pStream, mode == SAVER_MODE_READ );
}

void CXMLChunkSaver::PushReadChunkLevel( const NXml::CXmlNode *pNode )
{
	++nCurChunkLevel;
	if ( readChunkLevels.size() <= nCurChunkLevel )
		readChunkLevels.resize( nCurChunkLevel * 2 );
	
	readChunkLevels[nCurChunkLevel] = pNode;
}

void CXMLChunkSaver::PopReadChunkLevel()
{
	--nCurChunkLevel;
}

void CXMLChunkSaver::Start( CDataStream *pStream, bool bRead )
{
	bReading = bRead;
	pDstStream = 0;
	nCurChunkLevel = -1;
	readChunkLevels.resize( START_CHUNK_LEVELS );

	if ( IsReading() ) 
	{
		const char* pBuffer = (const char*)pStream->GetBuffer();
		pXmlReader = new NXml::CXmlReader( pBuffer, pBuffer + pStream->GetSize() );
		pReadNode = pXmlReader->GetRootElement();
		NI_ASSERT( pReadNode != 0, "Can't find root node" );
		//
		if ( StartChunk( "SharedClasses", 0 ) != false )
		{
			const int nSize = CountChunks();
			if ( nSize != 0 )
			{
				// read types and create empty objects
				for ( int i = 0; i < nSize; ++i )
				{
					if ( StartChunk( "Item", i + 1 ) != false )
					{
						int nClassTypeID = -1;
						void *pServerPtr = 0;
						Add( "__ClassTypeID", &nClassTypeID );
						Add( "__ServerPtr", &pServerPtr );
						CXmlResource *pObject = checked_cast<CXmlResource*>( NObjectFactory::MakeObject( nClassTypeID ) );
						if ( pObject )
							objects[pServerPtr] = pObject;
						//
						FinishChunk();
					}
				}
				// read objects data
				for ( int i = 0; i < nSize; ++i )
				{
					if ( StartChunk( "Item", i + 1 ) != false )
					{
						void *pServerPtr = 0;
						Add( "__ServerPtr", &pServerPtr );
						CObjectsHash::iterator pos = objects.find( pServerPtr );
						if ( pos != objects.end() )
							pos->second->operator&( *this );
						//
						FinishChunk();
					}
				}
			}
			FinishChunk();
		}
	}
	else
	{
		NI_VERIFY( pStream->CanWrite(), "Read only stream passed", return );

		pDstStream = pStream;
		// add XML declaration
		CXMLDeclaration *pDeclaration = new CXMLDeclaration();
		pDeclaration->SetVersion( "1.0" );
		document.AddChild( pDeclaration );
		// add base node
		pCurrNode = new CXMLElement();
		pCurrNode->SetValue( "Base" );
		document.AddChild( pCurrNode );
	}
}

void CXMLChunkSaver::Finish()
{
	if ( IsReading() ) 
	{
	}
	else
	{
		if ( !pDstStream )
			return;
		//
		if ( !toStore.empty() && StartChunk( "SharedClasses", 0 ) != false )
		{
			int nCounter = 1;
			for ( list< CPtr<CXmlResource> >::iterator it = toStore.begin(); it != toStore.end(); ++it, ++nCounter )
			{
				CPtr<CXmlResource> pElement = *it;
				int nClassTypeID = NObjectFactory::GetObjectTypeID( pElement );
				NI_ASSERT( nClassTypeID != -1, StrFmt("Unregistered object of type \"%s\"", typeid(*pElement.GetPtr()).name()) );
				if ( StartChunk("Item", nCounter) != false )
				{
					Add( "__ClassTypeID", &nClassTypeID );
					void *pServerPtr = pElement.GetPtr();
					DataChunk( "__ServerPtr", &pServerPtr, 4, 0 );
					//
					pElement->operator&( *this );
					//
					FinishChunk();
				}
			}
			//
			FinishChunk();
		}
		//
		NI_VERIFY( pDstStream != 0, "", return );
		NLXML_STREAM stream( pDstStream );
		document.Store( stream );
	}
	pDstStream = 0;
}

bool CXMLChunkSaver::StartChunk( const chunk_id idChunk, int nChunkNumber )
{
	if ( IsReading() ) 
	{
		if ( nChunkNumber > 0 )	// elements container
		{
			const vector<const NXml::CXmlNode*> &nodes = pReadNode->GetNodes();
			if ( nChunkNumber - 1 < nodes.size() )
			{
				const NXml::CXmlNode *pNode = nodes[nChunkNumber - 1];
				NI_ASSERT( (pNode->GetName() == "Item") || (pNode->GetName() == "item"), StrFmt("Wrong %dth item of container (%s)", nChunkNumber, pNode->GetName().ToString().c_str()) );
				PushReadChunkLevel( pReadNode );
				pReadNode = pNode;
				return true;
			}
		}
		else if ( idChunk != 0 )	// try single element by name
		{
			if ( const NXml::CXmlNode *pNode = pReadNode->FindChild( idChunk ) )
			{
				PushReadChunkLevel( pReadNode );
				pReadNode = pNode;
				return true;
			}
		}
		else	// use this node
		{
			PushReadChunkLevel( pReadNode );
			return true;
		}
	}
	else
	{
		chunkLevels.push_back( pCurrNode );
		//
		CXMLMultiNode *pNode = 0;
		if ( nChunkNumber > 0 )	// elements container
		{
			pNode = new CXMLElement();
			pNode->SetValue( "Item" );
		}
		else if ( idChunk != 0 )	// single element
		{
			pNode = new CXMLElement();
			pNode->SetValue( idChunk );
		}
		else	// use this node
			pNode = pCurrNode;
		//
		if ( pCurrNode != pNode )
			pCurrNode->AddChild( pNode );

		pCurrNode = pNode;
		return true;
	}
	return false;
}

void CXMLChunkSaver::FinishChunk()
{
	if ( IsReading() )
	{
		if ( nCurChunkLevel != -1 ) 
		{
			pReadNode = readChunkLevels[nCurChunkLevel];
			PopReadChunkLevel();
		}
		else
			pReadNode = 0;
	}
	else
	{
		if ( !chunkLevels.empty() ) 
		{
			pCurrNode = chunkLevels.back();
			chunkLevels.pop_back();
		}
		else
			pCurrNode = 0;
	}
}

int CXMLChunkSaver::CountChunks()
{
	if ( IsReading() )
		return pReadNode != 0 ? pReadNode->GetNodes().size() : 0;
	else
		return pCurrNode != 0 ? pCurrNode->CountChildren() : 0;
}

static const int ReadInt( const char *p, const int nSize )
{
/*	
	int n = 0;
	_snscanf(  p, nSize, "%i", &n );
	return n;
*/
	const char *pEnd = p + nSize;
	int n = 0;
	int nSign = 1;

	if ( nSize >= 2 && *p == '0' && *(p+1) == 'x' )
	{
		p += 2;
		while ( p < pEnd )
		{
			int nDst = 0;
			if ( *p >= '0' && *p <= '9' )
				nDst = *p - '0';
			else if ( *p >= 'a' && *p <= 'f' )
				nDst = *p - 'a' + 10;
			else if ( *p >= 'A' && *p <= 'F' )
				nDst = *p - 'A' + 10;
			else break;

			n = n * 16 + nDst;
			++p;
		}
	}
	else
	{
		if ( *p == '-' )
		{
			nSign = -1;
			++p;
		}

		while ( p < pEnd && *p >= '0' && *p <= '9' )
		{
			n = n*10 + *p - '0';
			++p;
		}
	}

	return n * nSign;
}

static const float ReadFloat( const char *p, const int nSize )
{
	const char *pEnd = p + nSize;
	float f = 0.0f;
	int nSign = 1;
	if ( *p == '-' )
	{
		nSign = -1;
		++p;
	}

	while ( p < pEnd && *p >= '0' && *p <= '9' )
	{
		f = f * 10.0f + *p - '0';
		++p;
	}

	if ( p == pEnd )
		return f * nSign;

	if ( *p == '.' )
	{
		++p;
		float fPower = 1.0f;
		while ( p < pEnd && *p >= '0' && *p <= '9' )
		{
			fPower /= 10.0f;
			f += (*p - '0') * fPower;
			++p;
		}
	}

	if ( *p == 'e' || *p == 'E' )
	{
		++p;
		if ( p < pEnd )
		{
			int nESign = 1;
			if ( *p == '-' )
			{
				nESign = -1;
				++p;
			}

			int nPower = 0;
			while ( p < pEnd && *p >= '0' && *p <= '9' )
			{
				nPower = nPower * 10.0f + *p - '0';
				++p;
			}
			nPower *= nESign;

			f *= pow( 10.0f, nPower );
		}
	}

	return f * nSign;
}

bool CXMLChunkSaver::DataChunk( const chunk_id idChunk, int *pnData, int nChunkNumber )
{
	if ( IsReading() ) 
	{
		if ( const NXml::CXmlNode *pNode = pReadNode->FindChild( idChunk ) )
		{
			*pnData = ReadInt( pNode->GetValue().pszBegin, pNode->GetValue().nSize );
			return true;
		}
		else
		{
			if ( (strcmp(idChunk, "Item") == 0) || (strcmp(idChunk, "item") == 0) ) 
			{
				if ( const NXml::CXmlNode *pNode = pReadNode->FindChild("item") )
				{
					const NXml::SXmlAttribute *pAttribute = pNode->GetDataAttribute();
					int nCheck = 0;
					if ( pAttribute && pAttribute->value.nSize )
						*pnData = ReadInt( pAttribute->value.pszBegin, pAttribute->value.nSize );
					return true;
				}
			}
			else
			{
				const NXml::SXmlAttribute *pAttribute = pReadNode->GetAttribute( idChunk );
				int nCheck = 0;
				if ( pAttribute && pAttribute->value.nSize ) 
					*pnData = ReadInt( pAttribute->value.pszBegin, pAttribute->value.nSize );
				return true;
			}
		}
	}
	else if ( pCurrNode ) 
	{
		CXMLElement *pElement = new CXMLElement();
		pElement->SetValue( idChunk );
		pElement->SetText( StrFmt("%d", *pnData) );
		pCurrNode->AddChild( pElement );
		return true;
	}
	return false;
}

bool CXMLChunkSaver::DataChunk( const chunk_id idChunk, float *pfData, int nChunkNumber )
{
	if ( IsReading() ) 
	{
		if ( const NXml::CXmlNode *pNode = pReadNode->FindChild( idChunk ) )
		{
			*pfData = ReadFloat( pNode->GetValue().pszBegin, pNode->GetValue().nSize );
			return true;
		}
		else
		{
			if ( (strcmp(idChunk, "Item") == 0) || (strcmp(idChunk, "item") == 0) ) 
			{
				if ( const NXml::CXmlNode *pNode = pReadNode->FindChild("item") )
				{
					const NXml::SXmlAttribute *pAttribute = pNode->GetDataAttribute();
					if ( pAttribute && pAttribute->value.nSize )
					{
						*pfData = ReadFloat( pAttribute->value.pszBegin, pAttribute->value.nSize );
						return true;
					}
					return false;
				}
			}
			else
			{
				const NXml::SXmlAttribute *pAttribute = pReadNode->GetAttribute( idChunk );
				if ( pAttribute && pAttribute->value.nSize )
				{
					*pfData = ReadFloat( pAttribute->value.pszBegin, pAttribute->value.nSize );
					return true;
				}
				return false;
			}
		}
	}
	else if ( pCurrNode ) 
	{
		CXMLElement *pElement = new CXMLElement();
		pElement->SetValue( idChunk );
		pElement->SetText( StrFmt("%g", *pfData) );
		pCurrNode->AddChild( pElement );
		return true;
	}
	return false;
}

static bool String2Bool( const char *pszStr )
{
	switch ( *pszStr )
	{
	case 'T':
	case 't':
	case '1':
		return true;

	case 'F':
	case 'f':
	case '0':
		return false;
	}
	NI_ASSERT( false, StrFmt("Can't convert value to bool" ) );
	return false;
}

bool CXMLChunkSaver::DataChunk( const chunk_id idChunk, bool *pData, int nChunkNumber )
{
	if ( IsReading() ) 
	{
		if ( const NXml::CXmlNode *pNode = pReadNode->FindChild(idChunk) )
		{
			if ( pNode->GetValue().nSize )
			{
				*pData = String2Bool( pNode->GetValue().pszBegin );
				return true;
			}
			return false;
		}
		else
		{
			if ( strcmp(idChunk, "Item") == 0 ) 
			{
				if ( const NXml::CXmlNode *pNode = pReadNode->FindChild("item") )
				{
					const NXml::SXmlAttribute *pAttribute = pNode->GetDataAttribute();
					if ( pAttribute && pAttribute->value.nSize ) 
					{
						*pData = String2Bool( pAttribute->value.pszBegin );
						return true;
					}
					else
						return false;
				}
			}
			else
			{
				const NXml::SXmlAttribute *pAttribute = pReadNode->GetAttribute( idChunk );
				if ( pAttribute && pAttribute->value.nSize ) 
				{
					*pData = String2Bool( pAttribute->value.pszBegin );
					return true;
				}
				else
					return false;
			}
		}
	}
	else if ( pCurrNode ) 
	{
		CXMLElement *pElement = new CXMLElement();
		pElement->SetValue( idChunk );
		const char *pszText = ( *pData == false ? "false" : "true" );
		pElement->SetText( pszText );
		pCurrNode->AddChild( pElement );
		return true;
	}
	return false;
}

bool CXMLChunkSaver::DataChunk( const chunk_id idChunk, void *pData, int nSize, int nChunkNumber )
{
	if ( idChunk != 0 && StartChunk(idChunk, nChunkNumber) == false )
		return false;
	NI_ASSERT( nSize > 0, "Wrong size passed" );
	string szString;
	if ( IsReading() ) 
	{
		DataChunkString( szString );
		if ( !szString.empty() )
		{
			NI_ASSERT( szString.size() == nSize * 2, "Wrong bin chunk size" );
			NStr::StringToBin( szString.c_str(), pData, 0 );
		}
		else
			memset( pData, 0, nSize );
	}
	else
	{
		szString.resize( nSize * 2 );
		NStr::BinToString( pData, nSize, &(szString[0]) );
		DataChunkString( szString );
	}
	//
	if ( idChunk != 0 )
		FinishChunk();
	return true;
}

bool CXMLChunkSaver::DataChunk( const chunk_id idChunk, GUID *pgData, int nChunkNumber )
{
	if ( idChunk != 0 && StartChunk(idChunk, nChunkNumber) == false )
		return false;
	//
	bool bRetVal = false;
	string szValue;
	if ( IsReading() )
	{
		if ( DataChunkString(szValue) )
		{
			NStr::String2GUID( szValue, pgData );
			bRetVal = true;
		}
	}
	else
	{
		NStr::GUID2String( &szValue, *pgData );
		bRetVal = DataChunkString( szValue );
	}
	FinishChunk();
	return bRetVal;
}

bool CXMLChunkSaver::DataChunkDBID( CDBID *pDBID )
{
	if ( IsReading() )
	{
		if ( pReadNode )
		{
			if ( const NXml::SXmlAttribute *pAttrib = pReadNode->GetHRefAttribute() )
			{
				const string szRef = pAttrib->value.ToString();
				if ( !szRef.empty() )
				{
					string szDBID = szRef.substr( 0, szRef.rfind('#') );
					if ( !szDBID.empty() )
					{
						NStr::UTF8ToMBCS( &szDBID, szDBID );
						if ( szDBID[0] == '/' || szDBID[0] == '\\' )
							*pDBID = szDBID.c_str() + 1;
						else
							*pDBID = GetCurrObjectPath() + szDBID;
					}
				}
				else
					*pDBID = "";
				return true;
			}
		}
	}
	else
	{
		if ( pCurrNode )
		{
			checked_cast<CXMLElement*>(pCurrNode)->SetAttribute( HREF_ATTRIBUTE_NAME, "/" + NDb::GetFileName(*pDBID) );
			return true;
		}
	}

	return true;
}

bool CXMLChunkSaver::DataChunkFilePath( NFile::CFilePath *pFilePath )
{
	if ( IsReading() )
	{
		if ( pReadNode )
		{
			string szFilePath;
			if ( const NXml::SXmlAttribute *pAttrib = pReadNode->GetHRefAttribute() )
			{
				const string szRefValue = pAttrib->value.ToString();
				if ( !szRefValue.empty() )
				{
					if ( szRefValue[0] == '/' || szRefValue[0] == '\\' )
						szFilePath = szRefValue.c_str() + 1;
					else
						szFilePath = GetCurrObjectPath() + szRefValue;
				}
				else
					szFilePath = "";
			}
			else
				szFilePath = pReadNode->GetValue().ToString();
			//
			if ( szFilePath.empty() )
				*pFilePath = "";
			else
				NStr::UTF8ToMBCS( pFilePath, szFilePath );
			//
			return true;
		}
	}
	else
	{
		if ( pCurrNode )
		{
			string szFilePath;
			NStr::MBCSToUTF8( &szFilePath, *pFilePath );
			checked_cast<CXMLElement*>(pCurrNode)->SetAttribute( HREF_ATTRIBUTE_NAME, "/" + szFilePath );
			return true;
		}
	}
	return true;
}

bool CXMLChunkSaver::DataChunkString( string &data )
{
	if ( IsReading() ) 
	{
		if ( pReadNode )
		{
			data = pReadNode->GetValue().ToString();
			NXml::ConvertToString( &data );

			return true;
		}
	}
	else if ( pCurrNode ) 
	{
		checked_cast<CXMLElement*>(pCurrNode)->SetText( data );
		return true;
	}
	return false;
}

bool CXMLChunkSaver::DataChunkString( wstring &data )
{
	if ( IsReading() ) 
	{
		if ( pReadNode )
		{
			string szData = pReadNode->GetValue().ToString();
			NXml::ConvertToString( &szData );
			NStr::UTF8ToUnicode( &data, szData );
			return true;
		}
	}
	else if ( pCurrNode ) 
	{
		string szString;
		NStr::UnicodeToUTF8( &szString, data );
		checked_cast<CXMLElement*>(pCurrNode)->SetText( szString );
		return true;
	}
	return false;
}

void CXMLChunkSaver::StoreObject( CObjectBase *pObject )
{
	if ( pObject )
	{
		NI_ASSERT( NObjectFactory::GetObjectTypeID( pObject ) != -1, StrFmt( "trying to save unregistered object \"%s\"", typeid(*pObject).name() ) );
	}	

	if ( pObject != 0 && storedObjects.find( pObject ) == storedObjects.end() )
	{
		toStore.push_back( checked_cast<CXmlResource*>(pObject) );
		storedObjects[pObject] = true; // важно присвоить хоть что-нибудь
	}
	void *pServerPtr = pObject;
	DataChunk( 0, &pServerPtr, 4, 0 );
}

void CXMLChunkSaver::ReportCurrentObject( const CDBID &dbid ) 
{ 
	string szFileName = NDb::GetFileName( dbid ); 
	NStr::ReplaceAllChars( &szFileName, '\\', '/' );
	const string szObjectPath = szFileName.substr( 0, szFileName.rfind('/') + 1 ); 
	objectNamesStack.clear();
	objectNamesStack.push_back( szObjectPath );
	szCurrObjectPath = szObjectPath;
}

void CXMLChunkSaver::PushCurrentObject( const CDBID &dbid )
{
	string szFileName = NDb::GetFileName( dbid ); 
	NStr::ReplaceAllChars( &szFileName, '\\', '/' );
	const string szObjectPath = szFileName.substr( 0, szFileName.rfind('/') + 1 ); 
	objectNamesStack.push_back( szObjectPath );
	szCurrObjectPath = szObjectPath;
}

void CXMLChunkSaver::PopCurrentObject() 
{ 
	NI_VERIFY( !objectNamesStack.empty(), "Poping from empty stack!", return ); 
	objectNamesStack.pop_back(); 
	szCurrObjectPath = objectNamesStack.empty() ? "" : objectNamesStack.back();
}

CObjectBase* CXMLChunkSaver::LoadObject()
{
	void *pServerPtr = 0;
	DataChunk( 0, &pServerPtr, 4, 0 );
	if ( pServerPtr != 0 )
	{
		CObjectsHash::iterator pFound = objects.find( pServerPtr );
		if ( pFound != objects.end() )
			return pFound->second;
		NI_ASSERT( false, "Here we are in trouble - stored object does not exist" );
		// here we are in problem - stored object does not exist
		// actually i think we got to throw the exception
	}
	return 0;
}

bool CXMLChunkSaver::AddAttribute( const chunk_id attrName, bool *pData )
{
	if ( IsReading() )
	{
		if ( pReadNode )
		{
			if ( const NXml::SXmlAttribute *pAttribute = pReadNode->GetAttribute(attrName) )
			{
				*pData = String2Bool( pAttribute->value.pszBegin );
				return true;
			}
		}
	}
	else
	{
		if ( pCurrNode )
		{
			checked_cast<CXMLElement*>(pCurrNode)->SetAttribute( attrName, (*pData == false ? "false" : "true") );
			return true;
		}
	}
	return false;
}

bool CXMLChunkSaver::AddAttribute( const chunk_id attrName, int *pData )
{
	if ( IsReading() )
	{
		if ( pReadNode )
		{
			if ( const NXml::SXmlAttribute *pAttribute = pReadNode->GetAttribute(attrName) )
			{
				*pData = ReadInt( pAttribute->value.pszBegin, pAttribute->value.nSize );
				return true;
			}
		}
	}
	else
	{
		if ( pCurrNode )
		{
			checked_cast<CXMLElement*>(pCurrNode)->SetAttribute( attrName, StrFmt("%d", *pData) );
			return true;
		}
	}
	return false;
}

bool CXMLChunkSaver::AddAttribute( const chunk_id attrName, float *pData )
{
	if ( IsReading() )
	{
		if ( pReadNode )
		{
			if ( const NXml::SXmlAttribute *pAttribute = pReadNode->GetAttribute(attrName) )
			{
				*pData = ReadFloat( pAttribute->value.pszBegin, pAttribute->value.nSize );
				return true;
			}
		}
	}
	else
	{
		if ( pCurrNode )
		{
			checked_cast<CXMLElement*>(pCurrNode)->SetAttribute( attrName, StrFmt("%g", *pData) );
			return true;
		}
	}
	return false;
}

bool CXMLChunkSaver::AddAttribute( const chunk_id attrName, string *pData )
{
	if ( IsReading() )
	{
		if ( pReadNode )
		{
			if ( strcmp(attrName, HREF_ATTRIBUTE_NAME) == 0 )
			{
				if ( const NXml::SXmlAttribute *pAttribute = pReadNode->GetHRefAttribute() )
				{
					string szHRef = pAttribute->value.ToString();
					if ( !szHRef.empty() )
					{
						NStr::UTF8ToMBCS( &szHRef, szHRef );
						if ( szHRef[0] == '/' || szHRef[0] == '\\' )
							*pData = szHRef.c_str() + 1;
						else
							*pData = GetCurrObjectPath() + szHRef;
					}
					else
						pData->clear();
					return true;
				}
			}
			else if ( const NXml::SXmlAttribute *pAttribute = pReadNode->GetAttribute( attrName ) )
			{
				NStr::UTF8ToMBCS( pData, pAttribute->value.ToString() );
				return true;
			}
		}
	}
	else
	{
		if ( pCurrNode )
		{
			string szVal;
			NStr::MBCSToUTF8( &szVal, *pData );
			if ( strcmp(attrName, HREF_ATTRIBUTE_NAME) == 0 )
				checked_cast<CXMLElement*>(pCurrNode)->SetAttribute( attrName, "/" + szVal );
			else
				checked_cast<CXMLElement*>(pCurrNode)->SetAttribute( attrName, szVal );
			return true;
		}
	}
	return false;
}

bool CXMLChunkSaver::AddAttribute( const chunk_id attrName, wstring *pData )
{
	if ( IsReading() )
	{
		if ( pReadNode )
		{
			if ( const NXml::SXmlAttribute *pAttribute = pReadNode->GetAttribute(attrName) )
			{
				NStr::UTF8ToUnicode( pData, pAttribute->value.ToString() );
				return true;
			}
		}
	}
	else
	{
		if ( pCurrNode )
		{
			string szRes;
			NStr::UnicodeToUTF8( &szRes, *pData );
			checked_cast<CXMLElement*>(pCurrNode)->SetAttribute( attrName, szRes );
		}
	}
	return false;
}


