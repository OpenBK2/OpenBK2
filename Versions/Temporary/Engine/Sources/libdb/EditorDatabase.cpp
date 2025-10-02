#include "StdAfx.h"

#include "Db.h"
#include "StructMetaInfo.h"
#include "Bind.h"
#include "Database.h"
#include "Index.h"
#include "ReportMetaInfo.h"
#include "../System/LightXML.h"
#include "../System/XMLReader.h"
#include "../System/VFS.h"
#include "../System/xmlreader.h"
#include "Logger.h"
#include "DBWatcherClient.h"

EXTERNVAR CLogger theLogger;

namespace NDb
{

struct SEditorDbForceLoadGuard
{
	static int s_nForceLoadCounter;
	static bool s_bForceLoad;
	bool bOpened;
	SEditorDbForceLoadGuard()
	{
		NI_ASSERT( s_nForceLoadCounter >= 0, "Invalid force load counter!" );
//		s_bForceLoad = true;
		s_bForceLoad = s_nForceLoadCounter == 0;
		++s_nForceLoadCounter;
		bOpened = true;
	}
	~SEditorDbForceLoadGuard()
	{
		Close();
		NI_ASSERT( s_nForceLoadCounter >= 0, "Invalid force load counter!" );
	}
	// special for PostLoad call
	void Close()
	{
		if ( bOpened )
			--s_nForceLoadCounter;
		bOpened = false;
	}
	//
	static bool IsLoading() { return s_nForceLoadCounter > 0; }
	static bool CanLoad() { return s_bForceLoad; }
};
int SEditorDbForceLoadGuard::s_nForceLoadCounter = 0;
bool SEditorDbForceLoadGuard::s_bForceLoad = true;

class CEditorDatabase : public CBasicDatabase
{
	OBJECT_NOCOPY_METHODS( CEditorDatabase );
	//
	struct SElement
	{
		STypeObjectHeader typeHeader;
		CObj<NBind::CBindStruct> pBind;
	};
	typedef hash_map<CDBID, SElement> CElementsMap;
	CElementsMap elementsMap;							// objects map
	bool bIndexChanged;										// index was changed during elements manipulation
	typedef hash_map<string, CObj<NTypeDef::STypeDef> > CTypesMap;
	CTypesMap typesMap;										// type definitions
	NMetaInfo::CMetaInfoMap metaInfoMap;
	//
	IObjMan *GetObjManInternal( const CDBID &dbid );
	bool LoadObject( NBind::CBindStruct *pBind, const CDBID &dbid );
	bool SaveObject( NBind::CBindStruct *pBind, const CDBID &dbid );
	bool ReallyRegisterResourceFile( const CDBID &dbid );
	//
	const SElement *GetElement( const CDBID &_dbid );
	int GetClassTypeID( const string &szClassTypeName );
	NBind::CBindStruct *CreateNewBind( const STypeObjectHeader &header );
	//! get raw struct meta info w/o linking with type defs
	NMetaInfo::SStructMetaInfo *GetRawStructMetaInfo( const string &szTypeName );
	//! get struct meta info, linked with type defs (link if necessary)
	NMetaInfo::SStructMetaInfo *GetStructMetaInfo( const string &szTypeName );
	//! check object exist (register it from storage if necessary)
	bool DoesObjectExist( const CDBID &dbid );
	//
	void RemoveObjectInternal( const CDBID &dbid );
	void AddNewObjectInternal( const CDBID &_dbid, IObjMan *pObjMan );
	//
	bool LoadTypesMap();
	//
	void SetIndexChanged() { bIndexChanged = true; }
	void ResetIndexChanged() { bIndexChanged = false; }
	bool IsIndexChanged() const { return bIndexChanged; }
	void RegisterObject( const SFullTypeHeader &hdr );
public:
	CEditorDatabase(): bIndexChanged( false ) {}
	//
	bool OpenDatabase( NVFS::IVFS *pVFS, NVFS::IFileCreator *pFileCreator );
	bool RegisterResourceFile( const string &szFileName );
	virtual bool IsFileRegistered( const string &szFileName );
	void SetLoadDepth( int nLoadDepth ) { NI_ASSERT( false, "this functionality are for game only" ) }
	//
	IObjMan *GetManipulator( const CDBID &dbid ) { return GetObjManInternal( dbid ); }
	CResource *GetObject( const CDBID &dbid )
	{
		IObjMan *pObjMan = GetObjManInternal( dbid );
		return pObjMan == 0 ? 0 : pObjMan->GetObject();
	}
	IObjMan *CreateNewObject( const string &szClassTypeName );
	bool AddNewObject( const string &szFilePath, const CDBID &dbid, IObjMan *pObjMan );
	bool RemoveObject( const CDBID &dbid );
	bool RenameObject( const CDBID &dbidOld, const CDBID &dbidNew );
	//
	void MarkChanged( const CDBID &dbid );
	void SaveChanges();
	bool SaveChangedIndex();
	void DropCachedResources();
	//
	bool GetClassesList( vector<NTypeDef::STypeClass*> *pRes );
	bool GetObjectsList( vector<CDBID> *pRes, const string &szClassTypeName );
	bool GetObjectsList( vector<CDBID> *pRes, const int nClassTypeID );
	//
	string GetClassTypeName( const CDBID &_dbid )
	{
		if ( const SElement *pElement = GetElement( _dbid ) )
			return pElement->typeHeader.szClassTypeName;
		else
			return "";
	}
};
CBasicDatabase *CreateEditorDatabase() { return new CEditorDatabase(); }

// ************************************************************************************************************************ //
// **
// ** 
// **
// **
// **
// ************************************************************************************************************************ //

bool CEditorDatabase::OpenDatabase( NVFS::IVFS *_pVFS, NVFS::IFileCreator *_pFileCreator )
{
	SetFileSystem( _pVFS, _pFileCreator );
	//
	bool bRet = NMetaInfo::CreateFullMetaInfoCopy( &metaInfoMap );
	bRet = LoadTypesMap() && bRet;
	LoadIndex(); 
	return bRet;
}

void CEditorDatabase::RegisterObject( const SFullTypeHeader &hdr )
{
	CDBID dbid;
	NormalizeDBID( &dbid, CDBID(hdr.szFileName) );
	elementsMap[dbid].typeHeader = *( static_cast<const STypeObjectHeader*>(&hdr) );
}

bool CEditorDatabase::SaveChangedIndex()
{
	if ( IsIndexChanged() )
	{
		CFileStream stream( GetFileCreator(), INDEX_FILE_NAME );
		if ( stream.IsOk() )
		{
			if ( CPtr<IBinSaver> pSaver = CreateBinSaver(&stream, SAVER_MODE_WRITE) )
			{
				// save changed index
				vector<SFullTypeHeader> objectsIndex( elementsMap.size() );
				int i = 0;
				for ( CElementsMap::const_iterator it = elementsMap.begin(); it != elementsMap.end(); ++it, ++i )
				{
					objectsIndex[i].szFileName = GetFileName( it->first );
					*( static_cast<STypeObjectHeader*>(&objectsIndex[i]) ) = it->second.typeHeader;
				}
				//
				pSaver->Add( 1, &objectsIndex );
				ResetIndexChanged();
				return true;
			}
			else
				return false;
		}
		else
			return false;
	}
	return true;
}

bool CEditorDatabase::LoadTypesMap()
{
	vector< CObj<NDb::NTypeDef::STypeDef> > topLevelTypes;
	CFileStream stream( GetVFS(), TYPES_FILE_NAME );
	if ( stream.IsOk() )
	{
		if ( CPtr<IXmlSaver> pSaver = CreateXmlSaver(&stream, SAVER_MODE_READ) )
			pSaver->Add( "Types", &topLevelTypes );
	}
	else
	{
		theLogger.WriteLog( StrFmt("ERROR: Failed to open file with type descriptions (%s)", TYPES_FILE_NAME) );
		return false;
	}
	if ( topLevelTypes.empty() )
		theLogger.WriteLog( StrFmt("WARNING: %s has loaded but contain no types", TYPES_FILE_NAME) );
	//
	for ( vector< CObj<NDb::NTypeDef::STypeDef> >::iterator it = topLevelTypes.begin(); it != topLevelTypes.end(); ++it )
	{
		if ( (*it)->eType == NDb::NTypeDef::TYPE_TYPE_CLASS )
		{
			NDb::NTypeDef::STypeClass *pTypeClass = checked_cast_ptr<NDb::NTypeDef::STypeClass*>( *it );
			NI_VERIFY( pTypeClass->nClassTypeID != -1, StrFmt("Non-terminal class \"%s\" registering!", pTypeClass->szTypeName.c_str()), continue );
			pTypeClass->RegisterTerminalType();
		}
		typesMap[ (*it)->GetTypeName() ] = *it;
	}
	//
	return true;
}

const CEditorDatabase::SElement *CEditorDatabase::GetElement( const CDBID &_dbid )
{
	CDBID dbid;
	NormalizeDBID( &dbid, _dbid );
	if ( DoesObjectExist(dbid) == false )
		return 0;
	return &( elementsMap[dbid] );
}

int CEditorDatabase::GetClassTypeID( const string &szClassTypeName )
{
	for ( CTypesMap::const_iterator it = typesMap.begin(); it != typesMap.end(); ++it )
	{
		if ( it->second->eType == NTypeDef::TYPE_TYPE_CLASS && it->first == szClassTypeName )
			return static_cast_ptr<const NTypeDef::STypeClass *>( it->second )->nClassTypeID;
	}
	return -1;
}

NMetaInfo::SStructMetaInfo *CEditorDatabase::GetRawStructMetaInfo( const string &szTypeName )
{
	NMetaInfo::CMetaInfoMap::iterator pos = metaInfoMap.find( szTypeName );
	if ( pos == metaInfoMap.end() )
	{
		metaInfoMap[szTypeName] = new NMetaInfo::SStructMetaInfo();
		pos = metaInfoMap.find( szTypeName );
	}
	return pos->second;
}

NMetaInfo::SStructMetaInfo *CEditorDatabase::GetStructMetaInfo( const string &szTypeName )
{
	NMetaInfo::SStructMetaInfo *pMetaInfo = GetRawStructMetaInfo( szTypeName );
	// link struct meta info with type meta info
	if ( !IsValid(pMetaInfo->pStructTypeDef) )
	{
		CTypesMap::iterator pos = typesMap.find( szTypeName );
		NI_VERIFY( pos != typesMap.end(), StrFmt("Can't find type meta info for \"%s\"", szTypeName.c_str()), return 0 );
		//
		NTypeDef::STypeClass *pTypeClass = dynamic_cast_ptr<NTypeDef::STypeClass *>( pos->second );
		NI_VERIFY( pTypeClass != 0, StrFmt("Can't find type meta info for \"%s\"", szTypeName.c_str()), return 0 );
		pMetaInfo->LinkWithTypeDef( "", pTypeClass );
	}
	return pMetaInfo;
}

bool CEditorDatabase::DoesObjectExist( const CDBID &dbid )
{
	if ( elementsMap.find( dbid ) != elementsMap.end() )
		return true;
	else
		return RegisterResourceFile( GetFileName(dbid) );
}

bool CEditorDatabase::IsFileRegistered( const string &szFileName )
{
	CDBID dbid, _dbid( szFileName );
	NormalizeDBID( &dbid, _dbid );
	
	return elementsMap.find( dbid ) != elementsMap.end();
}

bool CEditorDatabase::RegisterResourceFile( const string &szFileName )
{
	CDBID dbid, _dbid( szFileName );
	NormalizeDBID( &dbid, _dbid );
	//
	CElementsMap::iterator pos = elementsMap.find( dbid );
	if ( pos == elementsMap.end() )
	{
		bool bRegistered = ReallyRegisterResourceFile( dbid );
		bIndexChanged = bRegistered || IsIndexChanged();
		return bRegistered;
	}
	return true;
}

bool CEditorDatabase::ReallyRegisterResourceFile( const CDBID &dbid )
{
	NI_VERIFY( elementsMap.find( dbid ) == elementsMap.end(), StrFmt("Resource \"%s\" already exist!", dbid.ToString().c_str()), return false );
	// read object header
	STypeObjectHeader header;
	if ( ReadResourceHeader( &header, dbid ) )
	{
		elementsMap[dbid].typeHeader = header;
		return true;
	}
	return false;
}

NBind::CBindStruct *CEditorDatabase::CreateNewBind( const STypeObjectHeader &header )
{
	NMetaInfo::SStructMetaInfo *pMetaInfo = GetStructMetaInfo( header.szClassTypeName );
	NTypeDef::STypeClass *pTypeClass = checked_cast_ptr<NTypeDef::STypeClass *>( pMetaInfo->pStructTypeDef );
	CObj<CResource> pStruct = dynamic_cast<CResource*>( NObjectFactory::MakeObject(pTypeClass->nClassTypeID) );
	// paranoid check
	//if ( pStruct == 0 )
	//{
	//	const bool bNoCode = pTypeClass->pAttributes && 
	//		(pTypeClass->pAttributes->HashAttribute("noCode") || 
	//		pTypeClass->pAttributes->HashAttribute("noHeader") );
	//	NI_VERIFY( pStruct == 0 && (bNoCode || pTypeClass->nClassTypeID == -1), 
	//		StrFmt("Can't create new object of type \"%s\" correctly", header.szClassTypeName.c_str()), 
	//		return 0 );
	//}
	// create and register bind
	NBind::CBindStruct *pBind = new NBind::CBindStruct( pStruct, pMetaInfo );
	return pBind;
}

IObjMan *CEditorDatabase::GetObjManInternal( const CDBID &_dbid )
{
	SEditorDbForceLoadGuard forceLoadGuard;
	//
	CDBID dbid;
	NormalizeDBID( &dbid, _dbid );
	//
	if ( DoesObjectExist(dbid) == false )
	{
		DebugTrace( "Can't get object manipulator for \"%s\"", dbid.ToString().c_str() );
		return 0;
	}
//	NI_VERIFY( DoesObjectExist(dbid) != false, StrFmt("Can't get object manipulator for \"%s\"", dbid.ToString().c_str()), return 0 );
	//
	CElementsMap::iterator pos = elementsMap.find( dbid );
	if ( pos->second.pBind == 0 )
		pos->second.pBind = CreateNewBind( pos->second.typeHeader );
	NBind::CBindStruct *pBind = pos->second.pBind;
	NI_VERIFY( pBind != 0, StrFmt("Can't get bind for \"%s\" to create manipulator", dbid.ToString().c_str()), return 0 );
	if ( pBind->IsLoaded() )
		return pBind;
	else if ( !SEditorDbForceLoadGuard::CanLoad() )
	{
		pBind->SetDBID( dbid );
		return pBind;
	}
	else
	{
		NI_VERIFY( LoadObject( pBind, dbid ) != false, StrFmt("Can't load data for object \"%s\"", dbid.ToString().c_str()), return 0 );
		forceLoadGuard.Close();
		CResourceHelper::CallPostLoad( pBind->GetObject(), true );
		return pBind;
	}
}

bool CEditorDatabase::LoadObject( NBind::CBindStruct *pBind, const CDBID &dbid )
{
	NI_VERIFY( pBind->IsLoaded() == false, StrFmt("Trying to load already loaded object \"%s\"", dbid.ToString().c_str()), return true );
	//
	pBind->SetDBID( dbid );
	const string szFileName = GetFileName( dbid );
	CFileStream stream( GetVFS(), szFileName );
	NI_VERIFY( stream.IsOk(), StrFmt("Can't open stream \"%s\" to load db object", szFileName.c_str()), return false );

	//
	const char *pBuffer = (const char*)stream.GetBuffer();
	CPtr<NXml::CXmlReader> pXmlReader = new NXml::CXmlReader( pBuffer, pBuffer + stream.GetSize() );

	//
	const NXml::CXmlNode* pRootElement = pXmlReader->GetRootElement();

	const NXml::SXmlValue &name = pRootElement->GetName();
	NI_ASSERT( name == pBind->GetTypeName(),
						StrFmt("Base node name (%s) and type name (%s) mismatch!", name.ToString().c_str(), pBind->GetTypeName().c_str()) );
	// load attributes and objectID
	{
		int nObjectRecordID = -1;
		const vector<const NXml::SXmlAttribute*> &attributes = pRootElement->GetAttributes();
		for ( vector<const NXml::SXmlAttribute*>::const_iterator itAttribute = attributes.begin(); itAttribute != attributes.end(); ++itAttribute )
		{
			const NXml::SXmlAttribute *pAttr = *itAttribute;
			wstring wszUnicodeAttribute;
			NStr::UTF8ToUnicode( &wszUnicodeAttribute, pAttr->value.ToString() );
			pBind->SetAttribute( pAttr->name.ToString(), wszUnicodeAttribute );
		}
	}
	// load main object
	NI_VERIFY( pBind->LoadXML( "", pBind->GetMetaInfo()->pStructTypeDef, pRootElement ) != false, 
						  StrFmt("Can't load data for object \"%s\"", szFileName.c_str()), return false );
	CResourceHelper::SetLoaded( pBind->GetObject() );
	//
	return true;
}

IObjMan *CEditorDatabase::CreateNewObject( const string &szClassTypeName )
{
	NMetaInfo::SStructMetaInfo *pMetaInfo = GetStructMetaInfo( szClassTypeName );
	NI_VERIFY( pMetaInfo != 0 && pMetaInfo->pStructTypeDef != 0, "Can't get struct meta info", return 0 );
	NTypeDef::STypeClass *pTypeClass = dynamic_cast_ptr<NTypeDef::STypeClass *>( pMetaInfo->pStructTypeDef );
	CObj<CResource> pStruct = dynamic_cast<CResource*>( NObjectFactory::MakeObject( pTypeClass->nClassTypeID ) );
	NI_VERIFY( !(pStruct == 0 && pTypeClass->nClassTypeID != -1 && pMetaInfo->nNumCodeValues != 0), 
		         StrFmt("Can't create new object of type \"%s\" correctly", szClassTypeName.c_str()), return 0 );
	NBind::CBindStruct *pBind = new NBind::CBindStruct( pStruct, pMetaInfo );
	pBind->SetDefault( "", pTypeClass );
	pBind->SetLoaded();
	pBind->SetNew( true );
	CResourceHelper::CallPostLoad( pBind->GetObject(), true );
	return pBind;
}

void CEditorDatabase::AddNewObjectInternal( const CDBID &dbid, IObjMan *pObjMan )
{
	NBind::CBindStruct *pBind = dynamic_cast<NBind::CBindStruct *>( pObjMan );
	NI_VERIFY( pBind != 0, StrFmt("Trying to add wrong object \"%s\" to database", dbid.ToString().c_str()), return );
	elementsMap[dbid].pBind = pBind;
	elementsMap[dbid].typeHeader.szClassTypeName = pBind->GetTypeName();
	pBind->SetDBID( dbid );
}

bool CEditorDatabase::AddNewObject( const string &szFilePath, const CDBID &_dbid, IObjMan *pObjMan )
{
	NI_VERIFY( IsDBIDValid(_dbid), "Invalid DBID - can't add new object!", return false );
	//
	CDBID dbid;
	NormalizeDBID( &dbid, _dbid );
	NI_VERIFY( DoesObjectExist(dbid) == false, StrFmt("Element \"%s\" already exists!", dbid.ToString().c_str()), return false );
	AddNewObjectInternal( dbid, pObjMan );
	dynamic_cast<NBind::CBindStruct *>(pObjMan)->SetNew( true );
	SetIndexChanged();
	SetDataChanged();
	CBasicDatabase::ReportObjectAdded( _dbid );
	//
	return true;
}

void CEditorDatabase::RemoveObjectInternal( const CDBID &_dbid )
{
	CDBID dbid;
	NormalizeDBID( &dbid, _dbid );
	elementsMap.erase( dbid );
}

bool CEditorDatabase::RemoveObject( const CDBID &_dbid )
{
	CBasicDatabase::ReportObjectRemoved( _dbid );
	RemoveObjectInternal( _dbid );
	SetIndexChanged();
	SetDataChanged();
	return true;
}

bool CEditorDatabase::RenameObject( const CDBID &_dbidOld, const CDBID &_dbidNew )
{
	CDBID dbidOld, dbidNew;
	NormalizeDBID( &dbidOld, _dbidOld );
	NormalizeDBID( &dbidNew, _dbidNew );
	if ( IsDBIDValid(dbidNew) == false )
		return false;
	// load referenced object
	CObj<IObjMan> pReferencedObj = GetManipulator( dbidOld );
	if ( pReferencedObj == 0 )
		return false;
	//
	vector<CDBID> refObjs;
	if ( NDBWatcherClient::IDBWatcherClient *pClient = Singleton<NDBWatcherClient::IDBWatcherClient>() )
	{
		const string szFileName = GetFileName( dbidOld );
		while ( pClient->GetReferencingObjects( szFileName, &refObjs ) == 
			NDBWatcherClient::IDBWatcherClient::EResult::SERVICE_NOT_READY ) ;
	}
	else
		return false;
	// get all changed objects to force load and set them as changed
	for ( vector<CDBID>::const_iterator it = refObjs.begin(); it != refObjs.end(); ++it )
	{
		GetManipulator( *it );
		MarkChanged( *it );
	}
	//
	ReportObjectMoved( dbidOld, dbidNew );
	//
	RemoveObjectInternal( dbidOld );
	AddNewObjectInternal( dbidNew, pReferencedObj );
	SetIndexChanged();
	MarkChanged( dbidNew );
	//
	return true;
}

void CEditorDatabase::MarkChanged( const CDBID &dbid )
{
	CElementsMap::iterator pos = elementsMap.find( dbid );
	if ( pos != elementsMap.end() )
		pos->second.pBind->SetChanged();
	SetDataChanged();
	CBasicDatabase::ReportObjectChanged( dbid );
}

void CEditorDatabase::SaveChanges()
{
	bool bHasFailedElements = false;
	// save changed elements
	for ( CElementsMap::iterator itElement = elementsMap.begin(); itElement != elementsMap.end(); ++itElement )
	{
		if ( itElement->second.pBind && (itElement->second.pBind->IsChanged() || itElement->second.pBind->IsNew()) )
		{
			const string szFileName = GetFileName( itElement->first );
			CFileStream fileStream( GetFileCreator(), szFileName );
			if ( fileStream.IsOk() )
			{
				NLXML::CXMLDocument xmlDocument;
				// add XML declaration
				NLXML::CXMLDeclaration *pDeclaration = new NLXML::CXMLDeclaration();
				xmlDocument.AddChild( pDeclaration );
				// create root element and set attributes
				const string &szTypeName = itElement->second.pBind->GetTypeName();
				const NBind::CAttributesList &attributes = itElement->second.pBind->GetAttributes();
				NLXML::CXMLElement *pBaseNode = new NLXML::CXMLElement();
				pBaseNode->SetValue( itElement->second.pBind->GetTypeName() );
				for ( NBind::CAttributesList::const_iterator itAttribute = attributes.begin(); itAttribute != attributes.end(); ++itAttribute )
				{
					string szUTF8Attribute;
					NStr::UnicodeToUTF8( &szUTF8Attribute, itAttribute->second );
					pBaseNode->SetAttribute( itAttribute->first, szUTF8Attribute );
				}
				xmlDocument.AddChild( pBaseNode );
				// save object to root element
				NI_VERIFY( itElement->second.pBind->SaveXML( "", itElement->second.pBind->GetMetaInfo()->pStructTypeDef, pBaseNode ) != false,
					StrFmt("Can't save object \"%s\" to file", itElement->first.ToString().c_str()), continue );
				itElement->second.pBind->SetNew( false );
				//
				NLXML_STREAM stream( &fileStream );
				xmlDocument.Store( stream );
			}
			else
			{
				theLogger.WriteLog( StrFmt("Can't create stream \"%s\" to save object \"%s\" to file", szFileName.c_str(), itElement->first.ToString().c_str()) );
				bHasFailedElements = true;
			}
		}
	}
	// save changed index
	SaveChangedIndex();
	//
	if ( !bHasFailedElements )
		ResetDataChanged();
	//
	ReportSaveAllChanges();
}

void CEditorDatabase::DropCachedResources()
{
	{
		// collect new unsaved objects to remove it
		list<CDBID> newObjects;
		for ( CElementsMap::iterator it = elementsMap.begin(); it != elementsMap.end(); ++it )
		{
			if ( it->second.pBind && it->second.pBind->IsNew() )
				newObjects.push_back( it->first );
		}
		// remove new unsaved objects
		for ( list<CDBID>::const_iterator it = newObjects.begin(); it != newObjects.end(); ++it )
			RemoveObjectInternal( *it );
	}
	// reload all changed resources
	for ( CElementsMap::iterator it = elementsMap.begin(); it != elementsMap.end(); ++it )
	{
		if ( it->second.pBind && it->second.pBind->IsChanged() )
		{
			it->second.pBind->ResetLoaded();
			it->second.pBind->ResetChanged();
			LoadObject( it->second.pBind, it->second.pBind->GetDBID() );
		}
	}
	//
	ResetDataChanged();
	CBasicDatabase::ReportDiscardAllChanges();
}

// ************************************************************************************************************************ //
// **
// ** 
// **
// **
// **
// ************************************************************************************************************************ //

bool CEditorDatabase::GetClassesList( vector<NTypeDef::STypeClass*> *pRes )
{
	pRes->resize( 0 );
	pRes->reserve( typesMap.size() );
	for ( CTypesMap::const_iterator it = typesMap.begin(); it != typesMap.end(); ++it )
	{
		if ( it->second->eType == NTypeDef::TYPE_TYPE_CLASS )
			pRes->push_back( checked_cast_ptr<NTypeDef::STypeClass *>( it->second ) );
	}
	return !pRes->empty();
}

bool CEditorDatabase::GetObjectsList( vector<CDBID> *pRes, const int nClassTypeID )
{
	// first, find class type name
	string szClassTypeName;
	for ( CTypesMap::const_iterator it = typesMap.begin(); it != typesMap.end(); ++it )
	{
		if ( it->second->eType == NTypeDef::TYPE_TYPE_CLASS )
		{
			if ( checked_cast_ptr<const NTypeDef::STypeClass *>(it->second)->nClassTypeID == nClassTypeID )
			{
				szClassTypeName = it->first;
				break;
			}
		}
	}
	NI_VERIFY( !szClassTypeName.empty(), StrFmt("Can't find class type name for 0x%.8x", nClassTypeID), return false );
	// get objects list by class type name
	return GetObjectsList( pRes, szClassTypeName );
}

bool CEditorDatabase::GetObjectsList( vector<CDBID> *pRes, const string &szClassTypeName )
{
	if ( szClassTypeName.empty() )
	{
		pRes->resize( 0 );
		pRes->reserve( elementsMap.size() );
		for ( CElementsMap::const_iterator it = elementsMap.begin(); it != elementsMap.end(); ++it )
			pRes->push_back( it->first );
	}
	else
	{
		pRes->resize( 0 );
		pRes->reserve( 512 );
		for ( CElementsMap::const_iterator it = elementsMap.begin(); it != elementsMap.end(); ++it )
		{
			if ( it->second.typeHeader.szClassTypeName == szClassTypeName )
				pRes->push_back( it->first );
		}
	}
	return !pRes->empty();
}

}

