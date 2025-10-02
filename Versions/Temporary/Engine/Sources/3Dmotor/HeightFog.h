#pragma once

# include "..\System\DG.h"

namespace NDb
{
	struct SHeightFog;
}

namespace NGScene
{

class CObjectInfo;

CPtrFuncBase<CObjectInfo> *CreateHeightFogHolder( CPtrFuncBase<CObjectInfo> *pGeom, const NDb::SHeightFog *pHeightFog,
																								  const SFBTransform &place );

CPtrFuncBase<CObjectInfo> *CreateHeightFogHolder( CPtrFuncBase<CObjectInfo> *pGeom, const NDb::SHeightFog *pHeightFog,
																								  CFuncBase<SFBTransform> *pPlace );

} // namespace NGScene


