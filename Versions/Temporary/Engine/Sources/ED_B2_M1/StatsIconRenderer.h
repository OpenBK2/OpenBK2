#pragma once

#include "3Dmotor/GfxBuffers.h"
#include "MapEditorLib/IconRenderingOptions.h"
#include "Misc/2Darray.h"

#include <string>

class CDBID;

namespace NStatsIconRenderer
{
// Uses a private scene and camera, leaving the open document untouched.
bool Render( const CDBID &dbid,
             const NIconRendering::SOptions &options,
             CArray2D<NGfx::SPixel8888> *pCapture,
             std::string *pError );
} // namespace NStatsIconRenderer
