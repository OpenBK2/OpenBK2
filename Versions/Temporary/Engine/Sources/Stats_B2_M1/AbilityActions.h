#pragma once

#include "AIUpdates.h"
#include "SpecialAbilities.h"

struct SSpecialAbilityInfo : public SSuspendedUpdate
{
	ZDATA_( SSuspendedUpdate )
		int nAbilityType;							// ability type (NDb::EUnitSpecialAbility)
	float fCurValue;							// value for ability with progress bar
	SAbilitySwitchState state;    // new state for ability
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( SSuspendedUpdate *)this); f.Add(2,&nAbilityType); f.Add(3,&fCurValue); f.Add(4,&state); return 0; }
public:
	SSpecialAbilityInfo() : nAbilityType( 0 ), fCurValue( 0.0f ), state( (EAbilitySwitchState)0 ) {  }
};

struct SAISpecialAbilityUpdate : public SAIBasicUpdate
{
private:
	OBJECT_BASIC_METHODS(SAISpecialAbilityUpdate)
public:
	ZDATA_(SAIBasicUpdate)
		SSpecialAbilityInfo info;
	ZEND public: int operator&( IBinSaver &f ) { f.Add(1,(SAIBasicUpdate*)this); f.Add(2,&info); return 0; }
};

