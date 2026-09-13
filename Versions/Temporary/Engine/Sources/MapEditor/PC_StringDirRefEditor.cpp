#include "stdafx.h"
#include "MapEditorLib/ResourceDefines.h"
#include "MapEditorLib/CommandHandlerDefines.h"
#include "ResourceDefines.h"

#include "PC_StringDirRefEditor.h"

#include "MapEditorLib/Interface_MainFrame.h"
#include "MapEditorLib/Interface_MOD.h"

CPCStringDirRefEditor::CPCStringDirRefEditor( const std::string &rszObjectTypeName ) : szObjectTypeName( rszObjectTypeName )
{
}


// CPCItemEditor

void CPCStringDirRefEditor::GetValue( CVariant *pValue )
{
	if ( pValue )
	{
		if ( const SPropertyDesc *pDesc = GetPropertyDesc() )
		{
			CPCStringBrowseEditor::GetValue( pValue );
			std::string szPath = pValue->GetStr();
			bool bResult = false;
			if ( !szPath.empty() )
			{
				SUserData::ENormalizePathType pathType = SUserData::NPT_UNKNOWN;
				if ( ( pDesc->nIntParam > SUserData::NPT_UNKNOWN ) && ( pDesc->nIntParam < SUserData::NPT_COUNT ) )
				{
					pathType = static_cast<SUserData::ENormalizePathType>( pDesc->nIntParam );
				}
				if ( ::IsValidFileName( szPath, false ) )
				{
					( *pValue ) = szPath;
					//
					// Устанавливаем каталог куда будем заглядывать при последующем вызове диалога открытия файла
					const std::string szFullPath = Singleton<IMODContainer>()->GetDataFolder( pathType ) + szPath;
					std::string szObjectNamePrefix;
					CStringManager::SplitFileName( &szObjectNamePrefix, 0, 0, szFullPath );
					SUserData::CFilePathMap &rFilePathMap = Singleton<IUserDataContainer>()->Get()->filePathMap;
					rFilePathMap[NPropertyButton::PSZ_FOLDER_PATH_LABEL] = szFullPath;
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

// The folder picker, in NPropertyButton.
void CPCStringDirRefEditor::OnBrowse()
{
	PressButton( NPropertyButton::BUTTON_BROWSE );
}

// basement storage


