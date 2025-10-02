#pragma once

#include "SceneB2_export.h"


interface IStatSystem : public CObjectBase
{
	enum { tidTypeID = 0x110AA3C0 };

	virtual void UpdateEntry( const string &szName, const string &szEntry, const DWORD dwColor = 0xffffffff ) = 0;
};
SCENEB2_EXPORT IStatSystem *CreateStatSystem();


