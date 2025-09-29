#pragma once

#include "LinkObject.h"
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CAIUnit;
struct SAIObjectsUnderConstructionUpdate;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CLongObjectCreation : public CLinkObject
{
	ZDATA_(CLinkObject)
	float fWorkAccumulated;
	int nPlayer;
	bool bAllowAIModification;						// if false, then class must not change anything in AI.
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CLinkObject*)this); f.Add(2,&fWorkAccumulated); f.Add(3,&nPlayer); f.Add(4,&bAllowAIModification); return 0; }
protected:
	const BYTE GetPlayer() const { return nPlayer; }
	bool IsAnyUnitPrevent( const SRect &r1 ) const;
	void GetUnitsPreventing( const SRect &r1, list< CPtr<CAIUnit> > *units ) const;
	void UnlockPreventingUnits( list<CPtr<CAIUnit> > &preventing ) const;
	bool CanBuildOnRect( SRect r1, const list<SVector> &tilesUnder ) const;
	bool IsAIModificationAllowed() const { return bAllowAIModification; }

	// helper functions
	WORD GetLineAngle( const CVec2 &vBegin, const CVec2 &vEnd );
	void SplitLineToSegrments( vector<CVec2> *_vPoints, const CVec2 &vBegin, const CVec2 &vEnd, float TRENCHWIDTH );

public:
	CLongObjectCreation( const int _nPlayer, const bool _bAllowAIModification ) 
		: nPlayer( _nPlayer ), fWorkAccumulated( 0.0f ), bAllowAIModification ( _bAllowAIModification )
	{  
		if ( bAllowAIModification )
		{
			SetUniqueIdForObjects();
			Mem2UniqueIdObjs();
		}
	}
	virtual bool PreCreate( const CVec2 &vFrom, const CVec2 &vTo, const bool bCheckLock ) = 0;
	// максимальный размер окопа
	virtual const int GetMaxIndex() const = 0;
	// текущее состояние строительства
	virtual const int GetCurIndex() const = 0;
	// точка, где должы стоять строители
	virtual const CVec2 GetNextPoint( const int nPlace, const int nMaxPlace ) const = 0;
	// ставит очередной сегмент и переносит терминатор
	// для первого сегмента ставит начальный терминатор
	virtual void BuildNext() { fWorkAccumulated = 0.0f; }
	// находит юнитов, которые мещают дальнейшему строительству
	virtual void GetUnitsPreventing( list< CPtr<CAIUnit> > * units ) = 0;
	// есть ли хоть 1 юнит, который мешает
	virtual bool IsAnyUnitPrevent() const = 0;
	// может лт следуюший сегмент быть построен ( без учета юнитов )
	virtual bool CanBuildNext() const = 0;
	// чтобы каждый раз не проверять
	virtual void LockCannotBuild() = 0;
	// залочивает под сегментом, чтобы никто не встал сверху
	virtual void LockNext() = 0;
	// линия, с которой нужно убрать юнитов
	virtual CLine2 GetCurLine() = 0;
	virtual float GetPrice() = 0;
	// нужно ли читить, когда ходим от сегмента к сегменту
	virtual bool IsCheatPath() const { return false; }
	//when work finished, engineers must say that farther building impossible
	virtual bool CannotFinish() const { return false; }
	// work accumulation
	virtual void AddWork( const float fAdd ) { fWorkAccumulated += fAdd; }
	virtual float GetWorkDone() const { return fWorkAccumulated; }
	virtual void CreateObjects( SAIObjectsUnderConstructionUpdate * pUpdate ) { }
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace NLongObjectCreation
{
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct SCreationData
{
	CVec2 vStart;
	CVec2 vFinish;
	CPtr<CLongObjectCreation> pCreation;

	SCreationData( const CVec2 &_vStart, CPtr<CLongObjectCreation> _pCreation )
		: vStart( _vStart ), pCreation( _pCreation ), vFinish( -1, -1 )
	{
	}
};
typedef list<SCreationData> CCreations;
extern CCreations creations;
template <class CCreation>
void PreCreate( const CVec2 &vFinish, CObj<CLongObjectCreation> *_pCreation, bool bCheckLock )
{
	for ( CCreations::iterator it = creations.begin(); it != creations.end(); )
	{
		if ( !IsValid( it->pCreation ) )
			it = creations.erase( it );
		else
		{
			if ( it->pCreation == *_pCreation )
			{
				if ( it->vFinish == CVec2(-1,-1 ) )
				{
					// pre create (first call)
					it->vFinish = vFinish;
					it->pCreation->PreCreate( it->vStart, it->vFinish, bCheckLock );
				}
				else if ( it->vFinish == vFinish )
				{
					// nothing to do, all done
				}
				else
				{
					// create new and return
					SCreationData obj( it->vStart, dynamic_cast_ptr<CCreation*>(it->pCreation)->Duplicate() );
					obj.vFinish = vFinish;
					obj.pCreation->PreCreate( obj.vStart, obj.vFinish, bCheckLock );
					creations.push_back( obj );
					*_pCreation = dynamic_cast_ptr<CCreation*>( obj.pCreation );
				}
				return;
			}
			++it;
		}
	}
}
template <class CCreation>
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CLongObjectCreation * Create( const CVec2 &vPos, int nPlayer, bool bAllowModification )
{
	for ( CCreations::iterator it = creations.begin(); it != creations.end(); )
	{
		if ( !IsValid( it->pCreation ) )
			it = creations.erase( it );
		else
		{
			if ( it->vStart == vPos )
				return it->pCreation;
			++it;
		}
	}
	SCreationData data( vPos, new CCreation( nPlayer, bAllowModification ) );
	creations.push_back( data );
	return data.pCreation;
}
}
