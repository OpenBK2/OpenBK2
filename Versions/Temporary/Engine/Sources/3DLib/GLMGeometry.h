#pragma once

#include "3DLib/GGeometry.h"


#include "3DLib_export.h"

namespace NGScene
{

struct SLMQuad
{
	CTRect<int> quad; // exclusive right / down borders
	bool bFull; // if false only half of it is actually used
};

_3DLIB_EXPORT void MakeLMGeometry( CObjectInfo::SData *pRes, CTPoint<int> *pSize, const CObjectInfo::SData &src,
	float fLMResolution, int nLMSize, const CTPoint<int> &_shift );
_3DLIB_EXPORT void MakeLMCalcGeometry( CObjectInfo::SData *pRes, CTPoint<int> *pSize, const CObjectInfo::SData &src,
	float fLMResolution, int nLMSize, const CTPoint<int> &_shift, vector<SLMQuad> *pQuads );
_3DLIB_EXPORT void MakeSData( CObjectInfo::SData *pRes, const CObjectInfo &src );

}


