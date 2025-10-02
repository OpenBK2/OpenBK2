#pragma once

#include "UpdatableProcess.h"
#include "Stats_B2_M1/DBAnimB2.h"

class CDeadHouseAnimations : public IClientUpdatableProcess
{
	struct SAnimationInfo
	{
		int nID;
		NTimer::STime nEndTime;

		SAnimationInfo() : nID( -1 ), nEndTime( 0 ) {}
		SAnimationInfo( const int _nID, const NTimer::STime _nEndTime ) : nID( _nID ), nEndTime( _nEndTime ) {}
	};
	struct SSortAnimationsByTime
	{
		bool operator()( const SAnimationInfo &anim1, const SAnimationInfo &anim2 ) const 
		{ 
			return anim1.nEndTime > anim2.nEndTime;
		}
	};
	OBJECT_BASIC_METHODS( CDeadHouseAnimations );
	ZDATA
		vector<SAnimationInfo> animations;
	ZEND public: int operator&( IBinSaver &f ) { f.Add(2,&animations); return 0; } private:
private:
public:
	void Add( int nObjectID, const NTimer::STime &time, const NDb::SAnimB2 *pAnimation );
	void Sort();
	bool Update( const NTimer::STime &time );
	const bool IsEmpty() const { return animations.empty(); }
};


