#include "stdafx.h"

#include "ObjectManipulator.h"
#include "Misc/HashFuncs.h"
#include "Bind.h"
#include "EditorDb.h"

using namespace NDb::NTypeDef;

const char *ConvertEditorType2String( EEditorType eEditorType )
{
	switch ( eEditorType )
	{
	case EDITOR_TYPE_INT_INPUT:
		return "int_input";
	case EDITOR_TYPE_INT_SLIDER:
		return "int_slider";
	case EDITOR_TYPE_INT_COMBO:
		return "int_combo";
	case EDITOR_TYPE_INT_COLOR:
		return "int_color";
	case EDITOR_TYPE_INT_COLOR_WITH_ALPHA:
		return "int_color_with_alpha";
	case EDITOR_TYPE_FLOAT_INPUT:
		return "float_input";
	case EDITOR_TYPE_FLOAT_SLIDER:
		return "float_slider";
	case EDITOR_TYPE_FLOAT_COMBO:
		return "float_combo";
	case EDITOR_TYPE_BOOL_COMBO:
		return "bool_combo";
	case EDITOR_TYPE_BOOL_CHECKBOX:
		return "bool_checkbox";
	case EDITOR_TYPE_BOOL_SWITCHER:
		return "bool_switcher";
	case EDITOR_TYPE_GUID:
		return "guid";
	case EDITOR_TYPE_STRING_REF:
		return "string_ref";
	case EDITOR_TYPE_STRING_MULTI_REF:
		return "string_multi_ref";
	case EDITOR_TYPE_STRING_INPUT:
		return "string_input";
	case EDITOR_TYPE_STRING_BIG_INPUT:
		return "string_big_input";
	case EDITOR_TYPE_STRING_COMBO:
		return "string_combo";
	case EDITOR_TYPE_STRING_COMBO_REF:
		return "string_combo_ref";
	case EDITOR_TYPE_STRING_COMBO_MULTI_REF:
		return "string_combo_multi_ref";
	case EDITOR_TYPE_STRING_FILE_REF:
		return "string_file_ref";
	case EDITOR_TYPE_STRING_DIR_REF:
		return "string_dir_ref";
	case EDITOR_TYPE_BIT_FIELD:
		return "bit_field";
	case EDITOR_TYPE_STRING_NEW_REF:
		return "string_new_ref";
	case EDITOR_TYPE_STRING_NEW_MULTI_REF:
		return "string_new_multi_ref";
	}
	return "";
}

static bool FillPropertyDescFromField( SPropertyDesc *pDesc, const STypeStructBase::SField &field )
{
	if ( field.pType == 0 )
		return false;
	//
	if ( !field.wszDesc.empty() )
		NStr::ToMBCS( &pDesc->szDesc, field.wszDesc );
	if ( field.defaultValue.GetType() != CVariant::VT_NULL )
		field.pType->ToString( &pDesc->szDefault, field.defaultValue );
	pDesc->szTypeName = field.pType->GetTypeName();
	if ( field.pType->eType == TYPE_TYPE_ENUM )
	{
		const STypeEnum *pEnum = checked_cast_ptr<const STypeEnum *>( field.pType );
		for ( int i = 0; i < pEnum->entries.size(); ++i )
			pDesc->values.push_back( pEnum->entries[i].szName );
		pDesc->szEnumName = pEnum->szTypeName;
	}
	//
	pDesc->nSize = field.pType->GetTypeSize();
	//
	pDesc->bArray = field.pType->eType == TYPE_TYPE_ARRAY;
	pDesc->bStruct = field.pType->eType == TYPE_TYPE_STRUCT;
	//
	pDesc->bHidden = field.HasAttribute( "hidden" );
	pDesc->bReadOnly = field.HasAttribute( "readonly" ) || field.HasAttribute( "readOnly" );
	pDesc->bNoCode = field.HasAttribute( "nocode" ) || field.HasAttribute( "noCode" );
	pDesc->bNoHeader = field.HasAttribute( "noheader" ) || field.HasAttribute( "noHeader" );
	pDesc->bUseUpperType = field.HasAttribute( "useuppertype" ) || field.HasAttribute( "useUpperType" );
	pDesc->bNoBase = false;
	pDesc->bUnsafe = field.HasAttribute( "unsafe" );
	//
	if ( const CVariant *pVal = field.GetAttribute( "editorControl" ) )
		pDesc->szPropControlType = pVal->GetStr();
	if ( const CVariant *pVal = field.GetAttribute( "stringParam" ) )
		pDesc->szStringParam = pVal->GetStr();
	if ( const CVariant *pVal = field.GetAttribute( "intParam" ) )
		pDesc->nIntParam = *pVal;
	if ( pDesc->szPropControlType.empty() )
		pDesc->szPropControlType = ConvertEditorType2String( field.GetEditorType() );
	// array must have the same editor type as contained element :(
	if ( field.pType->eType == TYPE_TYPE_ARRAY )
	{
		if ( const STypeArray *pArray = checked_cast_ptr<const STypeArray *>( field.pType ) )
		{
			if ( const CVariant *pVal = pArray->field.GetAttribute( "editorControl" ) )
				pDesc->szPropControlType = pVal->GetStr();
			if ( const CVariant *pVal = pArray->field.GetAttribute( "stringParam" ) )
				pDesc->szStringParam = pVal->GetStr();
			if ( const CVariant *pVal = pArray->field.GetAttribute( "intParam" ) )
				pDesc->nIntParam = *pVal;
			if ( pDesc->szPropControlType.empty() )
				pDesc->szPropControlType = ConvertEditorType2String( pArray->field.GetEditorType() );
		}
	}
	//
	if ( field.pType->eType == TYPE_TYPE_REF )
	{
		const STypeRef *pTypeRef = checked_cast_ptr<const STypeRef *>( field.pType );
		std::vector<const STypeClass *> classes;
		pTypeRef->GetRefTypesList( &classes );
		for ( int i = 0; i < classes.size(); ++i )
			pDesc->refTypes[classes[i]->szTypeName] = classes[i]->nClassTypeID;
	}
	//
	return true;
}

