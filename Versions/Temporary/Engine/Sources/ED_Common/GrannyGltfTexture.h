#pragma once
#include <cstddef>
#include <vector>
#include <string>

namespace NGrannyGltf
{
// Lossless PNG or TGA representation of the first DDS mip, including alpha.
std::vector<std::byte> DdsPreview( const std::vector<std::byte> &dds, bool tga = false );
struct SExportFile
{
	std::string path;
	std::vector<std::byte> bytes;
};
// Kept outside database translation units: wx undefines Win32's GetObject,
// which is still part of libdb's compiled Windows interface.
// Stage all files before replacing any; callers put the GLB last.
void SaveExportFiles( const std::vector<SExportFile> &files );
void SaveGlb( const std::string &path, const std::vector<std::byte> &bytes );
}
