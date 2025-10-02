#pragma once
#include "../System/GResource.h"
#include "FontFormat.h"

namespace NGScene
{

class CFileFont: public CResourceLoader<int, CFontFormatInfo>
{
	OBJECT_BASIC_METHODS(CFileFont);
protected:
	virtual void Recalc();
};

}; // namespace 


