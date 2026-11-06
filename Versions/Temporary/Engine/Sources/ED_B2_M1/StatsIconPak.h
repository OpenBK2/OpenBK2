#pragma once

#include "3Dmotor/GfxBuffers.h"
#include "MapEditorLib/IconRenderingOptions.h"
#include "Misc/2Darray.h"

#include <memory>
#include <string>

class CDBID;

// Builds an independent overlay without changing live database objects. The
// destination is replaced only once the complete ZIP has been finalized.
class CStatsIconPak
{
	struct SImpl;
	std::unique_ptr<SImpl> pImpl;

public:
	explicit CStatsIconPak( NIconRendering::EBatchType eType = NIconRendering::MECH_UNITS );
	~CStatsIconPak();
	bool Add( const CDBID &dbid,
	          const CArray2D<NGfx::SPixel8888> &capture,
	          const NIconRendering::SOptions &options,
	          std::string *pError );
	bool Save( const std::string &szDestination, std::string *pError );
};
