#include "stdafx.h"

#include "PC_Constants.h"
#include "PC_StringComboRefEditor.h"
#include "libdb/ResourceManager.h"
#include "MapEditorLib/PCIEMnemonics.h"

// CPCItemEditor

bool CPCStringComboRefEditor::CreateEditor( const std::string &rszName, EPCIEType _nEditorType, const SPropertyDesc* _pPropertyDesc, int _nControlID, const SObjectSet &rObjectSet, CWnd *_pwndTargetWindow )
{
	if ( CPCStringComboEditor::CreateEditor( rszName, _nEditorType, _pPropertyDesc, _nControlID, rObjectSet, _pwndTargetWindow ) )
	{
		SetCreateControls( true );
		ResetContent();
		AddString( PCSV_NULL );
		//
		std::vector<std::string> stringList;
		//
		// Получаем список таблиц в базе данных
		std::list<std::string> tables;
		if ( IResourceManager *pResourceManager = Singleton<IResourceManager>() )
		{
			if ( CPtr<IManipulator> pTableManipulator = pResourceManager->CreateTableManipulator() )
			{
				if ( CPtr<IManipulatorIterator> pTableManipulatorIterator = pTableManipulator->Iterate( true, ECT_CACHE_LOCAL ) )
				{
					std::string szTableName;
					while ( !pTableManipulatorIterator->IsEnd() )
					{
						pTableManipulatorIterator->GetName( &szTableName );
						tables.push_back( szTableName );
						pTableManipulatorIterator->Next();
					}
				}
			}
			// Заполняем список объектов
			for ( std::list<std::string>::const_iterator itTable = tables.begin(); itTable != tables.end(); ++itTable )
			{
				if ( GetPropertyDesc()->refTypes.find( *itTable ) != GetPropertyDesc()->refTypes.end() )
				{
					if ( CPtr<IManipulator> pFolderManipulator = pResourceManager->CreateFolderManipulator( *itTable ) )
					{
						if ( CPtr<IManipulatorIterator> pFolderManipulatorIterator = pFolderManipulator->Iterate( true, ECT_CACHE_LOCAL ) )
						{
							std::string szTableName;
							if ( typePCIEMnemonics.IsMultiRef( GetItemEditorType() ) )
							{
								szTableName = *itTable + TYPE_SEPARATOR_CHAR;
							}
							std::string szName;
							while ( !pFolderManipulatorIterator->IsEnd() )
							{
								pFolderManipulatorIterator->GetName( &szName );
								if ( ( !szName.empty() ) &&
										 ( szName[szName.size() - 1] != PATH_SEPARATOR_CHAR ) )
								{
									stringList.push_back( szTableName + szName );			
								}
								pFolderManipulatorIterator->Next();
							}
						}
					}
				}
			}
		}
		//
		sort( stringList.begin(), stringList.end(), CPCStringComboRefEditorCompareItem() ); 
		for ( std::vector<std::string>::const_iterator itString = stringList.begin(); itString != stringList.end(); ++itString )
		{
			AddString( itString->c_str() );
		}
		SetCreateControls( false );
		return true;
	}
	return false;
}


void CPCStringComboRefEditor::SetValue( const CVariant &rValue )
{
	if ( rValue.GetType() == CVariant::VT_NULL )
	{
		CVariant nulRefValue = std::string( PCSV_NULL );
		CPCStringComboEditor::SetValue( nulRefValue );
	}
	else
	{
		CPCStringComboEditor::SetValue( rValue );
	}
}


void CPCStringComboRefEditor::GetValue( CVariant *pValue )
{
	if ( pValue )
	{
		CPCStringComboEditor::GetValue( pValue );
		if ( pValue->GetStringRecode().empty() || ( pValue->GetStringRecode() == std::string(PCSV_NULL) ) )
		{
			( *pValue ) = CVariant();
		}
	}
}

// basement storage  


