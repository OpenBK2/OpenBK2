#pragma once

#include <cstdint>

struct SUpdateInfo
{
	uint32_t dwVersion;
	bool bFullUpdate;
	list<int> removed;
	list<int> added;
	list<int> changed;

	void Clear()
	{
		dwVersion = 0;
		bFullUpdate = false;
		removed.clear();
		added.clear();
		changed.clear();
	}
};

class CUpdatableList
{
	enum EChange
	{ 
		EC_NOP = 0,
		EC_ADDED = 1,
		EC_REMOVED = 2,
		EC_CHANGED = 3,
	};

	struct SChange
	{
		int nID;
		uint8_t change;

		SChange() : change( EC_NOP ) { }
	};

	vector<SChange> changes;
	hash_set<int> now;
	uint32_t dwVersion;

	//
	void GetFullUpdate( const int nNoIncludeID, SUpdateInfo *pUpdate );
	void GetUpdate( const int nNoIncludeID, const uint32_t dwOldVersion, SUpdateInfo *pUpdate );
public:
	CUpdatableList();

	void Add( const int nID );
	void Remove( const int nID );
	void Change( const int nID );

	void GetDiff( const int nNoIncludeID, const uint32_t dwOldVersion, SUpdateInfo *pUpdate );
};


