#pragma once

#include "Stats_B2_M1/IClientGameConsts.h"

class CClientGameConsts : public IClientGameConsts
{
	OBJECT_NOCOPY_METHODS( CClientGameConsts )
	ZDATA
public:
	ZEND int operator&( IBinSaver &f ) override
	{
		return 0;
	}

public:
	CClientGameConsts() {}

	~CClientGameConsts() override {}

	const NDb::SClientGameConsts *GetClientGameConsts() override;
};

IClientGameConsts *CreateClientGameConsts();
