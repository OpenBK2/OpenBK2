#include "stdafx.h"

#include "Statistics.hpp"

std::unordered_map< std::string, CObj<IStatisticsData> > CStatisticsCollector::globalData;// name - data
std::unordered_map< std::string, CObj<CStatisticsCollector> > CStatisticsCollector::collectors;
UINT64 CStatisticsCollector::nStartTime;

std::string CStatisticsCollector::DumpToStringSpecific() const
{
	std::string szOutString = szSpecificName + "\n";
	for ( std::unordered_map< std::string, CObj<IStatisticsData> >::const_iterator it = specificData.begin(); it != specificData.end(); ++it )
	{
		const std::string szName = it->first;
		IStatisticsData* pData = it->second;
		szOutString += "  " + szName + StrFmt( ": %f\n", pData->GetValue() );
	}
	return szOutString;
}

std::string CStatisticsCollector::DumpToString()
{
	std::string szOutString = "GLOBAL\n";
	for ( std::unordered_map< std::string, CObj<IStatisticsData> >::const_iterator it = globalData.begin(); it != globalData.end(); ++it )
	{
		const std::string szName = it->first;
		const IStatisticsData* pData = it->second;
		szOutString += "  " + szName + StrFmt( ": %f\n", pData->GetValue() );
	}
	for ( std::unordered_map<std::string, CObj<CStatisticsCollector> >::const_iterator it = collectors.begin(); it != collectors.end(); ++it )
	{
		const std::string szCollectorName = it->first;
		const CStatisticsCollector* pCollector = it->second;
		szOutString += pCollector->DumpToStringSpecific();
	}
	long int nUptime = (long int)( ( GetLongTickCount() - nStartTime ) / 1000 );
	int nUptimeMsec = (int)( ( GetLongTickCount() - nStartTime ) % 1000 );
	szOutString += StrFmt( "Server is up for %ld.%d sec.\n", nUptime, nUptimeMsec );
	return szOutString;
}

void CStatisticsCollector::DumpToNameValueVectorsSpecific( std::vector<std::string> *pNames, std::vector<float> *pValues )
{
  for ( std::unordered_map< std::string, CObj<IStatisticsData> >::iterator it = specificData.begin(); it != specificData.end(); ++it )
	{
		IStatisticsData *pData = it->second;
		pNames->push_back( szSpecificName + "_" + it->first );
		pValues->push_back( pData->GetValue() );
	}
}

void CStatisticsCollector::DumpToNameValueVectors( std::vector<std::string> *pNames, std::vector<float> *pValues )
{
	for ( std::unordered_map< std::string, CObj<IStatisticsData> >::iterator it = globalData.begin(); it != globalData.end(); ++it )
	{
		IStatisticsData *pData = it->second;
		pNames->push_back( "GLOBAL_" + it->first );
		pValues->push_back( pData->GetValue() );
	}
	for ( std::unordered_map< std::string, CObj<CStatisticsCollector> >::iterator it = collectors.begin(); it != collectors.end(); ++it )
	{
		CStatisticsCollector *pCollector = it->second;
		pCollector->DumpToNameValueVectorsSpecific( pNames, pValues );
	}
}

void CStatisticsCollector::Reset()
{
	for ( std::unordered_map< std::string, CObj<IStatisticsData> >::iterator it = globalData.begin(); it != globalData.end(); ++it )
	{
		IStatisticsData *pData = it->second;
		pData->Reset();
	}
	for ( std::unordered_map< std::string, CObj<CStatisticsCollector> >::iterator it = collectors.begin(); it != collectors.end(); ++it )
	{
		CStatisticsCollector *pCollector = it->second;
		pCollector->ResetSpecific();
	}
}

void CStatisticsCollector::ResetSpecific()
{
	for ( std::unordered_map< std::string, CObj<IStatisticsData> >::iterator it = specificData.begin(); it != specificData.end(); ++it )
	{
		IStatisticsData *pData = it->second;
		pData->Reset();
	}
}

void CAverageTimeBetweenEvents::Add( const float &fValue )
{
	nEvents += fValue;
	if ( nStartTime == 0ULL )
		nStartTime = GetLongTickCount();
}

void CAverageValuePerTime::Add( const float& fValue )
{
	fEventsSum += fValue;
	if ( nStartTime == 0ULL )
		nStartTime = GetLongTickCount();
}

float CAverageTimeBetweenEvents::GetValue() const
{
	const UINT64 nTimeDiff = GetLongTickCount() - nStartTime;
	return nEvents == 0ULL ? 0.0f : (float)( double( nTimeDiff / 1000 ) / double( nEvents ) );
}

float CAverageValuePerTime::GetValue() const
{
	const UINT64 nTimeDiff = GetLongTickCount() - nStartTime;
	return nTimeDiff == 0ULL ? 0.0f : ( fEventsSum / nTimeDiff * 1000 );
}


