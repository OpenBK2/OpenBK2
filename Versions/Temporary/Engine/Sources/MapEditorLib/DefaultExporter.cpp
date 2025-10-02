#include "StdAfx.h"
#include "DefaultExporter.h"
#include "ExporterFactory.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


REGISTER_EXPORTER_IN_DLL( DEFAULT_EXPORTER_LABEL, CDefaultExporter )


