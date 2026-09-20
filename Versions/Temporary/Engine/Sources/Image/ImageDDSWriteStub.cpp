#include "stdafx.h"

#include "ImageDDS.h"

#include "System/LogStream.h"

// The DDS writer off Windows, which does not write anything yet.
//
// ImageDDSWrite.cpp, the real one, is built on Windows only, and its whole
// dependency on Direct3D is two D3DX calls: D3DXLoadSurfaceFromMemory to turn
// the ARGB image into the destination format, which is where DXT compression
// happens, and D3DXSaveTextureToFile to write the .dds. DXVK implements
// neither -- cmake/dxvk.cmake says so outright, "there is no dxvk::d3dx9 and
// there will not be" -- so the file does not link off Windows and this stands
// in for it.
//
// **TODO: write the DDS properly.** The job is smaller than it looks and needs
// no GPU at all. The function it replaces creates a D3DDEVTYPE_NULLREF device,
// a null reference device that renders nothing, purely to have something to
// call CreateTexture on; its own comment says "exporting is a CPU surface
// conversion". So Direct3D is being used here as a texture compressor and a
// file writer, and the replacement is:
//
//   1. a BC1/BC2/BC3 compressor, which is the only real work. stb_dxt.h is
//      single file and public domain and covers exactly these formats.
//   2. a DDS writer, which is a 128 byte header and the mip data after it.
//      Image/DDS.h already describes that header, since the tree reads DDS.
//
// Everything else in the Windows file is already portable: the mip chain is
// built with the tree's own Lanczos in ImageScale.cpp.
//
// Whoever picks this up should first check which EPixelFormat values
// ED_Common/TextureExporter.cpp actually passes. If they are the DXT formats
// and uncompressed ARGB, the above covers it; anything odder changes the
// estimate. The check is a diff against what a Windows build writes for the
// same source image, which makes this verifiable without a viewport.
namespace NImage
{

bool ConvertAndSaveAsDDSWithDX( IDirect3DDevice9 *, const std::string &szFileName, const CArray2D<uint32_t> &,
																EImageType, NGfx::EPixelFormat, int, bool, bool, float )
{
	// Loudly, and every time. The caller turns a false into ER_FAIL without
	// saying anything of its own, so silence here would look like a texture
	// that simply refused to export.
	csSystem << CC_RED << "DDS export is not implemented off Windows yet: " << szFileName.c_str()
					 << " was not written. See Image/ImageDDSWriteStub.cpp." << endl;
	return false;
}

}
