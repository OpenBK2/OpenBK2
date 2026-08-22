#pragma once
# include "../System/Dg.h"

namespace NGScene
{
class CObjectInfo;
CPtrFuncBase<CObjectInfo> *CreateDeformerHolder( CPtrFuncBase<CObjectInfo> *pGeom, const SFBTransform &place );
CPtrFuncBase<CObjectInfo> *CreateDeformerHolder( CPtrFuncBase<CObjectInfo> *pGeom, CFuncBase<SFBTransform> *pPlace );
CObjectBase *GetWindDeformerSource( CObjectBase *p );
}

