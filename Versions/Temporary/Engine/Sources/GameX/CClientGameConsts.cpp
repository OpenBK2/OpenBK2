#include "stdafx.h"
#include "CClientGameConsts.h"
#include "GetConsts.h"

IClientGameConsts* CreateClientGameConsts()
{
	return new CClientGameConsts();
}

const NDb::SClientGameConsts * CClientGameConsts::GetClientGameConsts()
{
	return NGameX::GetClientConsts();
}


