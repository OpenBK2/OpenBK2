#if !defined( __INTERACTIVE_MAYA_EXPORT_TOOL__ )
#define __INTERACTIVE_MAYA_EXPORT_TOOL
#pragma once

#include "InteractiveMaya.h"
#include "Interface_Exporter.h"

class CInteractiveMayaExportTool : public IExportTool
{
	OBJECT_NOCOPY_METHODS( CInteractiveMayaExportTool );

public:
	void StartExportTool() override {}

	void FinishExportTool() override
	{
		CInteractiveMaya *pMayaProcess = CInteractiveMaya::Get();
		if ( pMayaProcess->IsStarted() )
		{
			pMayaProcess->Stop();
		}
	}
};

#endif //#define __INTERACTIVE_MAYA_EXPORT_TOOL
