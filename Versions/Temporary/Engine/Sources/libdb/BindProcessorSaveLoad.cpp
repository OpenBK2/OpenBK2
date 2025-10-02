#include "StdAfx.h"
#include "BindProcessor.h"
#include "BindArray.h"
#include "Bind.h"
#include "EditorDb.h"
#include "../System/LightXML.h"
#include "../System/XmlReader.h"
#include "../System/XmlUtils.h"
#include "../System/FilePath.h"

namespace NDb
{
namespace NBind
{

// ************************************************************************************************************************ //
// **
// ** XML file loading with meta-information
// **
// **
// **
// ************************************************************************************************************************ //

static bool LoadRefFromNode( CVariant *pRes, const NXml::CXmlNode *pNode, const CDBID &dbidParent )
{
	if ( const NXml::SXmlAttribute *pAttribute = pNode->GetHRefAttribute() )
	{
		const string szAttrVal = pAttribute->value.ToString();
		string szResult;
		string szString = szAttrVal.substr( 0, szAttrVal.rfind( '#' ) );
		NStr::UTF8ToMBCS( &szString, szString );
		NFile::MakeFullPath( &szResult, szString, GetFileName(dbidParent) );
		NFile::NormalizePath( &szResult );
		*pRes = CDBID( szResult );
		return true;
	}
	*pRes = CVariant();
	return false;
}

static void LoadFilePathFromNode( CVariant *pRes, const NXml::CXmlNode *pNode, const CDBID &dbidParent )
{
	if ( const NXml::SXmlAttribute *pAttribute = pNode->GetHRefAttribute() )
	{
		string szResult;
		string szString = pAttribute->value.ToString();
		NXml::ConvertToString( &szString );
		NStr::UTF8ToMBCS( &szString, szString );
		NFile::MakeFullPath( &szResult, szString, GetFileName(dbidParent) );
		NFile::NormalizePath( &szResult );
		*pRes = szResult;
	}
	else
	{
		string szTemp;
		string szValue = pNode->GetValue().ToString();
		NXml::ConvertToString( &szValue );
		NStr::UTF8ToMBCS( &szTemp, szValue );
		*pRes = szTemp;
	}
}

bool LoadSimpleValueFromNode( CVariant *pRes, const NXml::CXmlNode *pNode, 
														  const NTypeDef::STypeStructBase::SField &field, 
															const string &szFullFieldName, IObjMan *pParent )
{
	if ( field.pType->eType == NTypeDef::TYPE_TYPE_REF )
		LoadRefFromNode( pRes, pNode, pParent->GetDBID() );
	else if ( field.pType->eType == NTypeDef::TYPE_TYPE_STRING && field.HasAttribute("filepath") )
		LoadFilePathFromNode( pRes, pNode, pParent->GetDBID() );
	else
	{
		string szValue = pNode->GetValue().ToString();
		if ( field.pType->eType == NTypeDef::TYPE_TYPE_STRING || field.pType->eType == NTypeDef::TYPE_TYPE_WSTRING )
			NXml::ConvertToString( &szValue );

		field.pType->FromString( pRes, szValue );
		NI_VERIFY( pRes->GetType() != CVariant::VT_NULL, StrFmt("Can't convert text to value for \"%s\"", szFullFieldName.c_str()), return false );
	}
	return true;
}

bool SBindProcessor::LoadXML( const string &szAddName, NTypeDef::STypeStructBase *pType, const NXml::CXmlNode *pBaseNode, IObjMan *pParent )
{
	if ( pType->pBaseType )
	{
		NI_VERIFY( LoadXML( szAddName, pType->pBaseType, pBaseNode, pParent ) != false, 
			("Can't load base substruct " + szAddName).c_str(), return false );
	}
	//
	for ( NTypeDef::STypeStructBase::CFieldsList::const_iterator itField = pType->fields.begin(); itField != pType->fields.end(); ++itField )
	{
		const string szFullFieldName = szAddName.empty() ? itField->szName : szAddName + "." + itField->szName;
		const NXml::CXmlNode *pNode = pBaseNode->FindChild( itField->szName.c_str() );
		//
		if ( itField->pType->IsSimpleType() )
		{
			// in the case of null XML node we must fill this field with default values!
			if ( pNode )
			{
				CVariant value;
				if ( LoadSimpleValueFromNode( &value, pNode, *itField, szFullFieldName, pParent ) == false )
					continue;
				const bool bValueSet = SetValue( szFullFieldName, value );
				NI_VERIFY( bValueSet != false, 
					StrFmt("Can't set value \"%s\" for field \"%s\"", itField->pType->ToString(value).c_str(), szFullFieldName.c_str()), continue );
			}
			else
			{
				CVariant vtDefVal = itField->GetDefaultValue();
				if ( vtDefVal.GetType() != CVariant::VT_NULL )
				{
					const bool bValueSet = SetValue( szFullFieldName, vtDefVal );
					NI_VERIFY( bValueSet != false, 
						StrFmt("Can't set value \"%s\" for field \"%s\"", itField->pType->ToString(vtDefVal).c_str(), szFullFieldName.c_str()), continue );
				}
			}
		}
		else if ( itField->pType->eType == NTypeDef::TYPE_TYPE_STRUCT )
		{
			if ( pNode )
			{
				NI_VERIFY( LoadXML( szFullFieldName, checked_cast_ptr<NTypeDef::STypeStructBase *>(itField->pType), pNode, pParent ) != false, 
					("Can't load substruct " + szFullFieldName).c_str(), continue );
			}
			else
			{
				NI_VERIFY( SetDefault( szFullFieldName, checked_cast_ptr<NTypeDef::STypeStructBase *>(itField->pType) ) != false, 
					("Can't set defaults for substruct " + szFullFieldName).c_str(), continue );
			}
		}
		else if ( itField->pType->eType == NTypeDef::TYPE_TYPE_ARRAY )
		{
			if ( pNode )
			{
				const vector<const NXml::CXmlNode*> &nodes = pNode->GetNodes();
				const int nSize = nodes.size();
				Remove( szFullFieldName, 0, -1 );
				if ( nSize > 0 )
				{
					Insert( szFullFieldName, 0, nSize );
					NTypeDef::STypeArray *pTypeArray = checked_cast_ptr<NTypeDef::STypeArray *>( itField->pType );
					if ( pTypeArray->field.pType->IsSimpleType() )
					{
						SBindProcessor::SArrayRequisites arrReqs;
						GetArrayRequisites( szFullFieldName, &arrReqs );
						CBindArray *pBindArray = GetBindArray( szFullFieldName );
						for ( int i = 0; i < nSize; ++i )
						{
							CVariant value;
							if ( LoadSimpleValueFromNode( &value, nodes[i], pTypeArray->field, szFullFieldName, pParent ) == false )
								continue;
							const bool bValueSet = pBindArray->SetValue( "", i, value, arrReqs.pRawVector, arrReqs.pContained );
							NI_VERIFY( bValueSet != false, 
								StrFmt("Can't set value \"%s\" for field \"%s.[%d]\"", arrReqs.pTypeArray->field.pType->ToString(value).c_str(), szFullFieldName.c_str(), i), continue );
						}
					}
					else if ( pTypeArray->field.pType->eType == NTypeDef::TYPE_TYPE_STRUCT )
					{
						CObj<IArrayObjMan> pMan = checked_cast<IArrayObjMan*>( pParent->CreateManipulator( szFullFieldName ) );
						ILoadableObjMan *pLoadMan = dynamic_cast_ptr<ILoadableObjMan*>( pMan );
						NI_VERIFY( pMan != 0, ("During loading, can't create manipulator for " + szFullFieldName).c_str(), continue );
						for ( int i = 0; i < nSize; ++i )
						{
							pMan->SetIndex( i );
							const NXml::CXmlNode *pArrayNode = nodes[i];
							NI_VERIFY( pLoadMan->LoadXML( "", checked_cast_ptr<NTypeDef::STypeStructBase*>(pTypeArray->field.pType), pArrayNode ) != false,
								StrFmt("Can't load %d element for %s", i, szFullFieldName.c_str()), continue );
						}
					}
				}
			}
			else
			{
				// TODO: set default array
				//				NI_ASSERT( false, "still not realized" );
			}
		}
	}
	return true;
}

static bool SaveRefToNode( NLXML::CXMLNode *pNode, const CVariant &value, const CDBID &dbidParent )
{
	string szRelPath;
	if ( value.GetType() == CVariant::VT_DBID )
	{
		const string szClassTypeName = NDb::GetClassTypeName( value.GetDBID() );
		if ( !szClassTypeName.empty() )
		{
			const string szFullFileName = GetFileName( value.GetDBID() );
			const string szParentFileName = GetFileName( dbidParent );
			NFile::MakeRelativePath( &szRelPath, szFullFileName, szParentFileName );
			NFile::NormalizePath( &szRelPath );
			NStr::MBCSToUTF8( &szRelPath, szRelPath );
			szRelPath += "#xpointer(/" + szClassTypeName + ")";
		}
	}
	checked_cast<NLXML::CXMLElement*>(pNode)->SetAttribute( "href", szRelPath );
	return true;
}

static void SaveFilePathToNode( NLXML::CXMLNode *pNode, const CVariant &value, const CDBID &dbidParent )
{
	string szRelPath, szTemp;
	NStr::MBCSToUTF8( &szTemp, value.GetStr() );
	NFile::MakeRelativePath( &szRelPath, szTemp, GetFileName(dbidParent) );
	NFile::NormalizePath( &szRelPath );
	checked_cast<NLXML::CXMLElement*>(pNode)->SetAttribute( "href", szRelPath );
}

bool SaveSimpleValueToNode( const CVariant &value, NLXML::CXMLElement *pElement, 
													  const NTypeDef::STypeStructBase::SField &field, IObjMan *pParent )
{
	if ( field.pType->eType == NTypeDef::TYPE_TYPE_REF )
		SaveRefToNode( pElement, value, pParent->GetDBID() );
	else if ( field.pType->eType == NTypeDef::TYPE_TYPE_STRING && field.HasAttribute("filepath") )
		SaveFilePathToNode( pElement, value, pParent->GetDBID() );
	else
	{
		string szRes;
		field.pType->ToString( &szRes, value );
		pElement->SetText( szRes );
	}
	return true;
}

bool SBindProcessor::SaveXML( const string &szAddName, NTypeDef::STypeStructBase *pType, NLXML::CXMLNode *pBaseNode, IObjMan *pParent )
{
	if ( pType->pBaseType )
	{
		NI_VERIFY( SaveXML( szAddName, pType->pBaseType, pBaseNode, pParent ) != false, 
			("Can't load base substruct " + szAddName).c_str(), return false );
	}
	//
	for ( NTypeDef::STypeStructBase::CFieldsList::const_iterator itField = pType->fields.begin(); itField != pType->fields.end(); ++itField )
	{
		const string szFullFieldName = szAddName.empty() ? itField->szName : szAddName + "." + itField->szName;
		NLXML::CXMLElement *pElement = new NLXML::CXMLElement();
		checked_cast<NLXML::CXMLElement*>(pBaseNode)->AddChild( pElement );
		pElement->SetValue( itField->szName );
		//
		if ( itField->pType->IsSimpleType() )
		{
			CVariant value;
			NI_VERIFY( GetValue( szFullFieldName, &value ) != false,
				("Can't get field value for " + szFullFieldName).c_str(), continue );
			if ( SaveSimpleValueToNode( value, pElement, *itField, pParent ) == false )
				continue;
		}
		else if ( itField->pType->eType == NTypeDef::TYPE_TYPE_STRUCT )
		{
			NI_VERIFY( SaveXML( szFullFieldName, checked_cast_ptr<NTypeDef::STypeStructBase *>(itField->pType), pElement, pParent ) != false, 
				("Can't save substruct " + szFullFieldName).c_str(), continue );
		}
		else if ( itField->pType->eType == NTypeDef::TYPE_TYPE_ARRAY )
		{
			CVariant value;
			NI_VERIFY( GetValue( szFullFieldName, &value ) != false, 
				("Can't get array size for " + szFullFieldName).c_str(), continue );
			const int nArraySize = (int)value;
			NTypeDef::STypeArray *pTypeArray = checked_cast_ptr<NTypeDef::STypeArray *>( itField->pType );
			if ( pTypeArray->field.pType->IsSimpleType() )
			{
				SBindProcessor::SArrayRequisites arrReqs;
				GetArrayRequisites( szFullFieldName, &arrReqs );
				CBindArray *pBindArray = GetBindArray( szFullFieldName );
				//
				for ( int i = 0; i < nArraySize; ++i )
				{
					NLXML::CXMLElement *pItem = new NLXML::CXMLElement();
					checked_cast<NLXML::CXMLElement*>(pElement)->AddChild( pItem );
					pItem->SetValue( "Item" );
					//
					pBindArray->GetValue( "", i, &value, arrReqs.pRawVector, arrReqs.pContained );
					if ( SaveSimpleValueToNode( value, pItem, arrReqs.pTypeArray->field, pParent ) == false )
						continue;
				}
			}
			else
			{
				CObj<IArrayObjMan> pMan = checked_cast<IArrayObjMan*>( pParent->CreateManipulator( szFullFieldName ) );
				NI_VERIFY( pMan != 0, ("During saving, can't create manipulator for " + szFullFieldName).c_str(), continue );
				ILoadableObjMan *pSaveMan = dynamic_cast_ptr<ILoadableObjMan*>( pMan );
				for ( int i = 0; i < nArraySize; ++i )
				{
					NLXML::CXMLElement *pItem = new NLXML::CXMLElement();
					checked_cast<NLXML::CXMLElement*>(pElement)->AddChild( pItem );
					pItem->SetValue( "Item" );
					//
					pMan->SetIndex( i );
					NI_VERIFY( pSaveMan->SaveXML( "", checked_cast_ptr<NTypeDef::STypeStructBase*>(pTypeArray->field.pType), pItem ) != false,
						StrFmt("Can't save %d element for %s", i, szFullFieldName.c_str()), continue );
				}
			}
		}
	}
	//
	return true;
}

bool SBindProcessor::SetDefault( const string &szAddName, NTypeDef::STypeStructBase *pType )
{
	if ( pType->pBaseType )
	{
		NI_VERIFY( SetDefault( szAddName, pType->pBaseType ) != false, 
			("Can't set default for base substruct " + szAddName).c_str(), return false );
	}
	//
	for ( NTypeDef::STypeStructBase::CFieldsList::const_iterator itField = pType->fields.begin(); itField != pType->fields.end(); ++itField )
	{
		const string szFullFieldName = szAddName.empty() ? itField->szName : szAddName + "." + itField->szName;
		if ( itField->pType->IsSimpleType() )
		{
			NI_VERIFY( SetValue( szFullFieldName, itField->GetDefaultValue() ) != false,
				("Can't set default value for " + szFullFieldName).c_str(), continue );
		}
		else if ( itField->pType->eType == NTypeDef::TYPE_TYPE_STRUCT )
		{
			NI_VERIFY( SetDefault( szFullFieldName, checked_cast_ptr<NTypeDef::STypeStructBase *>(itField->pType) ) != false, 
				("Can't set default value for substruct " + szFullFieldName).c_str(), continue );
		}
		else if ( itField->pType->eType == NTypeDef::TYPE_TYPE_ARRAY )
		{
			for ( vector< CObj<NTypeDef::SConstraints> >::const_iterator itConstraint = itField->constraints.begin(); 
				itConstraint != itField->constraints.end(); ++itConstraint )
			{
				if ( const NTypeDef::SConstraintsArrayMinMax *pConstraints = dynamic_cast_ptr<const NTypeDef::SConstraintsArrayMinMax *>(*itConstraint) )
				{
					SetValue( szFullFieldName, CVariant(pConstraints->nMinElements) );
					break;
				}
			}
		}
	}
	return true;
}

}
}
