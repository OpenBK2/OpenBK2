
#pragma once

#include "../System/BinaryResources.h"
#include "../libdb/Manipulator.h"

//
#define REFINFO_MAKE_UNIQUE_LIST	0x00000001
#define REFINFO_PROPERTY_NAME			0x00000002
#define REFINFO_OBJECT_TYPE_NAME	0x00000004
#define REFINFO_OBJECT_NAME				0x00000008
#define REFINFO_CHECK_EMPTY				0x00000010
#define REFINFO_CHECK_VALID				0x00000020
#define REFINFO_ALL								0xFFffFFff
//

class CManipulatorManager
{
public:
	struct SReferenceInfo
	{
		UINT nFlags;
		string szName;
		//
		string szObjectTypeName;
		string szObjectName;
		//
		bool isEmpty;
		bool isValid;

		SReferenceInfo() : nFlags( 0x00000000 ), isEmpty( true ), isValid( true ) {}
		SReferenceInfo( const SReferenceInfo &rReferenceInfo )
			: nFlags( rReferenceInfo.nFlags ),
				szName( rReferenceInfo.szName ),
				szObjectTypeName( rReferenceInfo.szObjectTypeName ),
				szObjectName( rReferenceInfo.szObjectName ),
				isEmpty( rReferenceInfo.isEmpty ),
				isValid( rReferenceInfo.isValid ) {}
		SReferenceInfo& operator=( const SReferenceInfo &rReferenceInfo )
		{
			if( &rReferenceInfo != this )
			{
				nFlags = rReferenceInfo.nFlags;
				szName = rReferenceInfo.szName;
				szObjectTypeName = rReferenceInfo.szObjectTypeName;
				szObjectName = rReferenceInfo.szObjectName;
				isEmpty = rReferenceInfo.isEmpty;
				isValid = rReferenceInfo.isValid;
			}
			return *this;
		}
	};
	typedef list<SReferenceInfo> CReferenceInfoList;

	// Скопировать значения из одного манипулятора в другой
	static bool CloneDBManipulator( interface IManipulator *pDestinationManipulator,
																	interface IManipulator *pSourceManipulator,
																	bool bEqual );
	// Проверить ссылку на то, что это ссылка, на заполненность, и получить данные о ней
	static bool GetParamsFromReference( const string &rszRefValueName,
																			const interface IManipulator *pSourceManipulator,
																			string *pszRefObjectTypeName,
																			string *pszRefObjectName,
																			const SPropertyDesc **ppRefDesc );
	// Получить манипулятор по ссылке ( если это ссылка )
	static interface IManipulator* CreateManipulatorFromReference( const string &rszRefValueName,
																																 const interface IManipulator *pSourceManipulator,
																																 string *pszRefObjectTypeName,
																																 string *pszRefObjectName,
																																 const SPropertyDesc **ppRefDesc );

	// Получить манипулятор по ссылке ( если это ссылка ), если ссылка NULL,
	//   создать ее и установить ее как значение.
	// Создание ссылки происходит по стандартному адресу:
	//   ПолноеИмяИсходногоМанипулятора\szReferenceName
	// Параметры:
	//   pResultManipulator - указатель на манипулятор-результат
	//          (в него поместится то что есть по ссылке, или новое значение)
	//   pManipulator       - указатель на исходный манипулятор, поле которого
	//          есть ссылка, которую необходимо получить или создать
	//   szTableName        - имя таблицы, в которой необходимо создать новую
	//          ссылку, если поле пусто
	//   szFieldName        - имя поля исходного манипулятора, из которого достаем
	//          ссылку (или записываем вновь созданную ссылку)
	//   szReferenceName    - имя вновь созданной ссылки
	//   pszResultName      - указатель на строку, в которую вернется имя созданного объекта. Может быть 0.
	static bool ForceCreateManipulatorForReference( CPtr<IManipulator> *pResultManipulator,
	                                      IManipulator *pManipulator,
																				const string &szTableName,
																				const string &szFieldName,
																				const string &szReferenceName,
																				string *pszResultName );

