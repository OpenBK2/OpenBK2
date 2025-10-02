#pragma once

struct IClientUpdatableProcess : public CObjectBase
{
	virtual bool Update( const NTimer::STime &time ) = 0;
};

namespace NUpdatableProcess
{
	void Register( IClientUpdatableProcess *pProcess );
}


