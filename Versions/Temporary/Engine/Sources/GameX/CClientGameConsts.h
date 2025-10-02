#pragma once

#include "../Stats_B2_M1/IClientGameConsts.h"


class CClientGameConsts: public IClientGameConsts
{
	OBJECT_NOCOPY_METHODS( CClientGameConsts )
		ZDATA
public:
	ZEND int operator&( IBinSaver &f ) { return 0; }
public:
	CClientGameConsts(){};
	~CClientGameConsts(){};
	virtual  const NDb::SClientGameConsts * GetClientGameConsts();
};

IClientGameConsts* CreateClientGameConsts();


