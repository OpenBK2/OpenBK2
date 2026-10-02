#pragma once

// Editor-only conversion core. Keeping database and window code out of this
// interface lets the exported skin and animation be checked without a renderer.
#include <gr2/granny.h>
#include <fastgltf/core.hpp>
#include <map>
#include <string>
#include <vector>

namespace NGrannyGltf
{
struct SClip
{
	const granny_animation *animation = nullptr;
	std::string name;
};

struct SModel
{
	std::string name;
	const granny_model *geometry = nullptr;
	bool mirrorX = false;
	// The Model's separate skeleton wins over the one embedded in Geometry.
	const granny_skeleton *skeleton = nullptr;
	struct SAIMesh
	{
		const granny_mesh *mesh = nullptr;
		const granny_skeleton *skeleton = nullptr;
	};
	std::vector<SAIMesh> aiMeshes;
	std::vector<SClip> clips;
	std::vector<int> materialQuantities;
	std::vector<int> meshAnimated;
};

struct SDocument
{
	fastgltf::Asset asset;
	std::vector<std::byte> binary;
	std::map<std::pair<fastgltf::Category, size_t>, std::string> extras;

	size_t AddBytes( const void *data, size_t size );
	size_t AddImage( const std::string &name, const std::vector<std::byte> &bytes, fastgltf::MimeType mime );
	std::vector<std::byte> Finish();
};

std::string Quote( const std::string &text );
void Convert( const SModel &source, SDocument *document );
}
