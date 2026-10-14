#pragma once
#include "FontFormat.h"
#include "System/GResource.h"

namespace NGScene
{

class CFileFont : public CResourceLoader<int, CFontFormatInfo>
{
	OBJECT_BASIC_METHODS( CFileFont );

protected:
	void Recalc() override;
};

}; // namespace NGScene
