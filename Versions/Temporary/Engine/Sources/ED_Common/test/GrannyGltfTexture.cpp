#include "../GrannyGltfTexture.h"
#include <wx/image.h>
#include <wx/imagpng.h>
#include <wx/imagtga.h>
#include <wx/mstream.h>
#include <gtest/gtest.h>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>

TEST( GrannyGltfTexture, Dxt1PreviewPreservesColorAndPunchThroughAlpha )
{
	std::vector<std::byte> dds(136);
	std::memcpy(dds.data(), "DDS ", 4);
	auto field = [&](size_t offset, uint32_t value)
	{
		for ( int b = 0; b < 4; ++b ) dds[offset + b] = std::byte((value >> (b * 8)) & 255);
	};
	field(4,124); field(12,4); field(16,4); field(76,32); field(80,4); field(84,0x31545844);
	dds[130] = dds[131] = std::byte{255};
	for ( int b = 132; b < 136; ++b ) dds[b] = std::byte{0xe4};
	const auto png = NGrannyGltf::DdsPreview(dds);
	wxMemoryInputStream input(png.data(), png.size());
	wxPNGHandler decoder;
	wxImage image;
	ASSERT_TRUE(decoder.LoadFile(&image, input, false));
	ASSERT_EQ(image.GetWidth(), 4); ASSERT_EQ(image.GetHeight(), 4);
	EXPECT_EQ(image.GetRed(0,0), 0); EXPECT_EQ(image.GetRed(1,0), 255);
	EXPECT_EQ(image.GetAlpha(0,0), 255); EXPECT_EQ(image.GetAlpha(3,0), 0);
	dds.resize(135);
	EXPECT_THROW(NGrannyGltf::DdsPreview(dds), std::runtime_error);
}

TEST( GrannyGltfTexture, SavesAndReplacesUnicodeDestination )
{
	const auto filename = "obk2-gr2-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + "-\xc4\x8d.glb";
	const auto path = std::filesystem::temp_directory_path() / std::filesystem::u8path(filename);
	const std::vector<std::byte> first = {std::byte{1}, std::byte{2}}, second = {std::byte{3}};
	NGrannyGltf::SaveGlb(path.u8string(), first);
	NGrannyGltf::SaveGlb(path.u8string(), second);
	std::ifstream input(path, std::ios::binary);
	EXPECT_EQ(input.get(), 3); EXPECT_EQ(input.get(), EOF);
	input.close();
	std::filesystem::remove(path);
}

TEST( GrannyGltfTexture, TgaPreservesRowOrderColorsAndAlpha )
{
	// A non-square, uncompressed DDS with different colors on each row catches
	// TGA origin/BGRA mistakes that a uniform DXT block would not expose.
	std::vector<std::byte> dds(128 + 24);
	std::memcpy(dds.data(), "DDS ", 4);
	auto field = [&](size_t offset, uint32_t value)
	{
		for ( int b = 0; b < 4; ++b ) dds[offset + b] = std::byte((value >> (b * 8)) & 255);
	};
	field(4,124); field(12,3); field(16,2); field(76,32); field(80,0x41); field(88,32);
	field(92,0xff); field(96,0xff00); field(100,0xff0000); field(104,0xff000000);
	for ( int i = 0; i < 6; ++i ) field(128 + i * 4, uint32_t(10 + i) | (uint32_t(20 + i) << 8) | (uint32_t(30 + i) << 16) | (uint32_t(i * 50) << 24));
	const auto tga = NGrannyGltf::DdsPreview(dds, true);
	wxMemoryInputStream input(tga.data(), tga.size());
	wxTGAHandler decoder;
	wxImage image;
	ASSERT_TRUE(decoder.LoadFile(&image, input, false));
	ASSERT_EQ(image.GetWidth(), 2); ASSERT_EQ(image.GetHeight(), 3);
	for ( int y = 0; y < 3; ++y )
		for ( int x = 0; x < 2; ++x )
		{
			const int i = y * 2 + x;
			EXPECT_EQ(image.GetRed(x,y), 10 + i);
			EXPECT_EQ(image.GetGreen(x,y), 20 + i);
			EXPECT_EQ(image.GetBlue(x,y), 30 + i);
			EXPECT_EQ(image.GetAlpha(x,y), i * 50);
		}
}
