#ifndef __aiInterval_H_
#define __aiInterval_H_
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
namespace NDb
{
	//struct SRPGArmor;
}
namespace NAI
{
struct SSourceInfo
{
	ZDATA
	CPtr<CObjectBase> pUserData;
	CDBPtr<NDb::CResource> pArmor;
	int nFloor;
	int nTSFlags;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pUserData); f.Add(3,&pArmor); f.Add(4,&nFloor); f.Add(5,&nTSFlags); return 0; }

	SSourceInfo() {}
	SSourceInfo( CObjectBase *_p, const NDb::CResource *_pArmor, int _nFloor, int _nTSFlags )
		: pUserData(_p), pArmor(_pArmor), nFloor(_nFloor), nTSFlags(_nTSFlags) {}
};

struct SInterval
{
	struct SCrossPoint
	{
		float fT;
		CVec3 ptNormal;
		//
		SCrossPoint() {}
		SCrossPoint( float _fT, const CVec3 &_ptNormal ): fT(_fT), ptNormal(_ptNormal) {}
	};
	//
	SCrossPoint enter, exit;
	const SSourceInfo *pSrc;
	int nUserID;
	//
	SInterval( const SSourceInfo &_src, int _nUserID, const SCrossPoint &_enter, const SCrossPoint &_exit )
		: pSrc(&_src), enter(_enter), exit(_exit), nUserID(_nUserID) {}
};

struct SSimpleInterval
{
	float fEnter, fExit;
	const SSourceInfo *pSrc;
	int nUserID;

	SSimpleInterval( const SSourceInfo &_src, int _nUserID, float _fEnter, float _fExit )
		: pSrc(&_src), fEnter(_fEnter), fExit(_fExit), nUserID(_nUserID) {}
};

void SortSimpleIntervals( vector<SSimpleInterval> *pRes );
void SortIntervals( vector<SInterval> *pRes );
void FillIntersectionResults( vector<SInterval> *pRes,
	vector<SInterval::SCrossPoint> *pEnter, 
	vector<SInterval::SCrossPoint> *pExit,
	const SSourceInfo &_src, int _nUserID, bool bTerrain );
void FillIntersectionResults( vector<SSimpleInterval> *pRes,
	vector<float> *pEnter, 
	vector<float> *pExit,
	const SSourceInfo &_src, int _nUserID, bool bTerrain );

}
#endif

