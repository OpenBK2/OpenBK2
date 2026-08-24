#pragma once


template <class TID, class TData, class TIDHash = std::hash<TID> >
class CControlSelection
{
private:
	struct SControlSelectionData
	{
		TID id;
		TData data;
	};
	//
public:
	typedef std::unordered_map<TID, SControlSelectionData, TIDHash> CControlSelectionDataMap;
private:
	CControlSelectionDataMap controlSelectionDataMap;
	//
public:
	const CControlSelectionDataMap& Get() const { return controlSelectionDataMap; }
	TData* GetData( const TID &rID )
	{
		typename CControlSelectionDataMap::const_iterator posControlSelectionData = controlSelectionDataMap.find( rItem );
		if ( posControlSelectionData != controlSelectionDataMap.end() )
		{
			return &( posControlSelectionData->second.data );
		}
		return 0;
	}
	//
	void Clear()
	{
		controlSelectionDataMap.clear();
	}
	//
	bool IsEmpty() const
	{
		return controlSelectionDataMap.empty();
	}
	//
	void Insert( const TID &rID, const TData &rData )
	{
		typename CControlSelectionDataMap::iterator posControlSelectionData = controlSelectionDataMap.find( rID );
		if ( posControlSelectionData == controlSelectionDataMap.end() )
		{
			controlSelectionDataMap[rID] = SControlSelectionData();
			posControlSelectionData = controlSelectionDataMap.find( rID );
		}
		posControlSelectionData->second.id = rID;
		posControlSelectionData->second.data = rData;
	}
	//
	void Remove( const TID &rID )
	{
		typename CControlSelectionDataMap::iterator posControlSelectionData = controlSelectionDataMap.find( rID );
		if ( posControlSelectionData != controlSelectionDataMap.end() )
		{
			controlSelectionDataMap.erase( posControlSelectionData );
		}
	}
};
//


