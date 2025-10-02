#pragma once

#include "AIUpdates.h"
#include "DBMapInfo.h"

struct SAIReinfPointUpdate : public SAIBasicUpdate 
{
private:
	OBJECT_BASIC_METHODS( SAIReinfPointUpdate )
public:
	ZDATA_(SAIBasicUpdate)

		bool bEnable;
	int nPointID;
	NDb::SReinforcementPosition position;

	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SAIBasicUpdate*)this); f.Add(2,&bEnable); f.Add(3,&nPointID); f.Add(4,&position); return 0; }
public:
	SAIReinfPointUpdate() : bEnable( false ) {}
};

struct SAIAvailableReinfUpdate : public SAIBasicUpdate 
{
private:
	OBJECT_BASIC_METHODS( SAIAvailableReinfUpdate )
public:
	ZDATA_(SAIBasicUpdate)
	bool bEnabled;	
	CDBPtr<NDb::SReinforcement> pReinf;
	int nReinforcementCallsLeft;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SAIBasicUpdate*)this); f.Add(2,&bEnabled); f.Add(3,&pReinf); f.Add(4,&nReinforcementCallsLeft); return 0; }
public:
	SAIAvailableReinfUpdate() : bEnabled( false ), nReinforcementCallsLeft( 0 ) {}
};

struct SAIReinfRecycleUpdate : public SAIBasicUpdate 
{
private:
	OBJECT_BASIC_METHODS( SAIReinfRecycleUpdate )
public:
	ZDATA_(SAIBasicUpdate)
	bool bEnabled;	
	NTimer::STime timeRecycleEnd;
	float fProgress;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SAIBasicUpdate*)this); f.Add(2,&bEnabled); f.Add(3,&timeRecycleEnd); f.Add(4,&fProgress); return 0; }
public:
	SAIReinfRecycleUpdate() : bEnabled( false ), fProgress( 0.0f ), timeRecycleEnd( 0 ) {}
};


