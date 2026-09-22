#include "stdafx.h"

#include "port/unicode.h"

#include "Db.h"
#include "StructMetaInfo.h"
#include "Bind.h"
#include "Database.h"
#include "Index.h"
#include "ReportMetaInfo.h"
#include "System/LightXML.h"
#include "System/XMLReader.h"
#include "System/VFS.h"
#include "System/VFSOperations.h"
#include "System/XMLReader.h"
#include "Logger.h"
#include "DBReferenceScan.h"

#include <fmt/format.h>

#include "libdb_export.h"

EXTERNVAR LIBDB_EXPORT CLogger theLogger;

namespace NDb
{

static std::string szLastSaveError;
const std::string& GetLastSaveError() { return szLastSaveError; }

static void ReportSaveError( const std::string &error )
{
	if ( szLastSaveError.empty() ) szLastSaveError = error;
	theLogger.WriteLog( "Failed to save: " + error );
}

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
	typedef std::unordered_map<CDBID, SElement> CElementsMap;
	CElementsMap elementsMap;							// objects map
	bool bIndexChanged;										// index was changed during elements manipulation
	typedef std::unordered_map<std::string, CObj<NTypeDef::STypeDef> > CTypesMap;
	CTypesMap typesMap;										// type definitions
	NMetaInfo::CMetaInfoMap metaInfoMap;
	//
	IObjMan *GetObjManInternal( const CDBID &dbid );
	bool LoadObject( NBind::CBindStruct *pBind, const CDBID &dbid );
	bool SaveObject( NBind::CBindStruct *pBind, const CDBID &dbid );
	bool ReallyRegisterResourceFile( const CDBID &dbid );
	//
	const SElement *GetElement( const CDBID &_dbid );
	int GetClassTypeID( const std::string &szClassTypeName );
	NBind::CBindStruct *CreateNewBind( const STypeObjectHeader &header );
	//! get raw struct meta info w/o linking with type defs
	NMetaInfo::SStructMetaInfo *GetRawStructMetaInfo( const std::string &szTypeName );
	//! get struct meta info, linked with type defs (link if necessary)
	NMetaInfo::SStructMetaInfo *GetStructMetaInfo( const std::string &szTypeName );
	//! check object exist (register it from storage if necessary)
	bool DoesObjectExist( const CDBID &dbid );
	//
	void RemoveObjectInternal( const CDBID &dbid );
	void AddNewObjectInternal( const CDBID &_dbid, IObjMan *pObjMan );
	bool RenameObjectInternal( const CDBID &dbidOld, const CDBID &dbidNew, const std::vector<CDBID> &refObjs );
	//! answers for every target in a single pass over the database
	void CollectReferencingObjects( std::vector< std::vector<CDBID> > *pRes, const std::vector<CDBID> &targets );
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
	bool RegisterResourceFile( const std::string &szFileName );
	virtual bool IsFileRegistered( const std::string &szFileName );
	void SetLoadDepth( int nLoadDepth ) { NI_ASSERT( false, "this functionality are for game only" ) }
	//
	IObjMan *GetManipulator( const CDBID &dbid ) { return GetObjManInternal( dbid ); }
	CResource *GetObject( const CDBID &dbid )
	{
		IObjMan *pObjMan = GetObjManInternal( dbid );
		return pObjMan == 0 ? 0 : pObjMan->GetObject();
	}
	IObjMan *CreateNewObject( const std::string &szClassTypeName );
	bool AddNewObject( const std::string &szFilePath, const CDBID &dbid, IObjMan *pObjMan );
	bool RemoveObject( const CDBID &dbid );
	bool RenameObject( const CDBID &dbidOld, const CDBID &dbidNew );
	bool RenameObjects( const std::vector< std::pair<CDBID, CDBID> > &renames );
	bool GetReferencingObjects( std::vector<CDBID> *pRes, const CDBID &dbid );
	//
	void MarkChanged( const CDBID &dbid );
	void SaveChanges();
	bool SaveChangedIndex();
	void DropCachedResources();
	//
	bool GetClassesList( std::vector<NTypeDef::STypeClass*> *pRes );
	bool GetObjectsList( std::vector<CDBID> *pRes, const std::string &szClassTypeName );
	bool GetObjectsList( std::vector<CDBID> *pRes, const int nClassTypeID );
	//
	std::string GetClassTypeName( const CDBID &_dbid )
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
		CMemoryStream stream;
		if ( stream.IsOk() )
		{
			if ( CPtr<IBinSaver> pSaver = CreateBinSaver(&stream, SAVER_MODE_WRITE) )
			{
				// save changed index
				std::vector<SFullTypeHeader> objectsIndex( elementsMap.size() );
				int i = 0;
				for ( CElementsMap::const_iterator it = elementsMap.begin(); it != elementsMap.end(); ++it, ++i )
				{
					objectsIndex[i].szFileName = GetFileName( it->first );
					*( static_cast<STypeObjectHeader*>(&objectsIndex[i]) ) = it->second.typeHeader;
				}
				//
				pSaver->Add( 1, &objectsIndex );
				// Binary savers finish their output on destruction.
				pSaver = nullptr;
				std::string error;
				if ( !NVFS::WriteFile( GetFileCreator(), INDEX_FILE_NAME, stream, &error ) )
				{
					ReportSaveError( error );
					SetDataChanged();
					return false;
				}
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
	std::vector< CObj<NDb::NTypeDef::STypeDef> > topLevelTypes;
	CFileStream stream( GetVFS(), TYPES_FILE_NAME );
	if ( stream.IsOk() )
	{
		if ( CPtr<IXmlSaver> pSaver = CreateXmlSaver(&stream, SAVER_MODE_READ) )
			pSaver->Add( "Types", &topLevelTypes );
	}
	else
	{
		theLogger.WriteLog( fmt::format("ERROR: Failed to open file with type descriptions ({})", TYPES_FILE_NAME) );
		return false;
	}
	if ( topLevelTypes.empty() )
		theLogger.WriteLog( fmt::format("WARNING: {} has loaded but contain no types", TYPES_FILE_NAME) );
	//
	for ( std::vector< CObj<NDb::NTypeDef::STypeDef> >::iterator it = topLevelTypes.begin(); it != topLevelTypes.end(); ++it )
	{
		if ( (*it)->eType == NDb::NTypeDef::TYPE_TYPE_CLASS )
		{
			NDb::NTypeDef::STypeClass *pTypeClass = checked_cast_ptr<NDb::NTypeDef::STypeClass*>( *it );
			NI_VERIFY( pTypeClass->nClassTypeID != -1, fmt::format("Non-terminal class \"{}\" registering!", pTypeClass->szTypeName), continue );
			pTypeClass->RegisterTerminalType();
		}
		typesMap[ (*it)->GetTypeName() ] = *it;
	}

	// Installed game data often has the original types.xml. Unlike the game XML
	// loader, editor binds only read fields declared there, so GLB references and
	// selectors must be linked even when the external schema predates GLB support.
	struct SModelField { const char *type; const char *name; int chunk; };
	const SModelField modelFields[] = {
		{ "Geometry", "ModelFileRef", 12 }, { "Geometry", "RootMesh", 13 },
		{ "AIGeometry", "ModelFileRef", 8 }, { "AIGeometry", "RootMesh", 9 },
		{ "Skeleton", "ModelFileRef", 5 }, { "Skeleton", "RootJoint", 6 },
		{ "AnimB2", "ModelFileRef", 11 }, { "AnimB2", "FirstFrame", 12 },
		{ "AnimB2", "LastFrame", 13 }, { "AnimB2", "ClipName", 14 }
	};
	for ( const SModelField &field : modelFields )
	{
		const auto type = typesMap.find( field.type );
		const auto meta = metaInfoMap.find( field.type );
		if ( type == typesMap.end() || meta == metaInfoMap.end() )
			continue;
		NTypeDef::STypeClass *pClass = dynamic_cast<NTypeDef::STypeClass*>( type->second.GetPtr() );
		if ( !pClass )
			continue;
		const auto compiledField = meta->second->fields.find( field.name );
		if ( compiledField == meta->second->fields.end() )
			continue;
		NTypeDef::STypeStructBase::SField *pField = 0;
		for ( auto &existing : pClass->fields )
		{
			if ( existing.szName == field.name )
			{
				pField = &existing;
				break;
			}
		}
		if ( !pField )
		{
			const bool bInteger = compiledField->second.GetType() == NTypeDef::TYPE_TYPE_INT;
			const char *pszType = bInteger ? "int" : "string";
			if ( typesMap.find( pszType ) == typesMap.end() )
				typesMap[pszType] = bInteger
					? static_cast<NTypeDef::STypeDef*>(new NTypeDef::STypeInt())
					: static_cast<NTypeDef::STypeDef*>(new NTypeDef::STypeString());
			NTypeDef::STypeStructBase::SField added;
			added.szName = field.name;
			added.nChunkID = field.chunk;
			added.pType = typesMap[pszType];
			added.defaultValue = bInteger ? CVariant(0) : CVariant("");
			added.wszDesc = L"GLTF/GLB model source or selector. Leave empty for legacy GR2 resources.";
			pClass->fields.push_back( added );
			pField = &pClass->fields.back();
		}
		if ( pField->szName == "ModelFileRef" && !pField->HasAttribute("filepath") )
		{
			// ModelFileRef is a FilePathRef, serialized as <ModelFileRef href="..."/>.
			// Without this attribute the editor reads empty element text, falls back
			// to the GR2 UID, and can erase the GLB reference when saving the resource.
			// Repair existing incomplete schemas too, preserving their other attributes.
			pField->pAttributes = pField->pAttributes
				? new NTypeDef::SAttributes(pField->pAttributes->attributes)
				: new NTypeDef::SAttributes();
			pField->pAttributes->attributes["filepath"] = true;
		}
	}
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

int CEditorDatabase::GetClassTypeID( const std::string &szClassTypeName )
{
	for ( CTypesMap::const_iterator it = typesMap.begin(); it != typesMap.end(); ++it )
	{
		if ( it->second->eType == NTypeDef::TYPE_TYPE_CLASS && it->first == szClassTypeName )
			return static_cast_ptr<const NTypeDef::STypeClass *>( it->second )->nClassTypeID;
	}
	return -1;
}

NMetaInfo::SStructMetaInfo *CEditorDatabase::GetRawStructMetaInfo( const std::string &szTypeName )
{
	NMetaInfo::CMetaInfoMap::iterator pos = metaInfoMap.find( szTypeName );
	if ( pos == metaInfoMap.end() )
	{
		metaInfoMap[szTypeName] = new NMetaInfo::SStructMetaInfo();
		pos = metaInfoMap.find( szTypeName );
	}
	return pos->second;
}

NMetaInfo::SStructMetaInfo *CEditorDatabase::GetStructMetaInfo( const std::string &szTypeName )
{
	NMetaInfo::SStructMetaInfo *pMetaInfo = GetRawStructMetaInfo( szTypeName );
	// link struct meta info with type meta info
	if ( !IsValid(pMetaInfo->pStructTypeDef) )
	{
		CTypesMap::iterator pos = typesMap.find( szTypeName );
		NI_VERIFY( pos != typesMap.end(), fmt::format("Can't find type meta info for \"{}\"", szTypeName), return 0 );
		//
		NTypeDef::STypeClass *pTypeClass = dynamic_cast_ptr<NTypeDef::STypeClass *>( pos->second );
		NI_VERIFY( pTypeClass != 0, fmt::format("Can't find type meta info for \"{}\"", szTypeName), return 0 );
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

bool CEditorDatabase::IsFileRegistered( const std::string &szFileName )
{
	CDBID dbid, _dbid( szFileName );
	NormalizeDBID( &dbid, _dbid );
	
	return elementsMap.find( dbid ) != elementsMap.end();
}

bool CEditorDatabase::RegisterResourceFile( const std::string &szFileName )
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
	NI_VERIFY( elementsMap.find( dbid ) == elementsMap.end(), fmt::format("Resource \"{}\" already exist!", dbid.ToString()), return false );
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
//	NI_VERIFY( DoesObjectExist(dbid) != false, fmt::format("Can't get object manipulator for \"{}\"", dbid.ToString()), return 0 );
	//
	CElementsMap::iterator pos = elementsMap.find( dbid );
	if ( pos->second.pBind == 0 )
		pos->second.pBind = CreateNewBind( pos->second.typeHeader );
	NBind::CBindStruct *pBind = pos->second.pBind;
	NI_VERIFY( pBind != 0, fmt::format("Can't get bind for \"{}\" to create manipulator", dbid.ToString()), return 0 );
	if ( pBind->IsLoaded() )
		return pBind;
	else if ( !SEditorDbForceLoadGuard::CanLoad() )
	{
		pBind->SetDBID( dbid );
		return pBind;
	}
	else
	{
		NI_VERIFY( LoadObject( pBind, dbid ) != false, fmt::format("Can't load data for object \"{}\"", dbid.ToString()), return 0 );
		forceLoadGuard.Close();
		CResourceHelper::CallPostLoad( pBind->GetObject(), true );
		return pBind;
	}
}

bool CEditorDatabase::LoadObject( NBind::CBindStruct *pBind, const CDBID &dbid )
{
	NI_VERIFY( pBind->IsLoaded() == false, fmt::format("Trying to load already loaded object \"{}\"", dbid.ToString()), return true );
	//
	pBind->SetDBID( dbid );
	const std::string szFileName = GetFileName( dbid );
	CFileStream stream( GetVFS(), szFileName );
	NI_VERIFY( stream.IsOk(), fmt::format("Can't open stream \"{}\" to load db object", szFileName), return false );

	//
	const char *pBuffer = (const char*)stream.GetBuffer();
	CPtr<NXml::CXmlReader> pXmlReader = new NXml::CXmlReader( pBuffer, pBuffer + stream.GetSize() );

	//
	const NXml::CXmlNode* pRootElement = pXmlReader->GetRootElement();

	const NXml::SXmlValue &name = pRootElement->GetName();
	NI_ASSERT( name == pBind->GetTypeName(),
						fmt::format("Base node name ({}) and type name ({}) mismatch!", name.ToString(), pBind->GetTypeName()) );
	// load attributes and objectID
	{
		int nObjectRecordID = -1;
		const std::vector<const NXml::SXmlAttribute*> &attributes = pRootElement->GetAttributes();
		for ( std::vector<const NXml::SXmlAttribute*>::const_iterator itAttribute = attributes.begin(); itAttribute != attributes.end(); ++itAttribute )
		{
			const NXml::SXmlAttribute *pAttr = *itAttribute;
			std::wstring wszUnicodeAttribute;
			wszUnicodeAttribute = UTF8ToWide( pAttr->value.ToString() );
			pBind->SetAttribute( pAttr->name.ToString(), wszUnicodeAttribute );
		}
	}
	// load main object
	NI_VERIFY( pBind->LoadXML( "", pBind->GetMetaInfo()->pStructTypeDef, pRootElement ) != false, 
						  fmt::format("Can't load data for object \"{}\"", szFileName), return false );
	CResourceHelper::SetLoaded( pBind->GetObject() );
	//
	return true;
}

IObjMan *CEditorDatabase::CreateNewObject( const std::string &szClassTypeName )
{
	NMetaInfo::SStructMetaInfo *pMetaInfo = GetStructMetaInfo( szClassTypeName );
	NI_VERIFY( pMetaInfo != 0 && pMetaInfo->pStructTypeDef != 0, "Can't get struct meta info", return 0 );
	NTypeDef::STypeClass *pTypeClass = dynamic_cast_ptr<NTypeDef::STypeClass *>( pMetaInfo->pStructTypeDef );
	CObj<CResource> pStruct = dynamic_cast<CResource*>( NObjectFactory::MakeObject( pTypeClass->nClassTypeID ) );
	NI_VERIFY( !(pStruct == 0 && pTypeClass->nClassTypeID != -1 && pMetaInfo->nNumCodeValues != 0), 
		         fmt::format("Can't create new object of type \"{}\" correctly", szClassTypeName), return 0 );
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
	NI_VERIFY( pBind != 0, fmt::format("Trying to add wrong object \"{}\" to database", dbid.ToString()), return );
	elementsMap[dbid].pBind = pBind;
	elementsMap[dbid].typeHeader.szClassTypeName = pBind->GetTypeName();
	pBind->SetDBID( dbid );
}

bool CEditorDatabase::AddNewObject( const std::string &szFilePath, const CDBID &_dbid, IObjMan *pObjMan )
{
	NI_VERIFY( IsDBIDValid(_dbid), "Invalid DBID - can't add new object!", return false );
	//
	CDBID dbid;
	NormalizeDBID( &dbid, _dbid );
	NI_VERIFY( DoesObjectExist(dbid) == false, fmt::format("Element \"{}\" already exists!", dbid.ToString()), return false );
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

//! The rename itself, with the list of objects pointing at dbidOld already in
//! hand, so a caller renaming many objects pays for one scan rather than one
//! scan each.
bool CEditorDatabase::RenameObjectInternal( const CDBID &dbidOld, const CDBID &dbidNew,
	const std::vector<CDBID> &refObjs )
{
	if ( IsDBIDValid(dbidNew) == false )
		return false;
	// load referenced object
	CObj<IObjMan> pReferencedObj = GetManipulator( dbidOld );
	if ( pReferencedObj == 0 )
		return false;
	// Every object that points at this one is loaded and marked changed, so that
	// saving rewrites it with the new name. Without that the rename breaks each
	// of them.
	for ( std::vector<CDBID>::const_iterator it = refObjs.begin(); it != refObjs.end(); ++it )
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

bool CEditorDatabase::RenameObject( const CDBID &_dbidOld, const CDBID &_dbidNew )
{
	CDBID dbidOld, dbidNew;
	NormalizeDBID( &dbidOld, _dbidOld );
	NormalizeDBID( &dbidNew, _dbidNew );
	// Finding what points at the object used to mean asking the XDBWatcher, a C#
	// tray service reached over COM and then .NET Remoting, and spinning in a
	// `while ( ... == SERVICE_NOT_READY );` loop with no timeout while it
	// answered. The service has not existed on any machine this port has run on,
	// so the client reported failure, this function fell to an `else return
	// false`, and renaming an object in the editor silently did nothing at all.
	std::vector<CDBID> refObjs;
	if ( !GetReferencingObjects( &refObjs, dbidOld ) )
		return false;
	return RenameObjectInternal( dbidOld, dbidNew, refObjs );
}

bool CEditorDatabase::RenameObjects( const std::vector< std::pair<CDBID, CDBID> > &renames )
{
	if ( renames.empty() )
		return true;

	std::vector<CDBID> oldNames( renames.size() );
	for ( size_t i = 0; i < renames.size(); ++i )
		NormalizeDBID( &oldNames[i], renames[i].first );

	std::vector< std::vector<CDBID> > refObjs;
	CollectReferencingObjects( &refObjs, oldNames );

	bool bRes = true;
	for ( size_t i = 0; i < renames.size(); ++i )
	{
		CDBID dbidNew;
		NormalizeDBID( &dbidNew, renames[i].second );
		bRes = RenameObjectInternal( oldNames[i], dbidNew, refObjs[i] ) && bRes;
	}
	return bRes;
}

//! Reads every registered object and reports the ones pointing at dbid.
//!
//! The reverse reference map this replaces lived in XDBWatcher.exe, a C# tray
//! application that indexed the whole data directory up front, kept it current
//! with a FileSystemWatcher, and answered over a TCP socket through .NET
//! Remoting behind a COM shim. All of that existed so this question could be
//! answered instantly. It is asked when someone renames an object or opens the
//! References dialog, so instantly was never needed, and the dialog already
//! knows how to wait because the service used to answer "not ready" while it
//! loaded.
//!
//! Scanning on demand is slower per question and has nothing to go stale, which
//! the indexed version did whenever anything changed files behind the watcher:
//! it skipped .svn directories by name, so an update from version control moved
//! files it was not watching.
void CEditorDatabase::CollectReferencingObjects( std::vector< std::vector<CDBID> > *pRes,
	const std::vector<CDBID> &targets )
{
	pRes->clear();
	pRes->resize( targets.size() );
	if ( targets.empty() )
		return;

	// Answering for every target in one pass, because the alternative is
	// quadratic where it matters most: renaming a folder renames each .xdb in it
	// one at a time, and a scan per file over a database this size would take
	// minutes for a directory anyone would think of as small.
	std::vector<std::string> refs;
	for ( CElementsMap::const_iterator it = elementsMap.begin(); it != elementsMap.end(); ++it )
	{
		const CDBID &dbidCandidate = it->first;

		const std::string szFileName = GetFileName( dbidCandidate );
		CFileStream stream( GetVFS(), szFileName );
		if ( !stream.IsOk() )
		{
			// The index outlives the files it names, so a missing one is an
			// ordinary state here and not worth an assert.
			continue;
		}
		const char *pBuffer = reinterpret_cast<const char *>( stream.GetBuffer() );
		if ( pBuffer == 0 )
			continue;

		CollectObjectReferences( &refs, pBuffer, pBuffer + stream.GetSize(), szFileName );
		if ( refs.empty() )
			continue;

		for ( size_t nTarget = 0; nTarget < targets.size(); ++nTarget )
		{
			if ( dbidCandidate == targets[nTarget] )
				continue;
			for ( std::vector<std::string>::const_iterator itRef = refs.begin(); itRef != refs.end(); ++itRef )
			{
				// CDBID compares case and separator insensitively, so the
				// reference as written matches however it was spelled.
				if ( CDBID( *itRef ) == targets[nTarget] )
				{
					(*pRes)[nTarget].push_back( dbidCandidate );
					break;
				}
			}
		}
	}
}

bool CEditorDatabase::GetReferencingObjects( std::vector<CDBID> *pRes, const CDBID &_dbid )
{
	pRes->clear();
	CDBID dbidTarget;
	NormalizeDBID( &dbidTarget, _dbid );
	if ( IsDBIDValid( dbidTarget ) == false )
		return false;

	std::vector< std::vector<CDBID> > results;
	CollectReferencingObjects( &results, std::vector<CDBID>( 1, dbidTarget ) );
	pRes->swap( results[0] );
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
	szLastSaveError.clear();
	bool bHasFailedElements = false;
	// save changed elements
	for ( CElementsMap::iterator itElement = elementsMap.begin(); itElement != elementsMap.end(); ++itElement )
	{
		if ( itElement->second.pBind && (itElement->second.pBind->IsChanged() || itElement->second.pBind->IsNew()) )
		{
			const std::string szFileName = GetFileName( itElement->first );
			CMemoryStream fileStream;
			if ( fileStream.IsOk() )
			{
				NLXML::CXMLDocument xmlDocument;
				// add XML declaration
				NLXML::CXMLDeclaration *pDeclaration = new NLXML::CXMLDeclaration();
				xmlDocument.AddChild( pDeclaration );
				// create root element and set attributes
				const std::string &szTypeName = itElement->second.pBind->GetTypeName();
				const NBind::CAttributesList &attributes = itElement->second.pBind->GetAttributes();
				NLXML::CXMLElement *pBaseNode = new NLXML::CXMLElement();
				pBaseNode->SetValue( itElement->second.pBind->GetTypeName() );
				for ( NBind::CAttributesList::const_iterator itAttribute = attributes.begin(); itAttribute != attributes.end(); ++itAttribute )
				{
					std::string szUTF8Attribute;
					szUTF8Attribute = WideToUTF8( itAttribute->second );
					pBaseNode->SetAttribute( itAttribute->first, szUTF8Attribute );
				}
				xmlDocument.AddChild( pBaseNode );
				// save object to root element
				if ( !itElement->second.pBind->SaveXML( "", itElement->second.pBind->GetMetaInfo()->pStructTypeDef, pBaseNode ) )
				{
					ReportSaveError( "Cannot serialize " + szFileName );
					bHasFailedElements = true;
					continue;
				}
				//
				{
					// Flush the XML writer's final buffered bytes before committing.
					NLXML_STREAM stream( &fileStream );
					xmlDocument.Store( stream );
				}
				std::string error;
				if ( !NVFS::WriteFile( GetFileCreator(), szFileName, fileStream, &error ) )
				{
					ReportSaveError( error );
					bHasFailedElements = true;
					continue;
				}
				itElement->second.pBind->SetNew( false );
				itElement->second.pBind->ResetChanged();
			}
			else
			{
				theLogger.WriteLog( fmt::format("Can't create stream \"{}\" to save object \"{}\" to file", szFileName, itElement->first.ToString()) );
				bHasFailedElements = true;
			}
		}
	}
	// save changed index
	bHasFailedElements = !SaveChangedIndex() || bHasFailedElements;
	//
	if ( !bHasFailedElements )
	{
		ResetDataChanged();
		ReportSaveAllChanges();
	}
	else
		SetDataChanged();
}

void CEditorDatabase::DropCachedResources()
{
	{
		// collect new unsaved objects to remove it
		std::list<CDBID> newObjects;
		for ( CElementsMap::iterator it = elementsMap.begin(); it != elementsMap.end(); ++it )
		{
			if ( it->second.pBind && it->second.pBind->IsNew() )
				newObjects.push_back( it->first );
		}
		// remove new unsaved objects
		for ( std::list<CDBID>::const_iterator it = newObjects.begin(); it != newObjects.end(); ++it )
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

bool CEditorDatabase::GetClassesList( std::vector<NTypeDef::STypeClass*> *pRes )
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

bool CEditorDatabase::GetObjectsList( std::vector<CDBID> *pRes, const int nClassTypeID )
{
	// first, find class type name
	std::string szClassTypeName;
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
	NI_VERIFY( !szClassTypeName.empty(), fmt::format("Can't find class type name for 0x{:08x}", nClassTypeID), return false );
	// get objects list by class type name
	return GetObjectsList( pRes, szClassTypeName );
}

bool CEditorDatabase::GetObjectsList( std::vector<CDBID> *pRes, const std::string &szClassTypeName )
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

