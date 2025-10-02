#include "StdAfx.h"
#include "MemObject.h"
#include "GMemFormat.h"
#include "GMemBuilder.h"

namespace NGScene
{

CPtrFuncBase<CObjectInfo>* CreateObjectInfo( CMemObject *pO )
{
	return pO ? new CMemObjectInfo( pO->GetTris(), pO->GetPoints(), pO->GetNormals() ) : 0;
}

} // namespace

using namespace NGScene;
