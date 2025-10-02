#if !defined(__INTERACTIVE_MAYA_EXPORT_TOOL__)
#define __INTERACTIVE_MAYA_EXPORT_TOOL
#pragma once

#include "Interface_Exporter.h"
#include "InteractiveMaya.h"


class CInteractiveMayaExportTool : public IExportTool
{
	OBJECT_NOCOPY_METHODS( CInteractiveMayaExportTool );

public:
	virtual void StartExportTool()
	{
	}

	virtual void FinishExportTool()
	{
		CInteractiveMaya * pMayaProcess = CInteractiveMaya::Get();
		if ( pMayaProcess->IsStarted() )
		{
			pMayaProcess->Stop();
		}
	}
};

#endif //#define __INTERACTIVE_MAYA_EXPORT_TOOL