	// получить список ссылок из манипулятора
	static bool EnumReferences( CReferenceInfoList *pReferenceInfoList,
															const interface IManipulator* pSourceManipulator,
															const UINT nFlags,
															const bool bEnumHidden,
															const ECacheType eCacheType );
	//
	// Привести размер массива к указанному ( добавить необходимое или удалить лишнее )
	static bool EnsureArraySize( const int nSize,
															 interface IManipulator *pManipulator,
															 const string &rszArrayName );
	// получить список ссылок из манипулятора
	static void Trace( const string &rszPrefix, interface IManipulator* pManipulator );
	//
	// создать манипулятор по ObjectSet 
	static IManipulator *CreateObectSetManipulator( const struct SObjectSet &rObjectSet );

	template<class TValue> 
	static bool SetValue( const TValue &rData, interface IManipulator *pManipulator, const string &rszName )
	{
		NI_ASSERT( pManipulator != 0, "CManipulatorManager::SetValue(): pManipulator == 0" );
		return pManipulator->SetValue( rszName, CVariant( rData ) );
	}
	//
	template<> 
	static bool SetValue( const CVariant &rData, interface IManipulator *pManipulator, const string &rszName )
	{
		NI_ASSERT( pManipulator != 0, "CManipulatorManager::SetValue(): pManipulator == 0" );
		return pManipulator->SetValue( rszName, rData );
	}
	//
	static bool SetValue( const string &rszData, interface IManipulator *pManipulator, const string &rszName, bool bReference )
	{
		NI_ASSERT( pManipulator != 0, "CManipulatorManager::SetValue(): pManipulator == 0" );
		if ( rszData.empty() && bReference )
		{
			return pManipulator->SetValue( rszName, CVariant() );
		}
		else
		{
			return pManipulator->SetValue( rszName, CVariant( rszData ) );
		}
	}
	//
	template<class TValue> 
	static bool SetVec2( const TValue &rvData, interface IManipulator *pManipulator, const string &rszName )
	{
		NI_ASSERT( pManipulator != 0, "CManipulatorManager::SetVec2(): pManipulator == 0" );
		bool bResult = true;
		bResult = bResult && pManipulator->SetValue( StrFmt( "%s%cx", rszName.c_str(), LEVEL_SEPARATOR_CHAR ), CVariant( rvData.x ) );
		bResult = bResult && pManipulator->SetValue( StrFmt( "%s%cy", rszName.c_str(), LEVEL_SEPARATOR_CHAR ), CVariant( rvData.y ) );
		return bResult;
	}
	//
	template<class TValue> 
	static bool SetVec3( const TValue &rvData, interface IManipulator *pManipulator, const string &rszName )
	{
		NI_ASSERT( pManipulator != 0, "CManipulatorManager::SetVec3(): pManipulator == 0" );
		bool bResult = true;
		bResult = bResult && pManipulator->SetValue( StrFmt( "%s%cx", rszName.c_str(), LEVEL_SEPARATOR_CHAR ), CVariant( rvData.x ) );
		bResult = bResult && pManipulator->SetValue( StrFmt( "%s%cy", rszName.c_str(), LEVEL_SEPARATOR_CHAR ), CVariant( rvData.y ) );
		bResult = bResult && pManipulator->SetValue( StrFmt( "%s%cz", rszName.c_str(), LEVEL_SEPARATOR_CHAR ), CVariant( rvData.z ) );
		return bResult;
	}
	//
	template<class TValue> 
	static bool SetVec4( const TValue &rvData, interface IManipulator *pManipulator, const string &rszName )
	{
		NI_ASSERT( pManipulator != 0, "CManipulatorManager::SetVec4(): pManipulator == 0" );
		bool bResult = true;
		bResult = bResult && pManipulator->SetValue( StrFmt( "%s%cx", rszName.c_str(), LEVEL_SEPARATOR_CHAR ), CVariant( rvData.x ) );
		bResult = bResult && pManipulator->SetValue( StrFmt( "%s%cy", rszName.c_str(), LEVEL_SEPARATOR_CHAR ), CVariant( rvData.y ) );
		bResult = bResult && pManipulator->SetValue( StrFmt( "%s%cz", rszName.c_str(), LEVEL_SEPARATOR_CHAR ), CVariant( rvData.z ) );
		bResult = bResult && pManipulator->SetValue( StrFmt( "%s%cw", rszName.c_str(), LEVEL_SEPARATOR_CHAR ), CVariant( rvData.w ) );
		return bResult;
	}
	//
	template<> 
	static bool SetValue( const CVec2 &rData, interface IManipulator *pManipulator, const string &rszName )
	{
		return SetVec2<CVec2>( rData, pManipulator, rszName );
	}
	template<> 
	static bool SetValue( const CVec3 &rData, interface IManipulator *pManipulator, const string &rszName )
	{
		return SetVec3<CVec3>( rData, pManipulator, rszName );
	}
	template<> 
	static bool SetValue( const CVec4 &rData, interface IManipulator *pManipulator, const string &rszName )
	{
		return SetVec4<CVec4>( rData, pManipulator, rszName );
	}
	//
	template<class TValue> 
	static bool SetArray( const TValue &rData, interface IManipulator *pManipulator, const string &rszName )
	{
		NI_ASSERT( pManipulator != 0, "CManipulatorManager::SetArray(): pManipulator == 0" );
		int nExistingElementCount = 0;
		bool bResult = GetValue( &nExistingElementCount, pManipulator, rszName );
		if ( bResult )
		{
			int nElementIndex = 0;
			for ( TValue::const_iterator itElement = rData.begin(); itElement != rData.end(); ++itElement )
			{
				if ( nElementIndex >= nExistingElementCount )
				{
					bResult = pManipulator->InsertNode( rszName, nElementIndex );
				}
				bResult = bResult && SetValue( ( *itElement ), pManipulator, StrFmt( "%s%c%c%d%c", rszName.c_str(), LEVEL_SEPARATOR_CHAR, ARRAY_NODE_START_CHAR, nElementIndex, ARRAY_NODE_END_CHAR ) );
				if ( !bResult )
				{
					break;
				}
				++nElementIndex;
			}
			if ( bResult )
			{
				for ( ;nElementIndex < nExistingElementCount; --nExistingElementCount )
				{
					bResult = pManipulator->RemoveNode( rszName, nElementIndex );
					if ( !bResult )
					{
						break;
					}
				}
			}
		}
		return bResult;
	}
	//
	template<class TValue> 
	static bool Set2DArray( const TValue &rData, interface IManipulator *pManipulator, const string &rszName )
	{
		NI_ASSERT( pManipulator != 0, "CManipulatorManager::Set2DArray(): pManipulator == 0" );
		const string sz2DArrayName = StrFmt( "%s%cdata", rszName.c_str(), LEVEL_SEPARATOR_CHAR );
		int nExistingXCount = 0;
		bool bResult = GetValue( &nExistingXCount, pManipulator, sz2DArrayName );
		if ( bResult )
		{
			const int nXCount = rData.GetSizeX();
			const int nYCount = rData.GetSizeY();
			//
			int nXIndex = 0;
			for ( ;nXIndex < nXCount; ++nXIndex )
			{
				// Добавляем внешний элемент массива
				if ( nXIndex >= nExistingXCount )
				{
					bResult = pManipulator->InsertNode( sz2DArrayName, nXIndex );
				}
				const string szArrayName = StrFmt( "%s%c%c%d%c%cdata", sz2DArrayName.c_str(), LEVEL_SEPARATOR_CHAR, ARRAY_NODE_START_CHAR, nXIndex, ARRAY_NODE_END_CHAR, LEVEL_SEPARATOR_CHAR );
				// Добавляем внутренний массив
				int nExistingYCount = 0;
				bResult = bResult && GetValue( &nExistingYCount, pManipulator, szArrayName );
				if ( bResult )
				{
					int nYIndex = 0;
					for ( ;nYIndex < nYCount; ++nYIndex )
					{
						// Добавляем внутренний элемент массива
						if ( nYIndex >= nExistingYCount )
						{
							bResult = pManipulator->InsertNode( szArrayName, nYIndex );
						}
						bResult = bResult && SetValue( rData[nYIndex][nXIndex], pManipulator, StrFmt( "%s%c%c%d%c", szArrayName.c_str(), LEVEL_SEPARATOR_CHAR, ARRAY_NODE_START_CHAR, nYIndex, ARRAY_NODE_END_CHAR ) );
						if ( !bResult )
						{
							break;
						}
					}
				}
				if ( !bResult )
				{
					break;
				}
			}
			if ( bResult )
			{
				for ( ;nXIndex < nExistingXCount; --nExistingXCount )
				{
					// Удаляем сначало внутренний массив
					const string szArrayName = StrFmt( "%s%c%c%d%c%cdata", sz2DArrayName.c_str(), LEVEL_SEPARATOR_CHAR, ARRAY_NODE_START_CHAR, nXIndex, ARRAY_NODE_END_CHAR, LEVEL_SEPARATOR_CHAR );
					bResult = pManipulator->RemoveNode( szArrayName );
					// Удаляем элемент внешнего массива
					bResult = bResult && pManipulator->RemoveNode( sz2DArrayName, nXIndex );
					if ( !bResult )
					{
						break;
					}
				}
			}
		}
		return bResult;
	}
	//
	template<class TValue> 
	static bool GetValue( TValue *pData, interface IManipulator *pManipulator, const string &rszName )
	{
		NI_ASSERT( pData != 0, "CManipulatorManager::GetValue(): pData == 0" );
		NI_ASSERT( pManipulator != 0, "CManipulatorManager::GetValue(): pManipulator == 0" );
		CVariant value;
		bool bResult = pManipulator->GetValue( rszName, &value );
		bResult = bResult && ( value.GetType() != CVariant::VT_NULL );
		if ( bResult )
		{
			( *pData ) = (TValue)value;
		}
		return bResult;
	}
	//
	template<> 
	static bool GetValue( CVariant *pData, interface IManipulator *pManipulator, const string &rszName )
	{
		NI_ASSERT( pData != 0, "CManipulatorManager::GetValue(): pData == 0" );
		NI_ASSERT( pManipulator != 0, "CManipulatorManager::GetValue(): pManipulator == 0" );
		return pManipulator->GetValue( rszName, pData );
	}
	//
	template<> 
	static bool GetValue( string *pData, interface IManipulator *pManipulator, const string &rszName )
	{
		NI_ASSERT( pData != 0, "CManipulatorManager::GetValue(): pData == 0" );
		NI_ASSERT( pManipulator != 0, "CManipulatorManager::GetValue(): pManipulator == 0" );
		CVariant value;
		bool bResult = pManipulator->GetValue( rszName, &value );
		if ( bResult )
		{
			if ( value.GetType() != CVariant::VT_NULL ) 
			{
				( *pData ) = value.GetStringRecode();
			}
			else
			{
				pData->clear();
			}
		}
		return bResult;
	}
	//
	template<> 
	static bool GetValue( UINT *pData, interface IManipulator *pManipulator, const string &rszName )
	{
		NI_ASSERT( pData != 0, "CManipulatorManager::GetValue(): pData == 0" );
		NI_ASSERT( pManipulator != 0, "CManipulatorManager::GetValue(): pManipulator == 0" );
		CVariant value;
		bool bResult = pManipulator->GetValue( rszName, &value );
		bResult = bResult && ( value.GetType() != CVariant::VT_NULL );
		if ( bResult )
		{
			( *pData ) = (UINT)(int)value;
		}
		return bResult;
	}
	//
	template<> 
	static bool GetValue( WORD *pData, interface IManipulator *pManipulator, const string &rszName )
	{
		NI_ASSERT( pData != 0, "CManipulatorManager::GetValue(): pData == 0" );
		NI_ASSERT( pManipulator != 0, "CManipulatorManager::GetValue(): pManipulator == 0" );
		CVariant value;
		bool bResult = pManipulator->GetValue( rszName, &value );
		bResult = bResult && ( value.GetType() != CVariant::VT_NULL );
		if ( bResult )
		{
			( *pData ) = (WORD)(int)value;
		}
		return bResult;
	}
	//
	template<class TValue, class TFieldType>  
	static bool GetVec2( TValue *pvData, interface IManipulator *pManipulator, const string &rszName )
	{
		NI_ASSERT( pvData != 0, "CManipulatorManager::GetVec2(): pvData == 0" );
		NI_ASSERT( pManipulator != 0, "CManipulatorManager::GetVec2(): pManipulator == 0" );
		CVariant valueX;
		CVariant valueY;
		bool bResult = true;
		bResult = bResult && pManipulator->GetValue( StrFmt( "%s%cx", rszName.c_str(), LEVEL_SEPARATOR_CHAR ), &valueX );
		bResult = bResult && ( valueX.GetType() != CVariant::VT_NULL );
		bResult = bResult && pManipulator->GetValue( StrFmt( "%s%cy", rszName.c_str(), LEVEL_SEPARATOR_CHAR ), &valueY );
		bResult = bResult && ( valueY.GetType() != CVariant::VT_NULL );
		if ( bResult )
		{
			pvData->x = (TFieldType)valueX;
			pvData->y = (TFieldType)valueY;
		}
		return bResult;
	}
	//
	template<class TValue, class TFieldType>  
	static bool GetVec3( TValue *pvData, interface IManipulator *pManipulator, const string &rszName )
	{
		NI_ASSERT( pvData != 0, "CManipulatorManager::GetVec3(): pvData == 0" );
		NI_ASSERT( pManipulator != 0, "CManipulatorManager::GetVec3(): pManipulator == 0" );
		CVariant valueX;
		CVariant valueY;
		CVariant valueZ;
		bool bResult = true;
		bResult = bResult && pManipulator->GetValue( StrFmt( "%s%cx", rszName.c_str(), LEVEL_SEPARATOR_CHAR ), &valueX );
		bResult = bResult && ( valueX.GetType() != CVariant::VT_NULL );
		bResult = bResult && pManipulator->GetValue( StrFmt( "%s%cy", rszName.c_str(), LEVEL_SEPARATOR_CHAR ), &valueY );
		bResult = bResult && ( valueY.GetType() != CVariant::VT_NULL );
		bResult = bResult && pManipulator->GetValue( StrFmt( "%s%cz", rszName.c_str(), LEVEL_SEPARATOR_CHAR ), &valueZ );
		bResult = bResult && ( valueZ.GetType() != CVariant::VT_NULL );
		if ( bResult )
		{
			pvData->x = (TFieldType)valueX;
			pvData->y = (TFieldType)valueY;
			pvData->z = (TFieldType)valueZ;
		}
		return bResult;
	}
	//
	template<class TValue, class TFieldType>  
	static bool GetVec4( TValue *pvData, interface IManipulator *pManipulator, const string &rszName )
	{
		NI_ASSERT( pvData != 0, "CManipulatorManager::GetVec4(): pvData == 0" );
		NI_ASSERT( pManipulator != 0, "CManipulatorManager::GetVec4(): pManipulator == 0" );
		CVariant valueX;
		CVariant valueY;
		CVariant valueZ;
		CVariant valueW;
		bool bResult = true;
		bResult = bResult && pManipulator->GetValue( StrFmt( "%s%cx", rszName.c_str(), LEVEL_SEPARATOR_CHAR ), &valueX );
		bResult = bResult && ( valueX.GetType() != CVariant::VT_NULL );
		bResult = bResult && pManipulator->GetValue( StrFmt( "%s%cy", rszName.c_str(), LEVEL_SEPARATOR_CHAR ), &valueY );
		bResult = bResult && ( valueY.GetType() != CVariant::VT_NULL );
		bResult = bResult && pManipulator->GetValue( StrFmt( "%s%cz", rszName.c_str(), LEVEL_SEPARATOR_CHAR ), &valueZ );
		bResult = bResult && ( valueZ.GetType() != CVariant::VT_NULL );
		bResult = bResult && pManipulator->GetValue( StrFmt( "%s%cw", rszName.c_str(), LEVEL_SEPARATOR_CHAR ), &valueW );
		bResult = bResult && ( valueW.GetType() != CVariant::VT_NULL );
		if ( bResult )
		{
			pvData->x = (TFieldType)valueX;
			pvData->y = (TFieldType)valueY;
			pvData->z = (TFieldType)valueZ;
			pvData->w = (TFieldType)valueW;
		}
		return bResult;
	}
	//
	template<> 
	static bool GetValue( CVec2 *pData, interface IManipulator *pManipulator, const string &rszName )
	{
		return GetVec2<CVec2, float>( pData, pManipulator, rszName );
	}
	template<> 
	static bool GetValue( CVec3 *pData, interface IManipulator *pManipulator, const string &rszName )
	{
		return GetVec3<CVec3, float>( pData, pManipulator, rszName );
	}
	template<> 
	static bool GetValue( CVec4 *pData, interface IManipulator *pManipulator, const string &rszName )
	{
		return GetVec4<CVec4, float>( pData, pManipulator, rszName );
	}
	//
	template<>
	static bool GetValue( GUID *pData, interface IManipulator *pManipulator, const string &rszName )
	{
		NI_ASSERT( pData != 0, "CManipulatorManager::GetValue(): pData == 0" );
		NI_ASSERT( pManipulator != 0, "CManipulatorManager::GetValue(): pManipulator == 0" );
		CVariant value;
		bool bResult = pManipulator->GetValue( rszName, &value );
		bResult = bResult && ( value.GetType() != CVariant::VT_NULL );
		if ( bResult )
		{
			NI_ASSERT( ( value.GetType() == CVariant::VT_POINTER ) && ( value.GetBlobSize() == sizeof(GUID) ),
					StrFmt( "CManipulatorManager::GetValue<GUID>(): '%s' is not a GUID field", rszName.c_str() ) );
			bResult = ( value.GetType() == CVariant::VT_POINTER ) && ( value.GetBlobSize() == sizeof(GUID) );
			if ( bResult )
			{
				memcpy( pData, value.GetPtr(), sizeof(GUID) );
			}
		}
		return bResult;
	}
	static bool GetGUIDAsString( string *pData, interface IManipulator *pManipulator, const string &rszName )
	{
		NI_ASSERT( pData != 0, "CManipulatorManager::GetValue(): pData == 0" );
		NI_ASSERT( pManipulator != 0, "CManipulatorManager::GetValue(): pManipulator == 0" );
		GUID guid;
		if ( GetValue( &guid, pManipulator, rszName ) )
		{
			( *pData ) = NBinResources::GUIDToString(guid);
			return true;
		}
		return false;
	}
	//
	template<class TValue, class TElementType> 
	static bool GetArray( TValue *pvData, interface IManipulator *pManipulator, const string &rszName )
	{
		NI_ASSERT( pvData != 0, "CManipulatorManager::GetArray(): pvData == 0" );
		NI_ASSERT( pManipulator != 0, "CManipulatorManager::GetArray(): pManipulator == 0" );
		int nExistingElementCount = 0;
		bool bResult = GetValue( &nExistingElementCount, pManipulator, rszName );
		if ( bResult )
		{
			pvData->clear();
			for ( int nElementIndex = 0; nElementIndex != nExistingElementCount; ++nElementIndex )
			{
				TElementType element;
				bResult = GetValue( &element, pManipulator, StrFmt( "%s%c%c%d%c", rszName.c_str(), LEVEL_SEPARATOR_CHAR, ARRAY_NODE_START_CHAR, nElementIndex, ARRAY_NODE_END_CHAR ) );
				if ( !bResult )
				{
					break;
				}
				pvData->insert( pvData->end(), element ); 
			}
		}
		return bResult;
	}
	template<class TValue, class TElementType> 
	static bool GetVec2Array( TValue *pvData, interface IManipulator *pManipulator, const string &rszName )
	{
		NI_ASSERT( pvData != 0, "CManipulatorManager::GetArray(): pvData == 0" );
		NI_ASSERT( pManipulator != 0, "CManipulatorManager::GetArray(): pManipulator == 0" );
		int nExistingElementCount = 0;
		bool bResult = GetValue( &nExistingElementCount, pManipulator, rszName );
		if ( bResult )
		{
			pvData->clear();
			for ( int nElementIndex = 0; nElementIndex != nExistingElementCount; ++nElementIndex )
			{
				TElementType element;
				bResult = GetVec2( &element, pManipulator, StrFmt( "%s%c%c%d%c", rszName.c_str(), LEVEL_SEPARATOR_CHAR, ARRAY_NODE_START_CHAR, nElementIndex, ARRAY_NODE_END_CHAR ) );
				if ( !bResult )
				{
					break;
				}
				pvData->insert( pvData->end(), element ); 
			}
		}
		return bResult;
	}
	template<class TValue, class TElementType> 
	static bool GetVec3Array( TValue *pvData, interface IManipulator *pManipulator, const string &rszName )
	{
		NI_ASSERT( pvData != 0, "CManipulatorManager::GetArray(): pvData == 0" );
		NI_ASSERT( pManipulator != 0, "CManipulatorManager::GetArray(): pManipulator == 0" );
		int nExistingElementCount = 0;
		bool bResult = GetValue( &nExistingElementCount, pManipulator, rszName );
		if ( bResult )
		{
			pvData->clear();
			for ( int nElementIndex = 0; nElementIndex != nExistingElementCount; ++nElementIndex )
			{
				TElementType element;
				bResult = GetVec3( &element, pManipulator, StrFmt( "%s%c%c%d%c", rszName.c_str(), LEVEL_SEPARATOR_CHAR, ARRAY_NODE_START_CHAR, nElementIndex, ARRAY_NODE_END_CHAR ) );
				if ( !bResult )
				{
					break;
				}
				pvData->insert( pvData->end(), element ); 
			}
		}
		return bResult;
	}
	//
	template<class TValue, class TElementType> 
	static bool Get2DArray( TValue *pvData, interface IManipulator *pManipulator, const string &rszName, const TElementType &rDefaultValue )
	{
		NI_ASSERT( pvData != 0, "CManipulatorManager::Get2DArray(): pvData == 0" );
		NI_ASSERT( pManipulator != 0, "CManipulatorManager::Get2DArray(): pManipulator == 0" );
		const string sz2DArrayName = StrFmt( "%s%cdata", rszName.c_str(), LEVEL_SEPARATOR_CHAR );
		int nExistingXCount = 0;
		int nExistingYCount = 0;
		bool bResult = GetValue( &nExistingXCount, pManipulator, sz2DArrayName );
		// необходимо получить максимальный размер по Y
		if ( bResult )
		{
			vector<int> sizeList;
			sizeList.resize( nExistingXCount );
			for ( int nXIndex = 0; nXIndex != nExistingXCount; ++nXIndex )
			{
				const string szArrayName = StrFmt( "%s%c%c%d%c%cdata", sz2DArrayName.c_str(), LEVEL_SEPARATOR_CHAR, ARRAY_NODE_START_CHAR, nXIndex, ARRAY_NODE_END_CHAR, LEVEL_SEPARATOR_CHAR );
				int nLocalYCount = 0;
				bResult = GetValue( &nLocalYCount, pManipulator, szArrayName );
				if ( !bResult )
				{
					break;
				}
				if ( nLocalYCount > nExistingYCount )
				{
					nExistingYCount = nLocalYCount;
				}
				sizeList[nXIndex] = nLocalYCount;
			}
			if ( bResult )
			{
				if ( ( nExistingXCount * nExistingYCount ) > 0 )
				{
					pvData->SetSizes( nExistingXCount, nExistingYCount );
					pvData->FillEvery( rDefaultValue );
					// заполняем массив значениями
					for ( int nXIndex = 0; nXIndex != nExistingXCount; ++nXIndex )
					{
						const string szArrayName = StrFmt( "%s%c%c%d%c%cdata", sz2DArrayName.c_str(), LEVEL_SEPARATOR_CHAR, ARRAY_NODE_START_CHAR, nXIndex, ARRAY_NODE_END_CHAR, LEVEL_SEPARATOR_CHAR );
						for ( int nYIndex = 0; nYIndex != sizeList[nXIndex]; ++nYIndex )
						{
							bResult = CManipulatorManager::GetValue( &( ( *pvData )[nYIndex][nXIndex] ), pManipulator, StrFmt( "%s%c%c%d%c", szArrayName.c_str(), LEVEL_SEPARATOR_CHAR, ARRAY_NODE_START_CHAR, nYIndex, ARRAY_NODE_END_CHAR ) );
							if ( !bResult )
							{
								break;
							}
						}					
						if ( !bResult )
						{
							break;
						}
					}
				}
				else
				{
					pvData->Clear();
				}
			}
		}
		return bResult;
	}
	//
	static bool Remove2DArray( interface IManipulator *pManipulator, const string &rszName );
};


