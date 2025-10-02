#pragma	 once
#include "../Stats_B2_M1/DBClientConsts.h"

interface IClientGameConsts :  public CObjectBase
{
	enum { tidTypeID = 0x32168C00 };	
	virtual const NDb::SClientGameConsts *GetClientGameConsts() = 0;
};