UINT CObjectManipulatorWrapper::GetID( const std::string &rszName ) const
{
	return INVALID_NODE_ID;
}

CDBID CObjectManipulatorWrapper::GetDBID() const
{
	return pObjMan->GetDBID();
}

bool CObjectManipulatorWrapper::GetType( const std::string &rszName, std::string *pszType ) const
{
	if ( pszType == 0 )
		return false;
	if ( rszName.empty() )
	{
		*pszType = dynamic_cast_ptr<const NDb::NBind::CBindStruct *>( pObjMan )->GetTypeName();
		return true;
	}
	else if ( const STypeStructBase::SField *pField = pObjMan->GetDesc( rszName ) )
	{
		*pszType = pField->pType->GetTypeName();
		return true;
	}
	return false;
}

IManipulatorIterator *CObjectManipulatorWrapper::Iterate( bool bShowHidden, ECacheType eCache )
{
	return new CObjectManipulatorIteratorWrapper( pObjMan->CreateIterator(bShowHidden) );
}

typedef std::unordered_map<void *, SPropertyDesc, SDefaultPtrHash> CPropertyDescMap;
static CPropertyDescMap s_propertyDescMap;
const SPropertyDesc *GetPropertyDesc( const STypeStructBase::SField *pField )
{
	CPropertyDescMap::const_iterator pos = s_propertyDescMap.find( (void*)pField );
	if ( pos != s_propertyDescMap.end() )
		return &( pos->second );
	SPropertyDesc *pDesc = &( s_propertyDescMap[(void*)pField] );
	FillPropertyDescFromField( pDesc, *pField );
	return pDesc;
}

const SIteratorDesc *CObjectManipulatorWrapper::GetDesc( const std::string &szName ) const
{
	const STypeStructBase::SField *pField = pObjMan->GetDesc( szName );
	if ( pField == 0 )
		return 0;
	return GetPropertyDesc( pField );
}

bool CObjectManipulatorWrapper::GetValue( const std::string &szName, CVariant *pValue ) const
{
	if ( pObjMan->GetValue( szName, pValue ) != false )
	{
		// CRAP{ recode slashes to backslashes
		if ( pValue->GetType() == CVariant::VT_DBID )
		{
			const CDBID &dbid = pValue->GetDBID().ToString();
			std::string szString = pValue->GetDBID().ToString();
			NStr::ReplaceAllChars( &szString, '/', '\\' );
			*pValue = CDBID(szString);
		}
		// CRAP}
		return true;
	}
	else
		return false;
}

bool CObjectManipulatorWrapper::SetValue( const std::string &szName, const CVariant &value )
{
	return pObjMan->SetValue( szName, value );
}

bool CObjectManipulatorWrapper::CheckValue( const std::string &szName, const CVariant &value, bool *pResult ) const
{
	*pResult = true;
	return true;
}

bool CObjectManipulatorWrapper::InsertNode( const std::string &szName, int nNodeIndex )
{
	return pObjMan->Insert( szName, nNodeIndex, 1, true );
}

bool CObjectManipulatorWrapper::RemoveNode( const std::string &szName, int nNodeIndex )
{
	if ( nNodeIndex == NODE_REMOVEALL_INDEX )
		return pObjMan->Remove( szName, 0, -1 );
	else
		return pObjMan->Remove( szName, nNodeIndex, 1 );
}

bool CObjectManipulatorWrapper::RemoveNodeByID( const std::string &szName, int nNodeID )
{
	NI_ASSERT( false, "Feature RemoveNodeByID() can't be realized in this wrapper!" );
	return false;
}

bool CObjectManipulatorWrapper::IsNameExists( const std::string &rszName ) const
{
	return pObjMan->GetDesc( rszName ) != 0;
}


void CObjectManipulatorWrapper::ClearCache()
{
}

bool CObjectManipulatorWrapper::GetName( UINT nID, std::string *pszName ) const
{ 
	if ( nID != -1 )
		return false;
	*pszName = pObjMan->GetDBID().ToString();
	return true; 
}
// ************************************************************************************************************************ //
// **
// ** iterator
// **
// **
// **
// ************************************************************************************************************************ //

const SIteratorDesc* CObjectManipulatorIteratorWrapper::GetDesc() const
{
	const STypeStructBase::SField *pField = pIterator->GetDesc();
	if ( pField == 0 )
		return 0;
	return GetPropertyDesc( pField );
}

bool CObjectManipulatorIteratorWrapper::Next()
{
	return pIterator->Next();
}

bool CObjectManipulatorIteratorWrapper::IsEnd() const
{
	return pIterator->IsEnd();
}

bool CObjectManipulatorIteratorWrapper::GetName( std::string *pszName ) const
{
	*pszName = pIterator->GetName();
	return true;
}

bool CObjectManipulatorIteratorWrapper::GetType( std::string *pszType ) const
{
	const SPropertyDesc *pDesc = static_cast<const SPropertyDesc*>( GetDesc() );
	NI_ASSERT( pDesc != 0, "Unable to get property desc" );
	*pszType = pDesc->szPropControlType;
	return true;
}


