#pragma once

interface IStatisticsData : public CObjectBase
{
	virtual float GetValue() const = 0;
	virtual void Add( const float& fValue ) = 0;
	virtual void Reset() = 0;
};

interface IStatisticsCollector : public CObjectBase
{
	virtual void SetSpecific( const string &szName, IStatisticsData* pData ) = 0;
	virtual IStatisticsData* operator[]( const string &szName ) = 0;
};

namespace NStatistics
{
	IStatisticsCollector* CreateCollector( const string &szCollectorName );
	void SetGlobalCounter( const string &szName, IStatisticsData* pData );
	IStatisticsData* GetGlobal( const string &szName );
	string DumpToString();
	void DumpToNameValueVectors( vector<string> *pNames, vector<float> *pValues );
	IStatisticsData * CreateAverageTimePerEventCounter();
	IStatisticsData * CreateAverageValuePerTimeCounter();
	IStatisticsData * CreateAverageValueCounter();
	IStatisticsData * CreateEventsCounter();
	void Reset();
}


