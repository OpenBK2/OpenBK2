#include "StdAfx.h"

#include "DefaultBuilder.h"
#include "../MapEditorLib/BuilderFactory.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


REGISTER_BUILDER_IN_DLL( DEFAULT_BUILDER_LABEL, CDefaultBuilder )


