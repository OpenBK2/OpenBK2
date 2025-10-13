#include "stdafx.h"
#include "mapeditorlib/commandhandlerdefines.h"
#include "mapeditorlib/resourcedefines.h"
#include "MapEditorLib/Interface_MOD.h"
#include "Misc/2Darray.h"
#include "CommandHandlerDefines.h"
#include "ResourceDefines.h"

#include "AckExcelReader.h"
#include "AckSetBuilder.h"
#include "MapEditorLib/BuilderFactory.h"
#include "MapEditorLib/Interface_Exporter.h"
#include "MapEditorLib/Interface_Logger.h"
#include "MapEditorLib/Tools_HashSet.h"
#include "MapEditorLib/ManipulatorManager.h"
#include "System/FileUtils.h"
#include "Misc/StrProc.h"
#include "libdb/ResourceManager.h"
#include "Stats_B2_M1/AckTypes.h"

#include "libdb/EditorDb.h"
#include "libdb/ObjMan.h"

#include <cstdint>

#include <zconf.h>

#define ACK_SET_TYPE_NAME "AckSetRPGStats"
#define COMPLEX_SOUND_DESC_TYPE_NAME "ComplexSoundDesc"
#define SOUND_DESC_TYPE_NAME "SoundDesc"

//#define COMPLEX_SOUND_DESC_ADD_PATH "Game\\Mission\\Acks\\"
//#define SOUND_DESC_ADD_PATH "All\\Acks\\"

#define COMMON_ADD_PATH "Sounds\\Acks\\"

REGISTER_BUILDER_IN_DLL( AckSetRPGStats, CAcksBuilder )

