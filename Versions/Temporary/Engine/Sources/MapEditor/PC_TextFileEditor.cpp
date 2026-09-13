#include "stdafx.h"
#include "ResourceDefines.h"
#include "CommandHandlerDefines.h"
#include "PC_Constants.h"

#include "PC_TextFileEditor.h"
#include "System/FileUtils.h"
#include "MapEditorLib/StringManager.h"
#include "MapEditorLib/Interface_MOD.h"

// CPCItemEditor

void CPCTextFileEditor::GetValue( CVariant *pValue )
{
	if ( pValue )
	{
		if ( const SPropertyDesc *pDesc = GetPropertyDesc() )
		{
			CPCStringNewBrowseEditor::GetValue( pValue );
			std::string szFilePath = pValue->GetStr();
			bool bResult = false;
			if ( !szFilePath.empty() )
			{
				SUserData::ENormalizePathType pathType = SUserData::NPT_UNKNOWN;
				if ( ( pDesc->nIntParam > SUserData::NPT_UNKNOWN ) && ( pDesc->nIntParam < SUserData::NPT_COUNT ) )
				{
					pathType = static_cast<SUserData::ENormalizePathType>( pDesc->nIntParam );
				}
				if ( ::IsValidFileName( szFilePath, false ) )
				{
					( *pValue ) = szFilePath;
					//
					// Устанавливаем каталог куда будем заглядывать при последующем вызове диалога открытия файла
					const std::string szFullFilePath = Singleton<IMODContainer>()->GetDataFolder( pathType ) + szFilePath;
					std::string szObjectNamePrefix;
					CStringManager::SplitFileName( &szObjectNamePrefix, 0, 0, szFullFilePath );
					std::string szMask;
					if ( !CStringManager::GetStringValueFromString( pDesc->szStringParam, PCSPL_MASK, 0, PCSP_MASK_DIVIDERS, "", &szMask ) || szMask.empty() )
					{
						szMask = "All Files (*.*)|*.*||";
					}
					//
					SUserData::CFilePathMap &rFilePathMap = Singleton<IUserDataContainer>()->Get()->filePathMap;
					rFilePathMap[szMask] = szObjectNamePrefix;
					bResult = true;
				}
			}
			else
			{
				bResult = true;
			}
			if ( !bResult )
			{
				SetDefaultValue();
				CPCStringNewBrowseEditor::GetValue( pValue );
			}
		}
	}
}


// CPCStringNewBrowseEditor

// The second button, captioned "Edit": the file in the text editor, in
// NPropertyButton.
void CPCTextFileEditor::OnNew()
{
	PressButton( NPropertyButton::BUTTON_EDIT );
}


// The file picker, in NPropertyButton.
void CPCTextFileEditor::OnBrowse()
{
	PressButton( NPropertyButton::BUTTON_BROWSE );
}


bool CPCTextFileEditor::GetPCItemStringValue( std::string *pszValue, const CVariant &rValue, const SPropertyDesc *pPropertyDesc )
{
	( *pszValue ) = rValue.GetStringRecode();
	return true;
}


bool CPCTextFileEditor::GetPCItemValue( CVariant *pValue, const std::string &rszValue, const SPropertyDesc *pPropertyDesc )
{
	( *pValue ) = rszValue;
	return true;
}

// basement storage


