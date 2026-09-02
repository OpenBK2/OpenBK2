#pragma once

#include "DefaultController.h"
#include <fmt/format.h>
#include "ManipulatorManager.h"
#include "MapEditorLib_export.h"

// TYPE_INSERT:
// szName					- имя массива
// (int)newValue	- номер элемента куда вставлять, (-1) для вставки в конец массива
// (int)oldValue	- номер вставленного элемента, всегда реальный номер элемента (для Undo)
//
// TYPE_REMOVE:
// szName					- имя массива
// (int)newValue	- номер элемента откуда удалять, (-1) для удаления всех элементов
// (int)oldValue	- номер элемента куда вставлять при Redo (при удалении последнего элемента = (-1))
// arrayList			- список удаленных массивов в рекурсивном порядке (если выставлено (int)newValue (-1) то первым идет удаленный массив)
//   szName				- имя массива
//   nCount				- количество элементов в массивах
// valueList			- список удаленных полей со значениями (значимых, без массивов и структур)
//   szName				- имя элемента
//   value        - значение
//
// TYPE_CHANGE
// szName					- имя элемента (не массив, не структура)
// oldValue				- старое (предыдущее) значение
// newValue				- новое (установленное) значение
//
// TYPE_EXPAND		( используется только для Update )
// szName					- имя элемента (не массив, не структура)
// newValue				- 1 Expand, 0 Collapse

class MAPEDITORLIB_EXPORT CObjectBaseController : public CDefaultController
{
public:
	struct SUndoData
	{
		enum EType
		{
			TYPE_INSERT		= 0,
			TYPE_REMOVE		= 1,
			TYPE_CHANGE		= 2,
			TYPE_EXPAND		= 3,
		};
		//
		struct SArrayData
		{
			std::string szName;
			int nCount;
		};
		//
		struct SValueData
		{
			std::string szName;
			CVariant value;
		};
		//
		typedef std::list<SArrayData> CArrayDataList;
		typedef std::list<SValueData> CValueDataList;

		EType eType;
		std::string szName;
		CVariant oldValue;
		CVariant newValue;
		CArrayDataList arrayList;
		CValueDataList valueList;
		
		void FillLists( const std::string &szStartNodeName, IManipulator *pObjectManipulator );
		bool Undo( IManipulator *pObjectManipulator, const IManipulator::CNameMap *pNameMap ) const;
		MAPEDITORLIB_EXPORT bool Redo( IManipulator *pObjectManipulator, const IManipulator::CNameMap *pNameMap ) const;
	};
	//
	typedef std::list<SUndoData> CUndoDataList;
	
	// список данных подвергнутых изменениям
	CUndoDataList undoDataList;

protected:
	// CController
	virtual bool UndoWithoutUpdateViews();
	virtual bool RedoWithoutUpdateViews();

public:
	virtual void Trace() const;

	// IController
	virtual bool IsEmpty() const { return undoDataList.empty(); }
	virtual bool IsAbsolute() const;
	virtual void GetDescription( CString *pstrDescription ) const;

