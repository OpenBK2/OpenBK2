#ifndef __POINT_CHECKING_H__
#define __POINT_CHECKING_H__

#pragma ONCE

#include "../Common_RTS_AI/PointChecking.h"

class CAttackPointChecking : public IPointChecking
{
	OBJECT_BASIC_METHODS( CAttackPointChecking );

	ZDATA
	float fRangeMin, fRangeMax;
	SVector targetTile;
	bool bIgnoreObstacles;
	public: ZEND int operator&( IBinSaver &f ) { f.Add(2,&fRangeMin); f.Add(3,&fRangeMax); f.Add(4,&targetTile); f.Add(5,&bIgnoreObstacles); return 0; }
public:
	CAttackPointChecking() { }
	CAttackPointChecking( const float _fRangeMin, const float _fRangeMax, const SVector &_targetTile, const bool _bIgnoreObstacles )
		: fRangeMin( _fRangeMin ), fRangeMax( Max(0.0f, _fRangeMax - 5 * SConsts::TILE_SIZE ) ), targetTile( _targetTile ), bIgnoreObstacles( _bIgnoreObstacles ) { }
	
	virtual bool IsGoodTile( const SVector &curTile ) const;
};

class CAttackSideChecking : public IPointChecking
{
	OBJECT_BASIC_METHODS( CAttackSideChecking );

	ZDATA
	SAIAngle wAttackDir, wHalfAngle;
	float fRangeMin, fRangeMax;
	SVector targetTile;
	bool bIgnoreObstacles;
	public: ZEND int operator&( IBinSaver &f ) { f.Add(2,&wAttackDir); f.Add(3,&wHalfAngle); f.Add(4,&fRangeMin); f.Add(5,&fRangeMax); f.Add(6,&targetTile); f.Add(7,&bIgnoreObstacles); return 0; }
public:
	CAttackSideChecking() { }
	CAttackSideChecking( float _fRangeMin, const float _fRangeMax, const SVector _targetTile, const WORD _wAttackDir, const WORD _wHalfAngle, const bool _bIgnoreObstacles )
		: wAttackDir( _wAttackDir ), wHalfAngle( _wHalfAngle * 4 / 5 ), fRangeMin( _fRangeMin ), fRangeMax( Max(0.0f, _fRangeMax - 5 * SConsts::TILE_SIZE ) ), targetTile( _targetTile ), bIgnoreObstacles( _bIgnoreObstacles ) { }

	virtual bool IsGoodTile( const SVector &curTile ) const;
};

class CGoToDistance : public IPointChecking
{
	OBJECT_BASIC_METHODS( CGoToDistance );

	float tileDistance2;
	SVector targetTile;

public:
	CGoToDistance() { }
	// дистанция задаётся в тайлах
	CGoToDistance( const float tileDistance, const SVector &_targetTile )
		: tileDistance2( sqr( tileDistance ) ), targetTile( _targetTile ) { }

	virtual bool IsGoodTile( const SVector &curTile ) const;
};
class CStaticObject;

class CAttackStObjectChecking : public IPointChecking
{
	OBJECT_BASIC_METHODS( CAttackStObjectChecking );

	ZDATA
	float fRangeMin, fRangeMax;
	CPtr<CStaticObject> pObj;
	bool bIgnoreObstacles;
	public: ZEND int operator&( IBinSaver &f ) { f.Add(2,&fRangeMin); f.Add(3,&fRangeMax); f.Add(4,&pObj); f.Add(5,&bIgnoreObstacles); return 0; }
public:
	CAttackStObjectChecking() { }
	CAttackStObjectChecking( const float fRangeMin, const float fRangeMax, class CStaticObject *pObj, const bool _bIgnoreObstacles  );

	virtual bool IsGoodTile( const SVector &curTile ) const;
};

#endif // __POINT_CHECKING_H__

