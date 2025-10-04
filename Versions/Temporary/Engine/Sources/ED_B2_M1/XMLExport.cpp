#include "stdafx.h"

#include "Misc/StrProc.h"
#include "XMLExport.h"
#include "libdb/ResourceManager.h"
#include "MapEditorLib/ManipulatorManager.h"
#include "MapEditorLib/PCIEMnemonics.h"
#include "System/FilePath.h"

#define NUM_SYSTEM_CHUNKS 1
namespace NXMLExport
{

struct SLevelDesc
{
	string szName;
	string szType;
	//
	SLevelDesc() {}
	SLevelDesc( const string &_szName, const string &_szType )
		: szName( _szName ), szType( _szType ) {}
};
typedef vector<SLevelDesc> CLevelsList;
inline bool IsInRect( const CLevelsList &levels )
{
	if ( levels.size() < 2 )
		return false;
	return ( levels[levels.size() - 2].szType == "Rect" );
}

void ConvertXMLSysChars( string *pString )
{
	string szRes;
	szRes.reserve( pString->size() );
	for ( int i = 0; i < pString->size(); )
	{
		const int chr = (*pString)[i];

		switch ( chr ) 
		{
		case '&':
			if ( (i < ( (int)(*pString).length() - 2 )) && ((*pString)[i + 1] == '#') && ((*pString)[i + 2] == 'x') ) 
			{
				// Hexadecimal character reference.
				// Pass through unchanged.
				// &#xA9;	-- copyright symbol, for example.
				while ( i < (int)(*pString).length() )
				{
					szRes += (*pString)[i];
					++i;
					if ( (*pString)[i] == ';' )
						break;
				}
			}
			else
			{
				szRes += "&amp;";
				++i;
			}
			break;
		case '<':
			szRes += "&lt;";
			++i;
			break;
		case '>':
			szRes += "&gt;";
			++i;
			break;
		case '\"':
			szRes += "&quot;";
			++i;
			break;
		case '\'':
			szRes += "&apos;";
			++i;
			break;
		default:
			szRes += char( chr );
			++i;
		}
	}
	*pString = szRes;
}

void VariantToString( string *pString, const CVariant &variant, EPCIEType ePCIType, bool bConvertSysChars )
{
	switch ( variant.GetType() )
	{
	case CVariant::VT_NULL:
		pString->clear();
		break;

	case CVariant::VT_INT:
		*pString = StrFmt( "%d", (int)variant );
		break;

	case CVariant::VT_FLOAT:
		*pString = StrFmt( "%g", (float)variant );
		break;

	case CVariant::VT_BOOL:
		*pString = ((bool)variant) == false ? "false" : "true";
		break;

	case CVariant::VT_STR:
		*pString = variant.GetStr();
		if ( bConvertSysChars )
			ConvertXMLSysChars( pString );
		break;

	case CVariant::VT_WSTR:
		NStr::UnicodeToUTF8( pString, variant.GetWStr() );
		if ( bConvertSysChars )
			ConvertXMLSysChars( pString );
		break;

	case CVariant::VT_POINTER:
		if ( ePCIType == PCIE_GUID )
		{
			NI_ASSERT( variant.GetBlobSize() == sizeof(GUID), "Size mismatch during GUID convertion" );
			if ( variant.GetBlobSize() == sizeof(GUID) )
				NStr::GUID2String( pString, *((GUID*)variant.GetPtr()) );
			else
				pString->clear();
		}
		else
		{
			pString->resize( variant.GetBlobSize() * 2 );
			NStr::BinToString( variant.GetPtr(), variant.GetBlobSize(), const_cast<char*>(pString->c_str()) );
		}
		break;

	default:
		NI_ASSERT( false, StrFmt("Can't convert type %d to string", variant.GetType()) );
	}
}

void PrintTabs( const int nAmount, FILE *file )
{
	if ( nAmount <= 0 )
		return;
	char buffer[256];
	for ( int i = 0; i < nAmount; ++i )
		buffer[i] = '\t';
	buffer[nAmount] = 0;
	fprintf( file, buffer );
}

void StartLevel( const string &szName, const string &szType, CLevelsList &levels, FILE *file, int nArraySize, list< pair<string, string> > *pAtributes )
{
	string szAttributes;
	if ( pAtributes && !pAtributes->empty() )
	{
		for ( list< pair<string, string> >::const_iterator it = pAtributes->begin(); it != pAtributes->end(); ++it )
			szAttributes += " " + it->first + "=\"" + it->second + "\"";
	}
	//
	if ( nArraySize == 0 )
	{
		PrintTabs( levels.size(), file );
		if ( szName[0] == '[' )
			fprintf( file, "<Item%s/>\n", szAttributes.c_str() );
		else
			fprintf( file, "<%s%s/>\n", szName.c_str(), szAttributes.c_str() );
	}
	else
	{
		levels.push_back( SLevelDesc(szName, szType) );
		if ( !IsInRect(levels) )
		{
			PrintTabs( levels.size() - 1, file );
			if ( szName[0] == '[' )
			{
				if ( szAttributes.empty() )
					fprintf( file, "<Item>\n" );
				else
					fprintf( file, "<Item%s>\n", szAttributes.c_str() );
			}
			else
			{
				if ( szAttributes.empty() )
					fprintf( file, "<%s>\n", szName.c_str() );
				else
					fprintf( file, "<%s%s>\n", szName.c_str(), szAttributes.c_str() );
			}
		}
	}
}

void FinishLevel( CLevelsList &levels, FILE *file )
{
	NI_VERIFY( !levels.empty(), "Can't finish empty levels stack!", return );
	if ( !IsInRect(levels) )
	{
		PrintTabs( levels.size() - 1, file );
		if ( levels.back().szName[0] == '[' )
			fprintf( file, "</Item>\n" );
		else
			fprintf( file, "</%s>\n", levels.back().szName.c_str() );
	}
	levels.pop_back();
}

string MakeSimpleChunkName( const string &szFullName, CLevelsList &levels, FILE *file )
{
	int nLastPos = 0;
	for ( int i = NUM_SYSTEM_CHUNKS; i < levels.size(); ++i )
	{
		const int nPos = szFullName.find( '.', nLastPos );
		if ( nPos == string::npos || szFullName.compare( nLastPos, nPos - nLastPos, levels[i].szName ) != 0 )
		{
			// rollback from 'i - 1' level!
			int nNumRollback = levels.size() - i;
			while ( --nNumRollback >= 0 )
				FinishLevel( levels, file );
			break;
		}
		nLastPos = nPos + 1;
	}
	const string szChunkName = szFullName.c_str() + szFullName.rfind('.') + 1;
	return szChunkName[0] == '[' ? "Item" : szChunkName;
}

void MoveToNextLevel( const string &szFullName, const string &szType, CLevelsList &levels, FILE *file, int nArraySize )
{
	int nLastPos = 0;
	for ( int i = NUM_SYSTEM_CHUNKS; i < levels.size(); ++i )
	{
		const int nPos = szFullName.find( '.', nLastPos );
		if ( nPos == string::npos || szFullName.compare( nLastPos, nPos - nLastPos, levels[i].szName ) != 0 )
		{
			// rollback from 'i - 1' level!
			int nNumRollback = levels.size() - i;
			while ( --nNumRollback >= 0 )
				FinishLevel( levels, file );
			break;
		}
		nLastPos = nPos + 1;
	}
	string szChunkName = szFullName.c_str() + szFullName.rfind('.') + 1;
	StartLevel( szChunkName, szType, levels, file, nArraySize, 0 );
}

const string GetTypeName( const SPropertyDesc *pDesc, const string &szFullName )
{
	if ( pDesc->bArray && szFullName[szFullName.size() - 1] != ']' )
		return "vector< " + pDesc->szTypeName + " >";
	else
		return pDesc->szTypeName;
}

void CXmlExporter::ExportObjectToXML( FILE *file, const string &szTypeName, const int nClassTypeID, 
																			const string &szObjectName, const int nObjectID, const string &szFieldName )
{
	StartExport( szObjectName, szTypeName, szFieldName );
	//
	CLevelsList levels;
	levels.reserve( 100 );
	// XML header
	fprintf( file, "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n" );
	// base element with attribute -> ObjectRecordID
	{
		list< pair<string, string> > attributes;
		attributes.push_back( pair<string, string>("ObjectRecordID", StrFmt("%d", nObjectID)) );
		StartLevel( szTypeName, szTypeName, levels, file, -1, &attributes );
	}
	//
	IResourceManager *pRM = Singleton<IResourceManager>();
	if ( CObj<IManipulator> pObjMan = pRM->CreateObjectManipulator( szTypeName, szObjectName ) )
	{
		for ( CObj<IManipulatorIterator> pIt = pObjMan->Iterate(true, ECT_CACHE_GLOBAL); 
			pIt != 0 && !pIt->IsEnd(); pIt->Next() )
		{
			if ( const SPropertyDesc *pDesc = dynamic_cast<const SPropertyDesc*>(pIt->GetDesc()) )
			{
				string szFullFieldName;
				if ( pIt->GetName( &szFullFieldName ) != false && !szFullFieldName.empty() && szFullFieldName != " " )
				{
					EPCIEType ePCIType = typePCIEMnemonics.Get( pDesc, szFullFieldName );
					//
					if ( ePCIType == PCIE_UNKNOWN && pDesc->szTypeName == "hexBinary" )
						ePCIType = PCIE_BINARY_BIT_FIELD;
					//
					if ( typePCIEMnemonics.IsLeaf(ePCIType) )
					{
						const string szChunkName = MakeSimpleChunkName( szFullFieldName, levels, file );
						PrintTabs( levels.size(), file );
						//
						if ( typePCIEMnemonics.IsRef(ePCIType) )
						{
							string szRefTypeName, szRefObjName;
							if ( CManipulatorManager::GetParamsFromReference( szFullFieldName, pObjMan, &szRefTypeName, &szRefObjName, 0 ) != false )
							{
								if ( !szRefTypeName.empty() && szRefTypeName != " " && !szRefObjName.empty() && szRefObjName != " " )
								{
//									// hierarchical export
//									if ( ExportObject( szRefObjName, szRefTypeName, szFullFieldName ) != false )
									{
										string szFullPathName = MakePathName( szRefObjName, szRefTypeName, szFullFieldName );
										NStr::ToLowerASCII( &szFullPathName );
										fprintf( file, "<%s href=\"/%s#xpointer(/%s)\"/>\n", szChunkName.c_str(), szFullPathName.c_str(), szRefTypeName.c_str() );
									}
//									else
//										fprintf( file, "<%s href=\"\"/>\n", szChunkName.c_str() );
								}
								else
									fprintf( file, "<%s href=\"\"/>\n", szChunkName.c_str() );
							}
							else
								fprintf( file, "<%s href=\"\"/>\n", szChunkName.c_str() );
						}
						else 
						{
							CVariant value;
							string szValue;
							if ( pObjMan->GetValue( szFullFieldName, &value ) != false )
							{
								VariantToString( &szValue, value, ePCIType, true );
								if ( szValue.empty() )
									fprintf( file, "<%s />\n", szChunkName.c_str() );
								else
								{
									if ( szTypeName == "Text" && szFullFieldName == "Text" )
									{
										string szRawValue;
										VariantToString( &szRawValue, value, ePCIType, false );
										wstring wszUnicodeValue;
										NStr::ToUnicode( &wszUnicodeValue, szRawValue );
										// export text to text file
										const string szFullName = MakePathName( szObjectName, szTypeName, szFieldName );
										string szFullFileName = "c:\\b2\\xml\\" + szFullName;
										szFullFileName = szFullFileName.substr( 0, szFullFileName.rfind('.') ) + ".txt";
										NStr::ReplaceAllChars( &szFullFileName, '/', '\\' );
										//
										if ( FILE *txtfile = fopen( szFullFileName.c_str(), "wb" ) )
										{
											WORD wUnicodeMagic = 0xfeff;
											fwrite( &wUnicodeMagic, sizeof(WORD), 1, txtfile );
											if ( !wszUnicodeValue.empty() )
												fwrite( wszUnicodeValue.c_str(), sizeof(WORD), wszUnicodeValue.size(), txtfile );
											fclose( txtfile );
										}
									}
									else if ( szTypeName == "Script" && szFullFieldName == "ScriptText" )
									{
										string szRawValue;
										VariantToString( &szRawValue, value, ePCIType, false );
										const string szFullName = MakePathName( szObjectName, szTypeName, szFieldName );
										string szFullFileName = "c:\\b2\\xml\\" + szFullName;
										szFullFileName = szFullFileName.substr( 0, szFullFileName.rfind('.') ) + ".lua";
										NStr::ReplaceAllChars( &szFullFileName, '/', '\\' );
										//
										if ( FILE *txtfile = fopen( szFullFileName.c_str(), "wb" ) )
										{
											if ( !szRawValue.empty() )
												fwrite( szRawValue.c_str(), sizeof(char), szRawValue.size(), txtfile );
											fclose( txtfile );
										}
									}
									// convert MBCS string to UTF-8 one
									if ( (pDesc->szTypeRename == "wstring") || 
										   (szTypeName == "Text" && szFullFieldName == "Original") )
									{
										wstring wszUnicodeValue;
										NStr::ToUnicode( &wszUnicodeValue, szValue );
										NStr::UnicodeToUTF8( &szValue, wszUnicodeValue );
									}
									// convert type-rename int and DWORD to int value before saving
									if ( pDesc->szTypeRename == "int" || pDesc->szTypeRename == "DWORD" )
									{
										NI_ASSERT( value.GetType() == CVariant::VT_POINTER && value.GetBlobSize() == 4, StrFmt("Can't convert type %d to %s", value.GetType(), pDesc->szTypeRename.c_str()) );
										if ( value.GetType() == CVariant::VT_POINTER && value.GetBlobSize() == 4 )
										{
											int nIntValue = *( (int*)value.GetPtr() );
											VariantToString( &szValue, CVariant(nIntValue), ePCIType, true );
										}
									}
									// special case - convert int value of VecColor to CVec3 {x, y, z}
									if ( GetTypeName(pDesc, szFullFieldName) == "VecColor" )
									{
										const int nData = value.GetType() == CVariant::VT_NULL ? 0 : value;
										CVec3 vColor;
										vColor.r = float( ( nData >> 16 ) & 0x000000ff ) / 255.0f;
										vColor.g = float( ( nData >> 8 ) & 0x000000ff ) / 255.0f;
										vColor.b = float( nData & 0x000000ff ) / 255.0f;
										//
										StartLevel( szChunkName, "Vec3", levels, file, -1, 0 );

										PrintTabs( levels.size(), file );
										fprintf( file, "\t<x>%g</x>\n", vColor.x );
										PrintTabs( levels.size(), file );
										fprintf( file, "\t<y>%g</y>\n", vColor.y );
										PrintTabs( levels.size(), file );
										fprintf( file, "\t<z>%g</z>\n", vColor.z );

										FinishLevel( levels, file );
									}
									else if ( IsInRect(levels) )
									{
										if ( szFullFieldName.compare(szFullFieldName.size() - 9, 9, "LeftTop.x") == 0 )
											fprintf( file, "<x1>%s</x1>\n", szValue.c_str() );
										else if ( szFullFieldName.compare(szFullFieldName.size() - 9, 9, "LeftTop.y") == 0 )
											fprintf( file, "<y1>%s</y1>\n", szValue.c_str() );
										else if ( szFullFieldName.compare(szFullFieldName.size() - 13, 13, "RightBottom.x") == 0 )
											fprintf( file, "<x2>%s</x2>\n", szValue.c_str() );
										else if ( szFullFieldName.compare(szFullFieldName.size() - 13, 13, "RightBottom.y") == 0 )
											fprintf( file, "<y2>%s</y2>\n", szValue.c_str() );
										else
										{
											NI_ASSERT( false, StrFmt("Unknown field \"%s\" for rect export!", szFullFieldName.c_str()) );
										}
									}
									else
										fprintf( file, "<%s>%s</%s>\n", szChunkName.c_str(), szValue.c_str(), szChunkName.c_str() );
								}
							}
							else
								fprintf( file, "<%s />\n", szChunkName.c_str() );
						}
					}
					else
					{
						if ( ePCIType == PCIE_LIST )
						{
							CVariant value;
							if ( pObjMan->GetValue( szFullFieldName, &value ) != false )
								MoveToNextLevel( szFullFieldName, GetTypeName(pDesc, szFullFieldName), levels, file, (int)value );
							else
								MoveToNextLevel( szFullFieldName, GetTypeName(pDesc, szFullFieldName), levels, file, -1 );
						}
						else
							MoveToNextLevel( szFullFieldName, GetTypeName(pDesc, szFullFieldName), levels, file, -1 );
					}
				}
			}
		}
	}
	//
	while ( !levels.empty() )
		FinishLevel( levels, file );
	//
	FinishExport();
}

bool CXmlExporter::ExportObject( const string &szObjectName, const string &szClassTypeName, const string &szFieldName )
{
	const string szFullName = MakePathName( szObjectName, szClassTypeName, szFieldName );
	if ( szFullName.empty() )
		return false;
	if ( exportedObjects.find( szFullName ) != exportedObjects.end() )
		return true;
	else
	{
		IResourceManager *pRM = Singleton<IResourceManager>();
		if ( CObj<IManipulator> pTableMan = pRM->CreateTableManipulator() )
		{
			const int nClassTypeID = pTableMan->GetID( szClassTypeName );
			//
			string szFullFileName = "c:\\b2\\xml\\" + szFullName;
			NStr::ReplaceAllChars( &szFullFileName, '/', '\\' );
			NFile::CreatePath( szFullFileName.substr(0, szFullFileName.rfind('\\')) );
			if ( FILE *file = fopen(szFullFileName.c_str(), "wt") )
			{
				if ( CPtr<IManipulator> pFolderMan = pRM->CreateFolderManipulator( szClassTypeName ) )
				{
					exportedObjects[szFullName] = true;
					const int nObjectID = pFolderMan->GetID( szObjectName );
					ExportObjectToXML( file, szClassTypeName, nClassTypeID, szObjectName, nObjectID, szFieldName );
					fclose( file );
					return true;
				}
				else
				{
					fclose( file );
					return false;
				}
			}
		}
		//
		return false;
	}
}

void CXmlExporter::StartExport( const string &szObjectName, const string &szClassTypeName, const string &szFieldName )
{
	CObjectsStack::iterator pos = objectsStack.insert( objectsStack.end(), SObjStackEntry() );
	pos->szClassTypeName = szClassTypeName;
	pos->szObjectName = szObjectName;
	pos->szFieldName = szFieldName;
}

void CXmlExporter::FinishExport()
{
	objectsStack.pop_back();
}

static void DumpTypeNames()
{
	IResourceManager *pRM = Singleton<IResourceManager>();
	if ( CObj<IManipulator> pTableMan = pRM->CreateTableManipulator() )
	{
		vector<string> typeNames;
		for ( CObj<IManipulatorIterator> pItTable = pTableMan->Iterate(true, ECT_CACHE_GLOBAL); 
			pItTable != 0 && !pItTable->IsEnd(); pItTable->Next() )
		{
			string szTypeName;
			if ( pItTable->GetName( &szTypeName ) != false && !szTypeName.empty() && szTypeName != " " )
			{
				if ( szTypeName.size() > 7 && szTypeName.compare(szTypeName.size() - 7, 7, "Builder") == 0 )
					continue;
				if ( szTypeName.size() > 6 && szTypeName.compare(szTypeName.size() - 6, 6, "Copier") == 0 )
					continue;
				typeNames.push_back( szTypeName );
			}
		}
		//
		sort( typeNames.begin(), typeNames.end() );
		if ( FILE *fout = fopen("c:\\b2\\TypeNames.txt", "wt") )
		{
			for ( int i = 0; i < typeNames.size(); ++i )
				fprintf( fout, "{ \"%s\" },\n", typeNames[i].c_str() );
			fclose( fout );
		}
	}
}

static const char *s_pszTypeNames[] = 
{
	{ "AIExpLevel" },
	{ "AIGameConsts" },
	{ "AIGeometry" },
	{ "ARRemoveGlobalVar" },
	{ "ARSendGameMessage" },
	{ "ARSendUIMessage" },
	{ "ARSetForcedAction" },
	{ "ARSetGlobalVar" },
	{ "ARSetSpecialAbility" },
	{ "ARSwitchTab" },
	{ "AckSetRPGStats" },
	{ "ActionButtonInfo" },
	{ "AddReinforcementCalls" },
	{ "AmbientLight" },
	{ "AnimB2" },
	{ "AnimLight" },
	{ "ArmorPattern" },
	{ "ArmorPatternPlacement" },
	{ "AttachedModelVisObj" },
	{ "BackgroundFrameSequence" },
	{ "BackgroundSimpleScallingTexture" },
	{ "BackgroundSimpleTexture" },
	{ "BackgroundTiledTexture" },
	{ "BridgeRPGStats" },
	{ "BuildingRPGStats" },
	{ "BurningFuel" },
	{ "CameraLimits" },
	{ "Campaign" },
	{ "Chapter" },
	{ "ChapterBonus" },
	{ "CheckIsTabActive" },
	{ "CheckIsWindowEnabled" },
	{ "CheckIsWindowVisible" },
	{ "CheckPreprogrammed" },
	{ "CheckRunScript" },
	{ "ClientGameConsts" },
	{ "CoastDesc" },
	{ "ComplexEffect" },
	{ "ComplexSeasonedEffect" },
	{ "ComplexSoundDesc" },
	{ "ComplexStaticObject" },
	{ "Composition" },
	{ "CragDesc" },
	{ "CraterSet" },
	{ "CubeTexture" },
	{ "DBConstructorProfile" },
	{ "Decal" },
	{ "DeployTemplate" },
	{ "DepthOfField" },
	{ "DifficultyLevel" },
	{ "DirectionRange" },
	{ "DistanceFog" },
	{ "DistanceRange" },
	{ "DynamicDebrisSet" },
	{ "EditorOptions" },
	{ "EditorTest" },
	{ "Effect" },
	{ "EntrenchmentRPGStats" },
	{ "Fade" },
	{ "FenceRPGStats" },
	{ "Field" },
	{ "FlashEffect" },
	{ "Font" },
	{ "ForegroundTextString" },
	{ "ForegroundTextStringShared" },
	{ "GameConsts" },
	{ "Geometry" },
	{ "HeightFog" },
	{ "HeightRange" },
	{ "IconsSet" },
	{ "InfantryRPGStats" },
	{ "LakeDesc" },
	{ "LightInstance" },
	{ "M1UnitActionBuild" },
	{ "M1UnitActionTransform" },
	{ "M1UnitActions" },
	{ "M1UnitHelicopter" },
	{ "M1UnitStatsModifier" },
	{ "M1UnitType" },
	{ "ManuverDescriptor" },
//	{ "MapClip" },
	{ "MapInfo" },
	{ "MapMusic" },
	{ "Material" },
	{ "MechUnitRPGStats" },
	{ "Medal" },
	{ "MessageReactionComplex" },
	{ "MineRPGStats" },
	{ "Minimap" },
	{ "Mission" },
	{ "MissionObjective" },
	{ "Model" },
	{ "ModelInstance" },
	{ "MultiplayerConsts" },
	{ "MusicTrack" },
	{ "NetGameConsts" },
	{ "Notification" },
	{ "NotificationEvent" },
	{ "ObjectRPGStats" },
	{ "OptionSystem" },
	{ "Particle" },
	{ "ParticleInstance" },
	{ "PartyDependentInfo" },
	{ "PlayList" },
	{ "PlayTime" },
	{ "PlayerRank" },
	{ "PreLight" },
	{ "Projectile" },
	{ "Reinforcement" },
	{ "ReinforcementChange" },
	{ "ReinforcementDisable" },
	{ "ReinforcementEnable" },
	{ "ReinforcementTypes" },
	{ "RiverDesc" },
	{ "RoadDesc" },
	{ "SceneConsts" },
	{ "Script" },
	{ "Skeleton" },
	{ "SoundDesc" },
	{ "SpeedRange" },
	{ "Spot" },
	{ "SquadRPGStats" },
	{ "StaticDebrisSet" },
	{ "SunFlares" },
	{ "TGNoise" },
	{ "TGTerraSet" },
	{ "TGTerraType" },
	{ "TerraObjSetRPGStats" },
	{ "TerrainSpotDesc" },
	{ "Text" },
	{ "TextEntry" },
	{ "TextFormat" },
	{ "Texture" },
	{ "TooltipContext" },
	{ "TwoSidedLight" },
	{ "UIConsoleCommand" },
	{ "UIConstsB2" },
	{ "UISB2Move" },
	{ "UISB2MoveShared" },
	{ "UISButtonSubstate" },
	{ "UISDirectRunReaction" },
	{ "UISMoveTo" },
	{ "UISPlaySound" },
	{ "UISRunReaction" },
	{ "UISSendUIMessage" },
	{ "UnitActions" },
	{ "UnitSpecialAblityDesc" },
	{ "UnitStatsModifier" },
	{ "VisObj" },
	{ "VisObjIconsSet" },
	{ "Voice" },
	{ "Water" },
	{ "WaterSet" },
	{ "WeaponRPGStats" },
	{ "WeatherDesc" },
	{ "Window1LvlTreeControl" },
	{ "Window1LvlTreeControlShared" },
	{ "Window3DControl" },
	{ "Window3DControlShared" },
	{ "WindowComboBox" },
	{ "WindowComboBoxShared" },
	{ "WindowConsole" },
	{ "WindowConsoleOutput" },
	{ "WindowConsoleOutputShared" },
	{ "WindowConsoleShared" },
	{ "WindowEditLine" },
	{ "WindowEditLineShared" },
	{ "WindowFrameSequence" },
	{ "WindowFrameSequenceShared" },
	{ "WindowListCtrl" },
	{ "WindowListHeader" },
	{ "WindowListHeaderShared" },
	{ "WindowListItem" },
	{ "WindowListItemShared" },
	{ "WindowListSharedData" },
	{ "WindowMSButton" },
	{ "WindowMSButtonShared" },
	{ "WindowMiniMap" },
	{ "WindowMiniMapShared" },
	{ "WindowMultiTextureProgressBar" },
	{ "WindowMultiTextureProgressBarShared" },
	{ "WindowPlayer" },
	{ "WindowPlayerShared" },
	{ "WindowPotentialLines" },
	{ "WindowPotentialLinesShared" },
	{ "WindowProgressBar" },
	{ "WindowProgressBarShared" },
	{ "WindowRoundProgressBar" },
	{ "WindowRoundProgressBarShared" },
	{ "WindowScreen" },
	{ "WindowScreenShared" },
	{ "WindowScrollBar" },
	{ "WindowScrollBarShared" },
	{ "WindowScrollableContainer" },
	{ "WindowScrollableContainerShared" },
	{ "WindowSelection" },
	{ "WindowSelectionShared" },
	{ "WindowSimple" },
	{ "WindowSimpleShared" },
	{ "WindowSlider" },
	{ "WindowSliderShared" },
	{ "WindowStatsSystem" },
	{ "WindowStatsSystemShared" },
	{ "WindowTabControl" },
	{ "WindowTabControlShared" },
	{ "WindowTextView" },
	{ "WindowTextViewShared" },
	{ "WindowTooltip" },
	{ "WindowTooltipShared" },
	{ "GameRoot" },
	{ 0 }
};

void DumpAllObjects()
{
	IResourceManager *pRM = Singleton<IResourceManager>();
	CObj<IManipulator> pTableMan = pRM->CreateTableManipulator();
	if ( pTableMan == 0 )
		return;
	int nStartPosition = 0;
	const char *pszLastExportedTypeName = 0;
	// continue export after failure - find start position
	if ( pszLastExportedTypeName != 0 )
	{
		for ( int i = 0; s_pszTypeNames[i] != 0; ++i )
		{
			const string szTypeName = s_pszTypeNames[i];
			if ( szTypeName == pszLastExportedTypeName )
			{
				nStartPosition = i + 1;
				DebugTrace( "***** =====> Starting export from \"%s\" (position %d) <===== *****", s_pszTypeNames[nStartPosition], nStartPosition );
				break;
			}
		}
		NI_VERIFY( nStartPosition < ARRAY_SIZE(s_pszTypeNames), "Failed to find start position for data export!", return );
	}
	// export
	for ( int i = nStartPosition; s_pszTypeNames[i] != 0; ++i )
	{
		const string szTypeName = s_pszTypeNames[i];
		const int nClassTypeID = pTableMan->GetID( szTypeName );
		//
		if ( CObj<IManipulator> pFolderMan = pRM->CreateFolderManipulator(szTypeName) )
		{
			for ( CObj<IManipulatorIterator> pItFolder = pFolderMan->Iterate(true, ECT_CACHE_GLOBAL); 
				pItFolder != 0 && !pItFolder->IsEnd(); pItFolder->Next() )
			{
				string szObjectName;
				if ( !pItFolder->IsFolder() && pItFolder->GetName( &szObjectName ) != false && 
					!szObjectName.empty() && szObjectName != " " )
				{
					GetExporter()->ExportObject( szObjectName, szTypeName, "" );
				}
			}
		}
		DebugTrace( "***** =====> \"%s\"", szTypeName.c_str() );
	}
}

}

