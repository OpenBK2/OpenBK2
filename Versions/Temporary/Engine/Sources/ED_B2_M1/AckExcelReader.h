#pragma once

namespace NAcks
{

struct SAckEntry
{
	string szSituationCode;
	string szRecordCode;
	string szFileName;
	float fProbability;
	int nSubsetCode;
};

bool LoadAcksTable( vector<SAckEntry> *pRes, const string &szFileName );

}

