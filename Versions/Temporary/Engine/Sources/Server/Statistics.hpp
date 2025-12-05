#pragma once
#include "Statistics.h"
#include "Misc/Time64.h"


class CStatisticsCollector : public IStatisticsCollector
{
	OBJECT_NOCOPY_METHODS( CStatisticsCollector )
	static std::unordered_map< std::string, CObj<IStatisticsData> > globalData;			//	name -> data
	static std::unordered_map< std::string, CObj<CStatisticsCollector> > collectors;	//  name -> collector

	std::unordered_map< std::string, CObj<IStatisticsData> > specificData;						//	name -> data
	std::string szSpecificName;																						//	name of collector

	static UINT64 nStartTime;

	std::string DumpToStringSpecific() const;
	void ResetSpecific();
	void DumpToNameValueVectorsSpecific( std::vector<std::string> *pNames, std::vector<float> *pValues );
public:
	CStatisticsCollector() {}
	CStatisticsCollector( const std::string &szCollectorName ) : szSpecificName( szCollectorName )
	{
		collectors[szCollectorName] = this; 
		nStartTime = GetLongTickCount(); 
	} 
	virtual void SetSpecific( const std::string &szName, IStatisticsData* pData ) { specificData[szName] = pData; }
	virtual IStatisticsData* operator[]( const std::string &szName )
	{
		NI_ASSERT( specificData.find( szName ) != specificData.end(), 
			StrFmt( "Statistics counter %s for collector %s is not set", szName.c_str(), szSpecificName.c_str() ) )
		return specificData[szName]; 
	}
	static IStatisticsData* GetGlobal( const std::string &szName )
	{ 
		NI_ASSERT( globalData.find( szName ) != globalData.end(), 
			StrFmt( "Global statistics counter %s is not set", szName.c_str() ) );
		return globalData[szName]; 
	}
	static void SetGlobal( const std::string &szName, IStatisticsData* pData ) { globalData[szName] = pData; }
	static void Reset();
	static std::string DumpToString();
	static void DumpToNameValueVectors( std::vector<std::string> *pNames, std::vector<float> *pValues );
};

class CAverageTimeBetweenEvents : public IStatisticsData
{
	OBJECT_NOCOPY_METHODS( CAverageTimeBetweenEvents )
	UINT64 nEvents;
	UINT64 nStartTime;
public:
	CAverageTimeBetweenEvents() : nStartTime( 0ULL ), nEvents( 0ULL ) {}
	virtual float GetValue() const;
	virtual void Add( const float& fValue );
	virtual void Reset() { nStartTime = 0ULL; nEvents = 0ULL; }
};

class CAverageValuePerTime : public IStatisticsData
{
	OBJECT_NOCOPY_METHODS( CAverageValuePerTime )

	float fEventsSum;
	UINT64 nStartTime;
public:
	CAverageValuePerTime() : nStartTime( 0ULL ), fEventsSum( 0 ) {}
	virtual float GetValue() const;
	virtual void Add( const float& fValue );
	virtual void Reset() { fEventsSum = 0.0f; nStartTime = 0ULL; }
};

class CAverageValue : public IStatisticsData
{
	OBJECT_NOCOPY_METHODS( CAverageValue )
	unsigned int nEvents;
	float fSum;
public:
	CAverageValue() : fSum( 0.0f ), nEvents( 0U ) {}
	virtual float GetValue() const { return nEvents == 0U ? -1.0f : fSum / float( nEvents ); }
	virtual void Add( const float& fValue ) { fSum += fValue; ++nEvents; }
	virtual void Reset() { fSum = 0.0f; nEvents = 0U; }
};

class CEventsCount : public IStatisticsData
{
	OBJECT_NOCOPY_METHODS( CEventsCount )
	unsigned int nEvents;
public:
	CEventsCount() : nEvents( 0U ) {}
	virtual float GetValue() const { return (float)nEvents; }
	virtual void Add( const float& fValue ) { nEvents += fValue; }
	virtual void Reset() { nEvents = 0U; }
};

namespace NStatistics
{
	IStatisticsCollector* CreateCollector( const std::string &szCollectorName ) { return new CStatisticsCollector( szCollectorName ); }
	void SetGlobalCounter( const std::string &szName, IStatisticsData* pData ) { CStatisticsCollector::SetGlobal( szName, pData ); }
	IStatisticsData * CreateAverageTimePerEventCounter() { return new CAverageTimeBetweenEvents(); }
	IStatisticsData * CreateAverageValuePerTimeCounter() { return new CAverageValuePerTime(); }
	IStatisticsData * CreateAverageValueCounter() { return new CAverageValue(); }
	IStatisticsData * CreateEventsCounter() { return new CEventsCount(); }
	IStatisticsData* GetGlobal( const std::string &szName ) { return CStatisticsCollector::GetGlobal( szName ); }
	void Reset() { CStatisticsCollector::Reset(); }
	std::string DumpToString() { return CStatisticsCollector::DumpToString(); }
	void DumpToNameValueVectors( std::vector<std::string> *pNames, std::vector<float> *pValues ) { CStatisticsCollector::DumpToNameValueVectors( pNames, pValues ); }
}




