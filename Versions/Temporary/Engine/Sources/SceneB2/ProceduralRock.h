#pragma once

#include "SceneB2_export.h"

#include <cstdint>

class CVec3;
struct SBound;
template <class T>
class CPtrFuncBase;

namespace NGScene
{
class CObjectInfo;
}

// Dimensions and optional tight bounds are in model space, with the base at ground level.
SCENEB2_EXPORT CPtrFuncBase<NGScene::CObjectInfo> *
CreateProceduralRock( const CVec3 &vSize, float fRoughness, uint32_t nSeed, SBound *pBound );
