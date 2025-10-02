#pragma once
#include "3DLib_export.h"


class CMemObject;
template <class TResult> class CPtrFuncBase;
namespace NGScene
{

class CMemTriList;
class CObjectInfo;
_3DLIB_EXPORT CPtrFuncBase<CObjectInfo>* CreateObjectInfo( CMemObject *pO );

} // namespace


