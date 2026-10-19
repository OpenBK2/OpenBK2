#pragma once
#include <cstddef>
#include <string>
#include <vector>

namespace NGrannyGltf
{
// Lossless PNG or TGA representation of the first DDS mip, including alpha.
std::vector<std::byte> DdsPreview( const std::vector<std::byte> &dds, bool tga = false );

struct SExportFile
{
	std::string path;
	std::vector<std::byte> bytes;
};

// Kept outside database translation units: wx undefines windows.h's A/W
// macros, which turns the Win32 calls those units spell by macro name wide.
// Stage all files before replacing any; callers put the GLB last.
void SaveExportFiles( const std::vector<SExportFile> &files );
void SaveGlb( const std::string &path, const std::vector<std::byte> &bytes );
} // namespace NGrannyGltf
