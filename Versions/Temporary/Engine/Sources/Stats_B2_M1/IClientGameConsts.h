#pragma once
#include "Stats_B2_M1/DBClientConsts.h"

struct IClientGameConsts :  public CObjectBase
{
	enum { tidTypeID = 0x32168C00 };	
	virtual const NDb::SClientGameConsts *GetClientGameConsts() = 0;
};

