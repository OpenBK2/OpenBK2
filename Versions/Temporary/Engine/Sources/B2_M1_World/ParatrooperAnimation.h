#pragma once

#include "UpdatableProcess.h"
#include "Stats_B2_M1/Season.h"

class CMapObj;
class CMOUnitInfantry;
namespace NDb
{
	struct SAnimB2;
}
//
class CParatrooperAnimationProcess : public IClientUpdatableProcess
{
	OBJECT_NOCOPY_METHODS( CParatrooperAnimationProcess );
	//
	ZDATA
		CPtr<CMapObj> pInfantry;
		NTimer::STime timeToChange;
		ZSKIP
		int nParachuteID;
		ZSKIP
		CDBPtr<NDb::SAnimB2> pUnitAnim;
		CDBPtr<NDb::SAnimB2> pParaAnim;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pInfantry); f.Add(3,&timeToChange); f.Add(5,&nParachuteID); f.Add(7,&pUnitAnim); f.Add(8,&pParaAnim); return 0; }
	//
	CParatrooperAnimationProcess() {}
public:
	CParatrooperAnimationProcess( CMapObj *_pInfantry, NTimer::STime _timeToChange, const NDb::SAnimB2 *_pUnitAnim,
		int _nParachuteID, const NDb::SAnimB2 *_pParaAnim )
		: pInfantry( _pInfantry ), timeToChange( _timeToChange ), 
	    nParachuteID( _nParachuteID ), pUnitAnim( _pUnitAnim ),pParaAnim( _pParaAnim ) {}
	bool Update( const NTimer::STime &time );
};

class CParachuteFinishProcess : public IClientUpdatableProcess
{
	OBJECT_NOCOPY_METHODS( CParachuteFinishProcess );
	//
	ZDATA
		CPtr<CMOUnitInfantry> pInfantry;
		NTimer::STime timeToChange;
		ZSKIP
		NDb::ESeason eSeason;
		CDBPtr<NDb::SAnimB2> pIdleAnim;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pInfantry); f.Add(3,&timeToChange); f.Add(5,&eSeason); f.Add(6,&pIdleAnim); return 0; }
	//
	CParachuteFinishProcess() {}
public:
	CParachuteFinishProcess( CMOUnitInfantry *_pInfantry, NTimer::STime _timeToChange, const NDb::SAnimB2 *_pIdleAnim, NDb::ESeason _eSeason )
		: pInfantry( _pInfantry ), timeToChange( _timeToChange ), pIdleAnim( _pIdleAnim ), eSeason( _eSeason ) {}
	//
	bool Update( const NTimer::STime &time );
};