namespace NAcks
{

struct SAckDesc
{
	const char *pszExcelName;
	const char *pszCodeName;
	NDb::EUnitAckType eType;
};
static SAckDesc s_AckDescs[] = 
{
	{ "ACK_POSITIVE", "ACK_POSITIVE", NDb::ACK_POSITIVE },
	{ "ACK_NEGATIVE", "ACK_NEGATIVE", NDb::ACK_NEGATIVE },
	{ "ACK_SELECTED", "ACK_SELECTED", NDb::ACK_SELECTED },
	{ "ACK_INVALID_TARGET", "ACK_INVALID_TARGET", NDb::ACK_INVALID_TARGET },
	{ "ACK_DONT_SEE_THE_ENEMY", "ACK_DONT_SEE_THE_ENEMY", NDb::ACK_DONT_SEE_THE_ENEMY },
	{ "ACK_NOT_IN_ATTACK_ANGLE", "ACK_NOT_IN_ATTACK_ANGLE", NDb::ACK_NOT_IN_ATTACK_ANGLE },
	{ "ACK_NOT_IN_FIRE_RANGE", "ACK_NOT_IN_FIRE_RANGE", NDb::ACK_NOT_IN_FIRE_RANGE },
	{ "ACK_NO_AMMO", "ACK_NO_AMMO", NDb::ACK_NO_AMMO },
	{ "ACK_CANNOT_PIERCE", "ACK_CANNOT_PIERCE", NDb::ACK_CANNOT_PIERCE },
	{ "ACK_BORED_ATTACK", "ACK_BORED_ATTACK", NDb::ACK_BORED_ATTACK },
	{ "ACK_CANNOT_MOVE_TRACK_DAMAGED", "ACK_CANNOT_MOVE_TRACK_DAMAGED", NDb::ACK_CANNOT_MOVE_TRACK_DAMAGED },
	{ "ACK_BORED_LOW_AMMO", "ACK_BORED_LOW_AMMO", NDb::ACK_BORED_LOW_AMMO },
	{ "ACK_BORED_IDLE", "ACK_BORED_IDLE", NDb::ACK_BORED_IDLE },
	{ "ACK_BORED_LOW_HIT_POINTS", "ACK_BORED_LOW_HIT_POINTS", NDb::ACK_BORED_LOW_HIT_POINTS },
	{ "ACK_SELECTION_TO_MUCH", "ACK_SELECTION_TO_MUCH", NDb::ACK_SELECTION_TO_MUCH },
	{ "ACK_KILLED_ENEMY", "ACK_KILLED_ENEMY", NDb::ACK_KILLED_ENEMY },
	{ "ACK_BUILDING_FINISHED", "ACK_BUILDING_FINISHED", NDb::ACK_BUILDING_FINISHED },
	{ "DIED", "ACK_UNIT_DIED", NDb::ACK_UNIT_DIED },
	{ "ACK_MOVE_END", "ACK_MOVE_END", NDb::ACK_MOVE_END },
	{ "ACK_UNDER_ATTACK", "ACK_UNDER_ATTACK", NDb::ACK_UNDER_ATTACK },
	{ "ACK_ENEMY_FOUND", "ACK_ENEMY_FOUND", NDb::ACK_ENEMY_FOUND },
	{ "ACK_ORDER_FINISHED", "ACK_ORDER_FINISHED", NDb::ACK_ORDER_FINISHED },
	{ "ACK_REINFORCEMENT_ARRIVED", "ACK_REINFORCEMENT_ARRIVED", NDb::ACK_REINFORCEMENT_ARRIVED },
	{ "ACK_GOING_TO_STORAGE", "ACK_GOING_TO_STORAGE", NDb::ACK_GOING_TO_STORAGE },
	{ "ACK_NO_RESOURCES_CANT_FIND_DEPOT", "ACK_NO_RESOURCES_CANT_FIND_DEPOT", NDb::ACK_NO_RESOURCES_CANT_FIND_DEPOT },
	{ "ACK_START_SERVICE_RESUPPLY", "ACK_START_SERVICE_RESUPPLY", NDb::ACK_START_SERVICE_RESUPPLY },
	{ "ACK_START_SERVICE_REPAIR", "ACK_START_SERVICE_REPAIR", NDb::ACK_START_SERVICE_REPAIR },
	{ "ACK_CANNOT_FINISH_BUILD", "ACK_CANNOT_FINISH_BUILD", NDb::ACK_CANNOT_FINISH_BUILD },
	{ "ACK_MINE_FOUND", "ACK_MINE_FOUND", NDb::ACK_MINE_FOUND },
	{ "ACK_PLANE_REACH_POINT_START_ATTACK", "ACK_PLANE_REACH_POINT_START_ATTACK", NDb::ACK_PLANE_REACH_POINT_START_ATTACK },
	{ "ACK_PLANE_LEAVING", "ACK_PLANE_LEAVING", NDb::ACK_PLANE_LEAVING },
	{ "ACK_BEING_ATTACKED_BY_AVIATION", "ACK_BEING_ATTACKED_BY_AVIATION", NDb::ACK_BEING_ATTACKED_BY_AVIATION },
	{ "ACK_START_SERVICE_BUILDING", "ACK_START_SERVICE_BUILDING", NDb::ACK_START_SERVICE_BUILDING },
	{ 0, 0, NDb::ACK_NONE }
};
static hash_map<string, string> s_AckTypesMap;
typedef hash_map<CDBID, string> CDBIDMap;
static struct SEditorAckTypesAutoMagic
{
	SEditorAckTypesAutoMagic()
	{
		for ( SAckDesc *pAck = s_AckDescs; pAck->pszExcelName != 0; ++pAck )
			s_AckTypesMap[pAck->pszExcelName] = pAck->pszCodeName;
	}
} aEditorAckTypesAutoMagic;

void CollectEntries( CDBIDMap *pRes, const string &szFolderInEditor, const string &szTypeName )
{
	CDBID dbid1( szFolderInEditor );
	if ( CPtr<IManipulator> pFolderMan = Singleton<IResourceManager>()->CreateFolderManipulator(szTypeName) )
	{
		for ( CPtr<IManipulatorIterator> pIt = pFolderMan->Iterate(true, ECT_CACHE_LOCAL); !pIt->IsEnd(); pIt->Next() )
		{
			if ( !pIt->IsFolder() )
			{
				string szName;
				if ( pIt->GetName( &szName ) != false )
				{
					if ( szName.size() >= szFolderInEditor.size() )
					{
						CDBID dbid2( szName.substr(0, szFolderInEditor.size()) );
						if ( dbid2 == dbid1 )
							(*pRes)[szName] = szName;
					}
				}
			}
		}
	}
}

// type: artillery, tanks, infantry, etc.  \
// subtype: artillery1, artillery2, etc.    > addresses acks set
// voice ID: 0, 1, 2, 3                    /
// situation code: positive, negative, selected, etc. | addresses acks subset
// szRecordCode: artillery1-1-2, artillery2-3-1       | addresses concrete ack
struct SAckSetKey
{
	string szTypeName;
	int nSubtype;
	int nVoiceID;
	//
	SAckSetKey(): nSubtype(-1), nVoiceID(-1) {}
	SAckSetKey( const string &_szTypeName, int _nSubtype, int _nVoiceID )
		: szTypeName( _szTypeName ), nSubtype( _nSubtype ), nVoiceID( _nVoiceID ) {}
	//
	bool operator==( const SAckSetKey &key ) const 
	{ 
		return key.szTypeName == szTypeName && key.nSubtype == nSubtype && key.nVoiceID == nVoiceID; 
	}
};
}

