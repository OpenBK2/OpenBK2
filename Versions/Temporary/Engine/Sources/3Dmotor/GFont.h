#ifndef __GFONT_H__
#define __GFONT_H__
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "..\System\GResource.h"
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

#endif // __GFONT_H__
