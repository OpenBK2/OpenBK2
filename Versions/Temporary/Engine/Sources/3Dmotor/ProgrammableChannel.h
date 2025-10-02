#pragma once

#include "../System/Time.hpp"
#include "..\System\DG.h"

namespace NAnimation
{

// Особенности:
//  1) Плохо подходит для большого количества сегментов
//  2) Сегменты поддерживают только константные значения
//  3) Пользователь должен сам заботиться о том, чтобы сегменты добавлялись в правильном временном порядке и не перекрывались
//
class CProgrammableChannel : public CFuncBase<float>
{
	OBJECT_NOCOPY_METHODS(CProgrammableChannel);

private:
	struct SSegment
	{
		ZDATA
			STime tStartTime;
		STime tEndTime;
		float fValue;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&tStartTime); f.Add(3,&tEndTime); f.Add(4,&fValue); return 0; }
	};
	ZDATA
		CDGPtr< CFuncBase<STime> > pTime;
	vector<SSegment> segments;
	float fDefaultValue;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pTime); f.Add(3,&segments); f.Add(4,&fDefaultValue); return 0; }

protected:
	CProgrammableChannel() : CFuncBase<float>(0.f), fDefaultValue(0.f) {}

	virtual bool NeedUpdate()
	{
		return IsValid(pTime) && pTime.Refresh();
	}
	virtual void Recalc()
	{
		STime time = pTime->GetValue();
		for ( int i = 0; i < segments.size(); ++i )
		{
			const SSegment &segment = segments[i];
			if ( segment.tEndTime > time && segment.tStartTime <= time )
			{
				value = segment.fValue;
				return;
			}
		}
		value = fDefaultValue;
	}

public:
	CProgrammableChannel( CFuncBase<STime> *_pTime, float _fDefaultValue = 0.f )
		: CFuncBase<float>(_fDefaultValue), pTime(_pTime), fDefaultValue(_fDefaultValue)
	{
		ASSERT(IsValid(pTime));
	}

	void SetDefaultValue( float fValue )
	{
		fDefaultValue = fValue;
		Updated();
	}

	void AddSegment( STime startTime, STime endTime, float fValue )
	{
		SSegment &segment = segments.push_back();
		segment.tStartTime = startTime;
		segment.tEndTime = endTime;
		segment.fValue = fValue;
		Updated();
	}

	bool HasSegments()
	{
		return !segments.empty();
	}
};
}