namespace nstl
{
	template<> struct hash<NAcks::SAckSetKey>
	{
		size_t operator()( const NAcks::SAckSetKey &key ) const { return hash<string>()( key.szTypeName ) + key.nSubtype + key.nVoiceID; }
	};
}

namespace NAcks
{
struct SAckSet
{
	struct SAck
	{
		string szRecordCode;
		string szFileName;
		float fProbability;
	};
	hash_map<string, list<SAck> > acks;
};

bool UpdateAckSets( const string &szExcelFileName, const string &szFolderInEditor, const string &szSoundSource )
{
	vector<SAckEntry> entries;
	if ( LoadAcksTable( &entries, szExcelFileName ) == false || entries.empty() )
	{
		NLog::Log( LT_ERROR, "Can't load acks from Excel file \"%s\"", szExcelFileName.c_str() );
		return false;
	}
	//
	IFolderCallback *pFolderCallback = Singleton<IFolderCallback>();
	//
	// collect ack sets
	CDBIDMap oldAcksSets, oldComplexSoundDescs, oldSoundDescs;
	CollectEntries( &oldAcksSets, szFolderInEditor, ACK_SET_TYPE_NAME );
	for ( CDBIDMap::const_iterator itAckSet = oldAcksSets.begin(); itAckSet != oldAcksSets.end(); ++itAckSet )
	{
		CObj<NDb::IObjMan> pAckSetMan = NDb::GetManipulator( itAckSet->second );
		int nNumTypes = 0;
		pAckSetMan->GetValue( "types", &nNumTypes );
		for ( int i = 0; i < nNumTypes; ++i )
		{
			CDBID dbidComplexSoundDesc;
			pAckSetMan->GetValue( StrFmt("types.[%d].Ack", i), &dbidComplexSoundDesc );
			if ( !dbidComplexSoundDesc.IsEmpty() )
			{
				oldComplexSoundDescs[dbidComplexSoundDesc] = dbidComplexSoundDesc.ToString();
				if ( CObj<NDb::IObjMan> pComplexSoundDescMan = NDb::GetManipulator(dbidComplexSoundDesc) )
				{
					int nNumSounds = 0;
					pComplexSoundDescMan->GetValue( "sounds", &nNumSounds );
					for ( int j = 0; j < nNumSounds; ++j )
					{
						CDBID dbidSoundDesc;
						pComplexSoundDescMan->GetValue( StrFmt("sounds.[%d].PathName", j), &dbidSoundDesc );
						if ( !dbidSoundDesc.IsEmpty() )
							oldSoundDescs[dbidSoundDesc] = dbidSoundDesc.ToString();
					}
				}
			}
		}
	}

//	CollectEntries( &complexSoundDescs, COMMON_ADD_PATH + szFolderInEditor, COMPLEX_SOUND_DESC_TYPE_NAME );
//	CollectEntries( &soundDescs, COMMON_ADD_PATH + szFolderInEditor, SOUND_DESC_TYPE_NAME );
	//
	// create new ack sets from excel file
	//
	typedef hash_map<SAckSetKey, SAckSet> CAckSetsMap;
	CAckSetsMap newAckSets;
	for ( vector<SAckEntry>::const_iterator it = entries.begin(); it != entries.end(); ++it )
	{
		SAckSetKey key;
		const int nTypeNamePos = it->szRecordCode.find_first_of( "1234567890" );
		if ( nTypeNamePos == string::npos )
			continue;
		key.szTypeName = it->szRecordCode.substr( 0, nTypeNamePos );
		const int nSubtypePos = it->szRecordCode.find_first_not_of( "1234567890", nTypeNamePos );
		if ( nSubtypePos == string::npos )
			continue;
		key.nSubtype = atoi( it->szRecordCode.substr(nTypeNamePos, nSubtypePos - nTypeNamePos).c_str() );
		key.nVoiceID = it->nSubsetCode;
		//
		SAckSet &ackSet = newAckSets[key];
		list<SAckSet::SAck> &acks = ackSet.acks[it->szSituationCode];
		list<SAckSet::SAck>::iterator pos = acks.insert( acks.end(), SAckSet::SAck() );
		pos->szRecordCode = it->szRecordCode;
		pos->szFileName = it->szFileName;
		pos->fProbability = it->fProbability;
	}
	//
	// update old ack sets and create new ones
	for ( CAckSetsMap::const_iterator itAckSet = newAckSets.begin(); itAckSet != newAckSets.end(); ++itAckSet )
	{
		const string szAckSetFolder = szFolderInEditor + StrFmt( "%s\\%s%d_voice%d\\", itAckSet->first.szTypeName.c_str(), itAckSet->first.szTypeName.c_str(), itAckSet->first.nSubtype, itAckSet->first.nVoiceID );
		const string szAckSetName = szAckSetFolder + "AckSetRPGStats.xdb";
		const CDBID dbidAckSet( szAckSetName );
//		DebugTrace( szAckSetName.c_str() );
		//
		if ( oldAcksSets.find(dbidAckSet) == oldAcksSets.end() )
			pFolderCallback->InsertObject( ACK_SET_TYPE_NAME, szAckSetName );
		else
			oldAcksSets.erase( dbidAckSet );
		//
		if ( CObj<NDb::IObjMan> pAckSetMan = NDb::GetManipulator(dbidAckSet) )
		{
			pAckSetMan->Remove( "types", MAN_REMOVE_ALL );
			pAckSetMan->SetValue( "VoiceNumber", itAckSet->first.nVoiceID );
			//
			int nTypeNumber = 0;
			for ( hash_map<string, list<SAckSet::SAck> >::const_iterator itAckList = itAckSet->second.acks.begin(); itAckList != itAckSet->second.acks.end(); ++itAckList )
			{
				const string szAckSituationName = s_AckTypesMap[itAckList->first];
				const string szComplexSoundDescName = szAckSetFolder + szAckSituationName + ".xdb";
				const CDBID dbidComplexSoundDesc( szComplexSoundDescName );
				if ( oldComplexSoundDescs.find(dbidComplexSoundDesc) == oldComplexSoundDescs.end() )
					pFolderCallback->InsertObject( COMPLEX_SOUND_DESC_TYPE_NAME, szComplexSoundDescName );
				else
					oldComplexSoundDescs.erase( dbidComplexSoundDesc );
//				DebugTrace( "\t%s", szComplexSoundDescName.c_str() );
				//
				if ( CObj<NDb::IObjMan> pComplexSoundDescMan = NDb::GetManipulator(dbidComplexSoundDesc) )
				{
					if ( pAckSetMan->Insert( "types", MAN_APPEND ) == false )
						continue;
					const string szAckEntryName = StrFmt( "types.[%d]", nTypeNumber );
					pAckSetMan->SetValue( szAckEntryName + ".AckType", szAckSituationName );
					pAckSetMan->SetValue( szAckEntryName + ".Ack", dbidComplexSoundDesc );
					pComplexSoundDescMan->Remove( "sounds", MAN_REMOVE_ALL );
					//
					int nSoundNumber = 0;
					for ( list<SAckSet::SAck>::const_iterator itAck = itAckList->second.begin(); itAck != itAckList->second.end(); ++itAck )
					{
						const string szSoundDescBase = szAckSetFolder + itAck->szRecordCode;
						const string szSoundDescName = szSoundDescBase + ".xdb";
						const CDBID dbidSoundDesc( szSoundDescName );
						if ( oldSoundDescs.find(dbidSoundDesc) == oldSoundDescs.end() )
							pFolderCallback->InsertObject( SOUND_DESC_TYPE_NAME, szSoundDescName );
						else
							oldSoundDescs.erase( dbidSoundDesc );
//						DebugTrace( "\t\t%s", szSoundDescName.c_str() );
						if ( CObj<NDb::IObjMan> pSoundDescMan = NDb::GetManipulator(dbidSoundDesc) )
						{
							if ( pComplexSoundDescMan->Insert( "sounds", MAN_APPEND ) == false )
								continue;
							const string szSoundEntryName = StrFmt( "sounds.[%d]", nSoundNumber );
							pComplexSoundDescMan->SetValue( szSoundEntryName + ".Probability", itAck->fProbability );
							pComplexSoundDescMan->SetValue( szSoundEntryName + ".soundType", string("NORMAL") );
							pComplexSoundDescMan->SetValue( szSoundEntryName + ".PathName", szSoundDescName );
							//
							const string szSoundFileName = szSoundDescBase + ".wav";
							pSoundDescMan->SetValue( "SoundPath", szSoundFileName );
							//
							const string szSrcFileName = Singleton<IUserDataContainer>()->Get()->constUserData.szExportSourceFolder + szSoundSource + itAck->szFileName;
							const string szDstFileName = Singleton<IMODContainer>()->GetDataFolder( SUserData::NPT_EXPORT_DESTINATION ) + szSoundFileName;
							NFile::CopyFile( szSrcFileName, szDstFileName );
							// export sound desc
//							{
//								IResourceManager *pRM = Singleton<IResourceManager>();
//								if ( CPtr<IManipulator> pSoundDescMan2 = pRM->CreateObjectManipulator( SOUND_DESC_TYPE_NAME, szSoundDescName ) )
//									Singleton<IExporterContainer>()->ExportObject( pSoundDescMan2, SOUND_DESC_TYPE_NAME, szSoundDescName, true, false  );
//							}
							//
							++nSoundNumber;
						}
					}
					++nTypeNumber;
				}
			}
		}
	}
	// delete old entries
	for ( CDBIDMap::const_iterator it = oldSoundDescs.begin(); it != oldSoundDescs.end(); ++it )
		NDb::RemoveObject( it->first );
	for ( CDBIDMap::const_iterator it = oldComplexSoundDescs.begin(); it != oldComplexSoundDescs.end(); ++it )
		NDb::RemoveObject( it->first );
	for ( CDBIDMap::const_iterator it = oldAcksSets.begin(); it != oldAcksSets.end(); ++it )
		NDb::RemoveObject( it->first );
	//
	return true;
}

}

