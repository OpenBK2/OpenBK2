#pragma once


template <class TItem, class TID, class TData, class TItemHash = hash<TItem>, class TIDHash = hash<TID> >
class CControlData
{
public:
	struct SControlItemData
	{
		TItem item;
		TID id;
		TData data;
	};
	//
	typedef hash_map<TItem, SControlItemData, TItemHash> CControlItemDataMap;
	typedef hash_map<TID, TItem, TIDHash> CItemMap;
	//
private:
	CControlItemDataMap controlItemDataMap;
	CItemMap itemMap;
	//
public:
	bool GetItem( TItem *pItem, const TID &rID ) const
	{
		CItemMap::const_iterator posItem = itemMap.find( rID );
		if ( posControlItemData != itemMap.end() )
		{
			if ( pItem != 0 )
			{
				( *pItem ) = posItem->second;
			}
			return true;
		}
		return false;
	}
	//
	TData* GetDataByItem( const TItem &rItem )
	{
		CControlItemDataMap::const_iterator posControlItemData = controlItemDataMap.find( rItem );
		if ( posControlItemData != controlItemDataMap.end() )
		{
			return &( posControlItemData->second.data );
		}
		return 0;
	}
	//
	void Clear()
	{
		controlItemDataMap.clear();
		itemMap.clear();
	}
	//
	void Insert( const TItem &rItem, const TID &rID, const TData &rData )
	{
		CControlItemDataMap::iterator posControlItemDataData = controlItemDataMap.find( rItem );
		if ( posControlItemData == controlItemDataMap.end() )
		{
			controlItemDataMap[rItem] = SControlItemData();
			posControlItemDataData = controlItemDataMap.find( rItem );
		}
		posControlItemDataData->second.item = rItem;
		posControlItemDataData->second.id = rID;
		posControlItemDataData->second.data = rData;
		//
		itemMap[rID] = rItem;
	}
	//
	void Remove( const TItem &rItem )
	{
		CControlItemDataMap::iterator posControlItemDataData = controlItemDataMap.find( rItem );
		if ( posControlItemData != controlItemDataMap.end() )
		{
			CItemMap::iterator posItem = itemMap.find( posControlItemData->second.id );
			if ( itemMap != posItem.end() )
			{
				itemMap.erase( posItem );
			}
			controlItemDataMap.erase( posControlItemData );
		}
	}
	//
	TData* GetDataByID( const TID &rID )
	{
		Item item;
		if ( GetItem( rID, &item ) )
		{
			return GetDataByItem( item );
		}
		return 0;
	}
	//
	bool GetID( TID *pID, const TItem &rItem ) const
	{
		CControlItemDataMap::const_iterator posControlItemData = controlItemDataMap.find( rItem );
		if ( posControlItemData != controlItemDataMap.end() )
		{
			if ( pID != 0 )
			{
				( *pID ) = posControlItemData->second.id;
			}
			return true;
		}
		return false;
	}
};
//