	// Helpers
	bool AddInsertOperation( const std::string &rszArrayName, const int nIndex, IManipulator *pObjectManipulator );
	bool AddRemoveOperation( const std::string &rszArrayName, const int nIndex, IManipulator *pObjectManipulator );
	bool AddExpandOperation( const std::string &rszPropertyName, bool bExpand, IManipulator *pObjectManipulator );
	bool AddChangeOperation( const std::string &rszPropertyName, const CVariant &rValue, IManipulator *pObjectManipulator );
	//
	template<class TValue> 
	bool AddChangeValueOperation( const std::string &rszPropertyName, const TValue &rNewData, IManipulator *pObjectManipulator )
	{
		NI_ASSERT( pObjectManipulator != 0, "CObjectBaseController::AddChangeValueOperation() pObjectManipulator == 0" );
		//
		TValue oldData;
		if ( !CManipulatorManager::GetValue( &oldData, pObjectManipulator, rszPropertyName ) )
		{
			return false;
		}
		CUndoDataList::iterator posNewUndoData = undoDataList.insert( undoDataList.end(), SUndoData() );
		posNewUndoData->eType = SUndoData::TYPE_CHANGE;
		posNewUndoData->szName = rszPropertyName;
		posNewUndoData->newValue = rNewData;
		posNewUndoData->oldValue = oldData;
		//
		return posNewUndoData->Redo( pObjectManipulator, 0 );
	}
	//
	template<> 
	bool AddChangeValueOperation( const std::string &rszPropertyName, const unsigned &rNewData, IManipulator *pObjectManipulator )
	{
		NI_ASSERT( pObjectManipulator != 0, "CObjectBaseController::AddChangeValueOperation() pObjectManipulator == 0" );
		//
		unsigned oldData;
		if ( !CManipulatorManager::GetValue( &oldData, pObjectManipulator, rszPropertyName ) )
		{
			return false;
		}
		CUndoDataList::iterator posNewUndoData = undoDataList.insert( undoDataList.end(), SUndoData() );
		posNewUndoData->eType = SUndoData::TYPE_CHANGE;
		posNewUndoData->szName = rszPropertyName;
		posNewUndoData->newValue = (int)rNewData;
		posNewUndoData->oldValue = (int)oldData;
		//
		return posNewUndoData->Redo( pObjectManipulator, 0 );
	}
	//
	template<class TValue, class TFieldType>  
	bool AddChangeVec2Operation( const std::string &rszPropertyName, const TValue &rvNewData, IManipulator *pObjectManipulator )
	{
		NI_ASSERT( pObjectManipulator != 0, "CObjectBaseController::AddChangeVec2Operation() pObjectManipulator == 0" );
		//
		TValue vOldData;
		if ( !CManipulatorManager::GetVec2<TValue, TFieldType>( &vOldData, pObjectManipulator, rszPropertyName ) )
		{
			return false;
		}
		//
		{
			CUndoDataList::iterator posNewUndoData = undoDataList.insert( undoDataList.end(), SUndoData() );
			posNewUndoData->eType = SUndoData::TYPE_CHANGE;
			posNewUndoData->szName = fmt::format( "{}{:c}x", rszPropertyName.c_str(), LEVEL_SEPARATOR_CHAR );
			posNewUndoData->newValue = rvNewData.x;
			posNewUndoData->oldValue = vOldData.x;
			if ( !posNewUndoData->Redo( pObjectManipulator, 0 ) )
			{
				return false;
			}
		}
		//
		{
			CUndoDataList::iterator posNewUndoData = undoDataList.insert( undoDataList.end(), SUndoData() );
			posNewUndoData->eType = SUndoData::TYPE_CHANGE;
			posNewUndoData->szName = fmt::format( "{}{:c}y", rszPropertyName.c_str(), LEVEL_SEPARATOR_CHAR );
			posNewUndoData->newValue = rvNewData.y;
			posNewUndoData->oldValue = vOldData.y;
			if ( !posNewUndoData->Redo( pObjectManipulator, 0 ) )
			{
				return false;
			}
		}
		return true;
	}
	//
	template<class TValue, class TFieldType>  
	bool AddChangeVec3Operation( const std::string &rszPropertyName, const TValue &rvNewData, IManipulator *pObjectManipulator )
	{
		NI_ASSERT( pObjectManipulator != 0, "CObjectBaseController::AddChangeVec3Operation() pObjectManipulator == 0" );
		//
		TValue vOldData;
		if ( !CManipulatorManager::GetVec3<TValue, TFieldType>( &vOldData, pObjectManipulator, rszPropertyName ) )
		{
			return false;
		}
		//
		{
			CUndoDataList::iterator posNewUndoData = undoDataList.insert( undoDataList.end(), SUndoData() );
			posNewUndoData->eType = SUndoData::TYPE_CHANGE;
			posNewUndoData->szName = fmt::format( "{}{:c}x", rszPropertyName.c_str(), LEVEL_SEPARATOR_CHAR );
			posNewUndoData->newValue = rvNewData.x;
			posNewUndoData->oldValue = vOldData.x;
			if ( !posNewUndoData->Redo( pObjectManipulator, 0 ) )
			{
				return false;
			}
		}
		//
		{
			CUndoDataList::iterator posNewUndoData = undoDataList.insert( undoDataList.end(), SUndoData() );
			posNewUndoData->eType = SUndoData::TYPE_CHANGE;
			posNewUndoData->szName = fmt::format( "{}{:c}y", rszPropertyName.c_str(), LEVEL_SEPARATOR_CHAR );
			posNewUndoData->newValue = rvNewData.y;
			posNewUndoData->oldValue = vOldData.y;
			if ( !posNewUndoData->Redo( pObjectManipulator, 0 ) )
			{
				return false;
			}
		}
		//
		{
			CUndoDataList::iterator posNewUndoData = undoDataList.insert( undoDataList.end(), SUndoData() );
			posNewUndoData->eType = SUndoData::TYPE_CHANGE;
			posNewUndoData->szName = fmt::format( "{}{:c}z", rszPropertyName.c_str(), LEVEL_SEPARATOR_CHAR );
			posNewUndoData->newValue = rvNewData.z;
			posNewUndoData->oldValue = vOldData.z;
			if ( !posNewUndoData->Redo( pObjectManipulator, 0 ) )
			{
				return false;
			}
		}
		return true;
	}
	//
	template<class TValue, class TFieldType>  
	bool AddChangeVec4Operation( const std::string &rszPropertyName, const TValue &rvNewData, IManipulator *pObjectManipulator )
	{
		NI_ASSERT( pObjectManipulator != 0, "CObjectBaseController::AddChangeVec4Operation() pObjectManipulator == 0" );
		//
		TValue vOldData;
		if ( !CManipulatorManager::GetVec4<TValue, TFieldType>( &vOldData, pObjectManipulator, rszPropertyName ) )
		{
			return false;
		}
		//
		{
			CUndoDataList::iterator posNewUndoData = undoDataList.insert( undoDataList.end(), SUndoData() );
			posNewUndoData->eType = SUndoData::TYPE_CHANGE;
			posNewUndoData->szName = fmt::format( "{}{:c}x", rszPropertyName.c_str(), LEVEL_SEPARATOR_CHAR );
			posNewUndoData->newValue = rvNewData.x;
			posNewUndoData->oldValue = vOldData.x;
			if ( !posNewUndoData->Redo( pObjectManipulator, 0 ) )
			{
				return false;
			}
		}
		//
		{
			CUndoDataList::iterator posNewUndoData = undoDataList.insert( undoDataList.end(), SUndoData() );
			posNewUndoData->eType = SUndoData::TYPE_CHANGE;
			posNewUndoData->szName = fmt::format( "{}{:c}y", rszPropertyName.c_str(), LEVEL_SEPARATOR_CHAR );
			posNewUndoData->newValue = rvNewData.y;
			posNewUndoData->oldValue = vOldData.y;
			if ( !posNewUndoData->Redo( pObjectManipulator, 0 ) )
			{
				return false;
			}
		}
		//
		{
			CUndoDataList::iterator posNewUndoData = undoDataList.insert( undoDataList.end(), SUndoData() );
			posNewUndoData->eType = SUndoData::TYPE_CHANGE;
			posNewUndoData->szName = fmt::format( "{}{:c}z", rszPropertyName.c_str(), LEVEL_SEPARATOR_CHAR );
			posNewUndoData->newValue = rvNewData.z;
			posNewUndoData->oldValue = vOldData.z;
			if ( !posNewUndoData->Redo( pObjectManipulator, 0 ) )
			{
				return false;
			}
		}
		//
		{
			CUndoDataList::iterator posNewUndoData = undoDataList.insert( undoDataList.end(), SUndoData() );
			posNewUndoData->eType = SUndoData::TYPE_CHANGE;
			posNewUndoData->szName = fmt::format( "{}{:c}w", rszPropertyName.c_str(), LEVEL_SEPARATOR_CHAR );
			posNewUndoData->newValue = rvNewData.w;
			posNewUndoData->oldValue = vOldData.w;
			if ( !posNewUndoData->Redo( pObjectManipulator, 0 ) )
			{
				return false;
			}
		}
		return true;
	}
	//
	template<> 
		bool AddChangeValueOperation( const std::string &rszPropertyName, const CVec2 &vNewData, IManipulator *pObjectManipulator )
	{
		return AddChangeVec2Operation<CVec2, float>( rszPropertyName, vNewData, pObjectManipulator );
	}
	template<> 
		bool AddChangeValueOperation( const std::string &rszPropertyName, const CVec3 &vNewData, IManipulator *pObjectManipulator )
	{
		return AddChangeVec3Operation<CVec3, float>( rszPropertyName, vNewData, pObjectManipulator );
	}
	template<> 
		bool AddChangeValueOperation( const std::string &rszPropertyName, const CVec4 &vNewData, IManipulator *pObjectManipulator )
	{
		return AddChangeVec4Operation<CVec4, float>( rszPropertyName, vNewData, pObjectManipulator );
	}
	//
	template<class TValue, class TElementType> 
	bool AddChangeArrayOperation( const std::string &rszPropertyName, const TValue &rNewData, IManipulator *pObjectManipulator )
	{
		NI_ASSERT( pObjectManipulator != 0, "CObjectBaseController::AddChangeArrayOperation() pObjectManipulator == 0" );
		//
		int nExistingElementCount = 0;
		if ( !CManipulatorManager::GetValue( &nExistingElementCount, pObjectManipulator, rszPropertyName ) )
		{
			return false;
		}
		int nElementIndex = 0;
		for ( typename TValue::const_iterator itElement = rNewData.begin(); itElement != rNewData.end(); ++itElement )
		{
			// вставляем элемент если необходимо
			if ( nElementIndex >= nExistingElementCount )
			{
				CUndoDataList::iterator posNewUndoData = undoDataList.insert( undoDataList.end(), SUndoData() );
				posNewUndoData->eType = SUndoData::TYPE_INSERT;
				posNewUndoData->szName = rszPropertyName;
				posNewUndoData->newValue = NODE_ADD_INDEX;
				posNewUndoData->oldValue = nElementIndex;
				if ( !posNewUndoData->Redo( pObjectManipulator, 0 ) )
				{
					return false;
				}
			}
			// Устанавливаем значение
			{
				const std::string szElementName = fmt::format( "{}{:c}{:c}{}{:c}", rszPropertyName.c_str(), LEVEL_SEPARATOR_CHAR, ARRAY_NODE_START_CHAR, nElementIndex, ARRAY_NODE_END_CHAR );
				TElementType oldData;
				if ( !CManipulatorManager::GetValue( &oldData, pObjectManipulator, szElementName ) )
				{
					return false;
				}
				//
				CUndoDataList::iterator posNewUndoData = undoDataList.insert( undoDataList.end(), SUndoData() );
				posNewUndoData->eType = SUndoData::TYPE_CHANGE;
				posNewUndoData->szName = szElementName;
				posNewUndoData->newValue = ( *itElement );
				posNewUndoData->oldValue = oldData;
				if ( !posNewUndoData->Redo( pObjectManipulator, 0 ) )
				{
					return false;
				}
			}
			++nElementIndex;
		}
		// Удаляем лишние элементы 
		const std::string szElementName = fmt::format( "{}{:c}{:c}{}{:c}", rszPropertyName.c_str(), LEVEL_SEPARATOR_CHAR, ARRAY_NODE_START_CHAR, nElementIndex, ARRAY_NODE_END_CHAR );
		for ( ;nElementIndex < nExistingElementCount; --nExistingElementCount )
		{
			TElementType oldData;
			if ( !CManipulatorManager::GetValue( &oldData, pObjectManipulator, szElementName ) )
			{
				return false;
			}
			//
			CUndoDataList::iterator posNewUndoData = undoDataList.insert( undoDataList.end(), SUndoData() );
			posNewUndoData->eType = CObjectBaseController::SUndoData::TYPE_REMOVE;
			posNewUndoData->szName = rszPropertyName;
			posNewUndoData->newValue = nElementIndex;
			if ( nElementIndex == ( nExistingElementCount - 1 ) )
			{
				posNewUndoData->oldValue = NODE_ADD_INDEX;
			}
			else
			{
				posNewUndoData->oldValue = nElementIndex;
			}
			SUndoData::CValueDataList::iterator posValueData = posNewUndoData->valueList.insert( posNewUndoData->valueList.end(), SUndoData::SValueData() );		
			posValueData->szName = szElementName;
			posValueData->value = oldData;
			if ( !posNewUndoData->Redo( pObjectManipulator, 0 ) )
			{
				return false;
			}
		}
		return true;
	}
	template <class TValue> 
		bool AddChangeValueOperation( const std::string &rszPropertyName, const std::vector<TValue> &rNewData, IManipulator *pObjectManipulator )
	{
		return AddChangeArrayOperation<std::vector<TValue>, TValue>( rszPropertyName, rNewData, pObjectManipulator );
	}
	template <class TValue> 
		bool AddChangeValueOperation( const std::string &rszPropertyName, const std::list<TValue> &rNewData, IManipulator *pObjectManipulator )
	{
		return AddChangeArrayOperation<std::list<TValue>, TValue>( rszPropertyName, rNewData, pObjectManipulator );
	}
};



