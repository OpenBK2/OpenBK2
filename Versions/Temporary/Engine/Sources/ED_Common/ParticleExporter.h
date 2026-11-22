#pragma once

#include "StoreObjectExporter.h"

#include "ED_Common_export.h"

#include <string>

namespace NVFS
{
struct IFileCreator;
}

class CParticleExporter : public CStoreObjectExporter
{
	OBJECT_NOCOPY_METHODS( CParticleExporter );

public:
	// Empty on success. Batch callers can report the error without opening a dialog.
	static ED_COMMON_EXPORT std::string
	ImportClip( IManipulator *pManipulator, const std::string &source, NVFS::IFileCreator *destination, bool force );

	void FinishExport( const std::string &, bool ) override {}

	EXPORT_RESULT ExportObject( IManipulator *pManipulator,
	                            const std::string &rszObjectTypeName,
	                            const std::string &rszObjectName,
	                            bool bForce,
	                            EXPORT_TYPE exportType ) override;
};
