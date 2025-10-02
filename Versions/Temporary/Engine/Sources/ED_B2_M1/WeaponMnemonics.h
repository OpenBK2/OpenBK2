#if !defined(__WEAPON_MNEMONICS__)
#define __WEAPON_MNEMONICS__
#pragma once

#include "..\MapEditorLib\Tools_MnemonicsCollector.h"

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

#endif // !defined(__WEAPON_MNEMONICS__)

