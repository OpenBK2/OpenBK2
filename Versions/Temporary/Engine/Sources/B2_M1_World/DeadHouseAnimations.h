#pragma once

// Defensive member defaults; explicit constructor values still take precedence.

#include "Stats_B2_M1/DBAnimB2.h"
#include "UpdatableProcess.h"

class CDeadHouseAnimations : public IClientUpdatableProcess
{
	struct SAnimationInfo
	{
		int nID = 0;
		NTimer::STime nEndTime = 0;

		SAnimationInfo()
			: nID( -1 )
			, nEndTime( 0 )
		{
		}

		SAnimationInfo( const int _nID, const NTimer::STime _nEndTime )
			: nID( _nID )
			, nEndTime( _nEndTime )
		{
		}
	};

	// Descending, so Update() can drain the soonest animation off the back.
	//
	// The id breaks ties. Two windows of one building commonly finish on the
	// same tick, std::sort leaves equal elements in an unspecified order, and
	// this vector is written to the save through IBinSaver, so without a
	// tiebreak the same damage could save as different bytes depending on the
	// standard library. The ids are unique within one holder, which makes this
	// a total order and the output independent of the algorithm. It is the
	// convention the eight sorts in Common_RTS_AI and AILogic/Shell.cpp
	// already follow.
	//
	// Not an ASYNC either way: this is a client-side process created from an
	// AI notification, its only effect is Scene()->RemoveObject, nothing reads
	// it back into the simulation, and the damage path that fills it picks its
	// slot with NWin32Random, so its contents are already client-local.
	struct SSortAnimationsByTime
	{
		bool operator()( const SAnimationInfo &anim1, const SAnimationInfo &anim2 ) const
		{
			if ( anim1.nEndTime != anim2.nEndTime )
			{
				return anim1.nEndTime > anim2.nEndTime;
			}
			return anim1.nID > anim2.nID;
		}
	};

	OBJECT_BASIC_METHODS( CDeadHouseAnimations );
	ZDATA
	std::vector<SAnimationInfo> animations;

ZEND public : int operator&( IBinSaver &f ) override
	{
		f.Add( 2, &animations );
		return 0;
	}

private:
private:
public:
	void Add( int nObjectID, const NTimer::STime &time, const NDb::SAnimB2 *pAnimation );
	void Sort();
	bool Update( const NTimer::STime &time ) override;

	const bool IsEmpty() const
	{
		return animations.empty();
	}
};