// ************************************************************************************************************************ //
// **
// ** builder itself
// **
// **
// **
// ************************************************************************************************************************ //



CAcksBuilder::CAcksBuilder()
{
	Singleton<ICommandHandlerContainer>()->Set( CHID_ACKS_BUILDER, this );
	Singleton<ICommandHandlerContainer>()->Register( CHID_ACKS_BUILDER, ID_TOOLS_CREATE_ACK_SETS, ID_TOOLS_CREATE_ACK_SETS );
}


CAcksBuilder::~CAcksBuilder()
{
	Singleton<ICommandHandlerContainer>()->UnRegister( CHID_ACKS_BUILDER );
	Singleton<ICommandHandlerContainer>()->Remove( CHID_ACKS_BUILDER );
}


//CRAP{ PLAIN_TEXT
bool CAcksBuilder::IsValidBuildData( IManipulator *pBuildDataManipulator, string *pszDescription, IView *pBuildDataView )
{
	NI_ASSERT( pBuildDataManipulator != 0, "CMapInfoBuilder::IsValidBuildData() pBuildDataManipulator == 0" );
	NI_ASSERT( pszDescription != 0, "CMapInfoBuilder::IsValidBuildData() pszDescription == 0" );
	pszDescription->clear();	
	// Считываем данные
	string szExcelFileName;
	if ( !CManipulatorManager::GetValue( &szExcelFileName, pBuildDataManipulator, "ExcelFile" ) || szExcelFileName.empty() )
	{
		( *pszDescription ) = "<ExcelFile> must be filled.";
		return false;
	}
	if ( !NFile::DoesFileExist( ( Singleton<IUserDataContainer>()->Get()->constUserData.szExportSourceFolder + szExcelFileName ) ) )
	{
		( *pszDescription ) = "<ExcelFile> is invalid file name. Can't find file.";
		return false;
	}
	string szSourceDir;
	if ( !CManipulatorManager::GetValue( &szSourceDir, pBuildDataManipulator, "SourceDir" ) || szSourceDir.empty() )
	{
		( *pszDescription ) = "<SourceDir> must be filled.";
		return false;
	}
	return true;
}
//CRAP} PLAIN_TEXT


