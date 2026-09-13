#include "stdafx.h"
#include "MapEditorLib/ResourceDefines.h"
#include "MapEditorLib/CommandHandlerDefines.h"
#include "ResourceDefines.h"

#include "System/FileUtils.h"
#include "PC_StringFileRefEditor.h"
#include "MapEditorLib/Interface_UserData.h"
#include "MapEditorLib/Interface_MOD.h"

CPCStringFileRefEditor::CPCStringFileRefEditor( const std::string &rszObjectTypeName ) : szObjectTypeName( rszObjectTypeName )
{
}


// CPCItemEditor

void CPCStringFileRefEditor::GetValue( CVariant *pValue )
{
	if ( pValue )
	{
		if ( const SPropertyDesc *pDesc = GetPropertyDesc() )
		{
			CPCStringBrowseEditor::GetValue( pValue );
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
					szMask = pDesc->szStringParam;
					if ( szMask.empty() )
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
				CPCStringBrowseEditor::GetValue( pValue );
			}
		}
	}
}


// CPCStringBrowseEditor

// The file picker, in NPropertyButton.
void CPCStringFileRefEditor::OnBrowse()
{
	PressButton( NPropertyButton::BUTTON_BROWSE );
}

// basement storage


