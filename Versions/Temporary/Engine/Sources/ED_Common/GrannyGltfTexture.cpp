#include "GrannyGltfTexture.h"
#include <squish/squish.h>
#include <wx/image.h>
#include <wx/file.h>
#include <wx/imagpng.h>
#include <wx/imagtga.h>
#include <wx/mstream.h>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <memory>
#include <stdexcept>

namespace NGrannyGltf
{
namespace
{
void Check( bool valid )
{
	if ( !valid ) throw std::runtime_error("Unsupported or incomplete DDS texture");
}
uint32_t U32( const std::byte *p )
{
	return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}
uint8_t Channel( uint32_t pixel, uint32_t mask, uint8_t missing )
{
	if ( !mask ) return missing;
	while ( !(mask & 1) ) { mask >>= 1; pixel >>= 1; }
	Check((uint64_t(mask) & (uint64_t(mask) + 1)) == 0);
	return static_cast<uint8_t>((uint64_t(pixel & mask) * 255 + mask / 2) / mask);
}
}

std::vector<std::byte> DdsPreview( const std::vector<std::byte> &dds, bool tga )
{
	Check(dds.size() >= 128 && std::memcmp(dds.data(), "DDS ", 4) == 0);
	auto field = [&](size_t offset) { return U32(dds.data() + offset); };
	Check(field(4) == 124 && field(76) == 32);
	const uint32_t width = field(16), height = field(12);
	Check(width > 0 && height > 0 && width <= 16384 && height <= 16384 && uint64_t(width) * height <= 64 * 1024 * 1024);
	std::vector<uint8_t> rgba(size_t(width) * height * 4);
	if ( field(80) & 4 )
	{
		const uint32_t fourcc = field(84);
		const bool dxt1 = fourcc == 0x31545844;
		const bool dxt3 = fourcc == 0x33545844 || fourcc == 0x32545844;
		const bool dxt5 = fourcc == 0x35545844 || fourcc == 0x34545844;
		Check(dxt1 || dxt3 || dxt5);
		Check(dds.size() - 128 >= size_t((width + 3) / 4) * ((height + 3) / 4) * (dxt1 ? 8 : 16));
		// squish's decoder preserves BC1 punch-through alpha and edge blocks;
		// the renderer's older DXT unpacker is not a general image decoder.
		squish::DecompressImage(rgba.data(), width, height, dds.data() + 128,
			dxt1 ? squish::kDxt1 : dxt3 ? squish::kDxt3 : squish::kDxt5);
		if ( fourcc == 0x32545844 || fourcc == 0x34545844 )
			for ( size_t i = 0; i < rgba.size(); i += 4 )
				for ( int c = 0; c < 3 && rgba[i + 3]; ++c )
					rgba[i + c] = static_cast<uint8_t>(std::min(255u, unsigned(rgba[i + c]) * 255 / rgba[i + 3]));
	}
	else
	{
		const uint32_t bits = field(88);
		Check((field(80) & 0x40) && (bits == 16 || bits == 24 || bits == 32));
		const size_t rowBytes = size_t(width) * (bits / 8);
		const size_t pitch = (field(8) & 8) && field(20) ? field(20) : rowBytes;
		Check(pitch >= rowBytes && pitch <= (dds.size() - 128) / height);
		for ( size_t y = 0; y < height; ++y )
			for ( size_t x = 0; x < width; ++x )
			{
				uint32_t pixel = 0;
				for ( uint32_t b = 0; b < bits / 8; ++b ) pixel |= uint32_t(dds[128 + y * pitch + x * (bits / 8) + b]) << (b * 8);
				const size_t offset = (y * width + x) * 4;
				rgba[offset] = Channel(pixel, field(92), 0);
				rgba[offset + 1] = Channel(pixel, field(96), 0);
				rgba[offset + 2] = Channel(pixel, field(100), 0);
				rgba[offset + 3] = Channel(pixel, (field(80) & 1) ? field(104) : 0, 255);
			}
	}
	wxImage image(width, height, false);
	Check(image.IsOk());
	image.InitAlpha();
	for ( size_t i = 0; i < size_t(width) * height; ++i )
	{
		std::memcpy(image.GetData() + i * 3, rgba.data() + i * 4, 3);
		image.GetAlpha()[i] = rgba[i * 4 + 3];
	}
	wxMemoryOutputStream output;
	wxPNGHandler png;
	wxTGAHandler targa;
	wxImageHandler &encoder = tga ? static_cast<wxImageHandler &>(targa) : static_cast<wxImageHandler &>(png);
	if ( !encoder.SaveFile(&image, output, false) ) throw std::runtime_error("Cannot encode texture preview");
	std::vector<std::byte> bytes(output.GetSize());
	output.CopyTo(bytes.data(), bytes.size());
	return bytes;
}

void SaveExportFiles( const std::vector<SExportFile> &files )
{
	// Write all temporary files first, so a failed texture/GLB write leaves the
	// existing export intact. Commit the GLB only after its textures are saved.
	std::vector<std::unique_ptr<wxTempFile>> staged;
	for ( const auto &file : files )
	{
		auto output = std::make_unique<wxTempFile>(wxString::FromUTF8(file.path.c_str()));
		if ( !output->IsOpened() || !output->Write(file.bytes.data(), file.bytes.size()) )
			throw std::runtime_error("Cannot write export file: " + file.path);
		staged.push_back(std::move(output));
	}
	for ( size_t i = 0; i < files.size(); ++i )
		if ( !staged[i]->Commit() ) throw std::runtime_error("Cannot replace export file: " + files[i].path);
}

void SaveGlb( const std::string &path, const std::vector<std::byte> &bytes )
{
	SaveExportFiles({{path, bytes}});
}
}