bool CAcksBuilder::IsUniqueObjectName( const string &szObjectType, const string &szObjectName )
{
	return Singleton<IFolderCallback>()->IsUniqueName( szObjectType, szObjectName );
}


bool CAcksBuilder::UpdateAckSets( const string &rszAnimationFolder )
{
	CWaitCursor waitCursor;
	//
	SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	IFolderCallback *pFolderCallback = Singleton<IFolderCallback>();
	//
	string szAckSetTypeName = ACK_SET_TYPE_NAME;
	string szExcelFileName;
	string szSourceDir;
	// Получаем имя файла и RootJoint
	SBuildDataParams buildDataParams;
	buildDataParams.nFlags = BDF_CHECK_PROPERTIES;
	buildDataParams.szObjectTypeName = szAckSetTypeName;
	buildDataParams.szObjectName = rszAnimationFolder;
	buildDataParams.bNeedExport = false;
	buildDataParams.bNeedEdit = false;
	//
	string szBuildDataTypeName = "AcksBuilder";
	string szBuildDataName;
	if ( Singleton<IBuilderContainer>()->FillBuildData( &szBuildDataTypeName, &szBuildDataName, &buildDataParams, this ) )
	{
		if ( CPtr<IManipulator> pBuildDataManipulator = Singleton<IResourceManager>()->CreateObjectManipulator( szBuildDataTypeName, szBuildDataName ) )
		{
			string szDescription;
			if ( IsValidBuildData( pBuildDataManipulator, &szDescription, 0 ) )
			{
				if ( !CManipulatorManager::GetValue( &szExcelFileName, pBuildDataManipulator, "ExcelFile" ) )
				{
					return false;
				}
				if ( !CManipulatorManager::GetValue( &szSourceDir, pBuildDataManipulator, "SourceDir" ) )
				{
					return false;
				}
				NAcks::UpdateAckSets( pUserData->constUserData.szExportSourceFolder + szExcelFileName, rszAnimationFolder, szSourceDir );
				NDb::SaveChanges();
			}
		}
	}
	return false;
}


