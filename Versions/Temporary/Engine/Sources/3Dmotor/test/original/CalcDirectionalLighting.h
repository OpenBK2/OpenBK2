#pragma once

// Reference implementation of CalcDirectionalLighting: the C++ loop around the
// original MMX inner block, which now lives in CalcDirectionalLighting.asm rather
// than an __asm statement, so this builds on x64 as well as x86.

#include "3Dmotor/GLightPerVertex.h"
#include "3DLib/GGeometry.h"

#include <cstdint>
#include <vector>

extern "C" uint64_t CalcDirectionalLightingMMX(
    const void *pDirData,
    const NGfx::SMMXWord *pTranslucentShade,
    uint32_t dwNormal );

namespace original
{

static void ConvertColor( NGfx::SMMXWord *p, const CVec3 &v )
{
    p->nZ = Float2Int( v.z * 0x4000 );
    p->nY = Float2Int( v.y * 0x4000 );
    p->nX = Float2Int( v.x * 0x4000 );
    p->nW = 0;
}

static CVec3 MulPerComp( const CVec3 &a, const CVec3 &b )
{
    return CVec3( a.x * b.x, a.y * b.y, a.z * b.z );
}

static void CalcDirectionalLighting(
    const std::vector<WORD> &posIndices,
    const std::vector<NGfx::SCompactVector> &_normals,
    const NGScene::SPerVertexLightState &ls, bool bTranslucent, const CVec3 &vTranslucentColor,
    std::vector<DWORD> *pResColors, std::vector<DWORD> *pResShadow )
{
    pResColors->resize( posIndices.size() );
    pResShadow->resize( posIndices.size() );
    DWORD dwColor = 0, dwShadowColor = 0, dwPrevNormal = 0;
    const void *pDirData = &ls.ambient;
    // transHolder has to outlive the loop; an earlier sketch of this scoped it inside
    // the if and left pTranslucentShade dangling.
    NGfx::SMMXWord transHolder{};
    const NGfx::SMMXWord *pTranslucentShade = &ls.shadeColor;
    if ( bTranslucent )
    {
        ConvertColor( &transHolder, MulPerComp( ls.vLightColor, vTranslucentColor ) );
        pTranslucentShade = &transHolder;
    }
    for ( int k = 0; k < posIndices.size(); ++k )
    {
        DWORD dwNormal = _normals[k].dw;
        if ( dwNormal != dwPrevNormal )
        {
            const uint64_t nResult = CalcDirectionalLightingMMX( pDirData, pTranslucentShade, dwNormal );
            dwColor = static_cast<DWORD>( nResult );
            dwShadowColor = static_cast<DWORD>( nResult >> 32 );
        }
        (*pResColors)[k] = dwColor;
        (*pResShadow)[k] = dwShadowColor;
        dwPrevNormal = dwNormal;
    }
}

}
