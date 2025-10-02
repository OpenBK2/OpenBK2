#pragma once

#include "MapEditorLib/Tools_MnemonicsCollector.h"

class CMayaWeaponMnemonics : public CMnemonicsCollector<int>
{
	public:
	CMayaWeaponMnemonics();
};

class CWeaponMnemonics : public CMnemonicsCollector<int>
{
public:
	CWeaponMnemonics();
};

extern CMayaWeaponMnemonics typeMayaWeaponMnemonics;
extern CWeaponMnemonics typeWeaponMnemonics;