bool CAcksBuilder::HandleCommand( UINT nCommandID, uint32_t dwData )
{
	switch( nCommandID )
	{
	case ID_TOOLS_CREATE_ACK_SETS:
		{	
			SSelectionSet selectionSet;
			bool bResult = Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_OBJECT_STORAGE, ID_OS_GET_SELECTION, reinterpret_cast<uint32_t>( &selectionSet ) );
			const string szObjectTypeName = selectionSet.szObjectTypeName;
			bResult = bResult && ( szObjectTypeName == ACK_SET_TYPE_NAME );
			bResult = bResult && ( !selectionSet.objectNameList.empty() );
			if ( bResult )
			{
				const string szObjectName = selectionSet.objectNameList.front().ToString();
				bResult = bResult && ( szObjectName )[szObjectName.size() - 1] == PATH_SEPARATOR_CHAR;
				bResult = bResult && UpdateAckSets( szObjectName );
			}
			return bResult;
		}
	default:
		return false;
	}
	return false;
}


bool CAcksBuilder::UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CAcksBuilder::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CAcksBuilder::UpdateCommand(), pbCheck == 0" );
	//
	switch( nCommandID )
	{
	case ID_TOOLS_CREATE_ACK_SETS:
		{
			SSelectionSet selectionSet;
			bool bResult = Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_OBJECT_STORAGE, ID_OS_GET_SELECTION, reinterpret_cast<uint32_t>( &selectionSet ) );
			const string szObjectTypeName = selectionSet.szObjectTypeName;
			bResult = bResult && ( szObjectTypeName == ACK_SET_TYPE_NAME );
			bResult = bResult && ( !selectionSet.objectNameList.empty() );
			if ( bResult )
			{
				const string szObjectName = selectionSet.objectNameList.front().ToString();
				bResult = bResult && ( szObjectName )[szObjectName.size() - 1] == PATH_SEPARATOR_CHAR;
				( *pbEnable ) = bResult;
				( *pbCheck ) = false;
			}
			return true;
		}
	default:
		return false;
	}
	return false;
}



