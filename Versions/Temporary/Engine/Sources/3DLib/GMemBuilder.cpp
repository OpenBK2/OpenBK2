#include "stdafx.h"

#include "GMemBuilder.h"
#include "GMemFormat.h"
#include "MemObject.h"

namespace NGScene
{

CPtrFuncBase<CObjectInfo> *CreateObjectInfo( CMemObject *pO )
{
	return pO ? new CMemObjectInfo( pO->GetTris(), pO->GetPoints(), pO->GetNormals() ) : 0;
}

} // namespace NGScene

using namespace NGScene;
