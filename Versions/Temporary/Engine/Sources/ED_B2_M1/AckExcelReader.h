#pragma once

namespace NAcks
{

struct SAckEntry
{
	std::string szSituationCode;
	std::string szRecordCode;
	std::string szFileName;
	float fProbability;
	int nSubsetCode;
};

bool LoadAcksTable( std::vector<SAckEntry> *pRes, const std::string &szFileName );

}

