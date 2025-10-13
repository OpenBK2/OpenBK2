#pragma once

#include "SceneB2_export.h"

#include <cstdint>

struct IStatSystem : public CObjectBase
{
	enum { tidTypeID = 0x110AA3C0 };

	virtual void UpdateEntry( const std::string &szName, const std::string &szEntry, const uint32_t dwColor = 0xffffffff ) = 0;
};
SCENEB2_EXPORT IStatSystem *CreateStatSystem();


