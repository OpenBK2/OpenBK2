#pragma once

struct IStatisticsData : public CObjectBase
{
	virtual float GetValue() const = 0;
	virtual void Add( const float& fValue ) = 0;
	virtual void Reset() = 0;
};

struct IStatisticsCollector : public CObjectBase
{
	virtual void SetSpecific( const std::string &szName, IStatisticsData* pData ) = 0;
	virtual IStatisticsData* operator[]( const std::string &szName ) = 0;
};

namespace NStatistics
{
	IStatisticsCollector* CreateCollector( const std::string &szCollectorName );
	void SetGlobalCounter( const std::string &szName, IStatisticsData* pData );
	IStatisticsData* GetGlobal( const std::string &szName );
	std::string DumpToString();
	void DumpToNameValueVectors( std::vector<std::string> *pNames, std::vector<float> *pValues );
	IStatisticsData * CreateAverageTimePerEventCounter();
	IStatisticsData * CreateAverageValuePerTimeCounter();
	IStatisticsData * CreateAverageValueCounter();
	IStatisticsData * CreateEventsCounter();
	void Reset();
}


