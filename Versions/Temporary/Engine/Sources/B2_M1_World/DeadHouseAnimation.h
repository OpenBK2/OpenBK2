#pragma once

// Defensive member defaults; explicit constructor values still take precedence.

#include "UpdatableProcess.h"
#include "Stats_B2_M1/DBAnimB2.h"

class CDeadHouseAnimation : public IClientUpdatableProcess
{
	OBJECT_BASIC_METHODS( CDeadHouseAnimation )
	
	ZDATA
	int nID = 0;
	NTimer::STime nEndTime = 0;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nID); f.Add(3,&nEndTime); return 0; }
private:
public:
	void Init( int nObjectID, const NTimer::STime &time, const NDb::SAnimB2 *pAnimation );
	bool Update( const NTimer::STime &time );
};


