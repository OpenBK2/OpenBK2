#include "stdafx.h"
#include "GrannyModelExport.h"
#include "GrannyGltf.h"
#include "GrannyGltfTexture.h"
#include "3Dmotor/DBScene.h"
#include "libdb/Manipulator.h"
#include "libdb/ObjMan.h"
#include "MapEditorLib/Interface_Logger.h"
#include "System/BinaryResources.h"
#include "System/VFSOperations.h"
#include <filesystem>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>

namespace NEditorGltf
{
namespace
{
std::vector<std::byte> Read( const std::string &path )
{
	CFileStream stream(NVFS::GetMainVFS(), path);
	if ( !stream.IsOk() || !stream.CanRead() || stream.GetSize() <= 0 )
		throw std::runtime_error("Cannot read game resource: " + path);
	std::vector<std::byte> bytes(stream.GetSize());
	stream.Read(bytes.data(), bytes.size());
	if ( !stream.IsOk() ) throw std::runtime_error("Incomplete game resource: " + path);
	return bytes;
}

struct SFiles
{
	using TFile = std::unique_ptr<granny_file, decltype(&GrannyFreeFile)>;
	std::vector<TFile> files;
	std::map<std::string, granny_file_info *> cache;

	template<class T> granny_file_info *Load( const char *folder, const T *resource )
	{
		const std::string path = NBinResources::GetExistentBinaryFileName(std::string("bin/") + folder,
			resource->GetRecordID(), resource->uid);
		const auto old = cache.find(path);
		if ( old != cache.end() ) return old->second;
		const auto bytes = Read(path);
		TFile file(GrannyReadEntireFileFromMemory(static_cast<int>(bytes.size()), bytes.data()), GrannyFreeFile);
		auto *info = file ? GrannyGetFileInfo(file.get()) : nullptr;
		if ( !info ) throw std::runtime_error("Cannot decode Granny3D resource: " + path);
		files.push_back(std::move(file));
		cache.emplace(path, info);
		return info;
	}
};

const granny_model *FirstModel( const granny_file_info *info )
{
	if ( !info || info->ModelCount < 1 || !info->Models || !info->Models[0] )
		throw std::runtime_error("Granny3D resource contains no model");
	return info->Models[0];
}

void RequireGranny( const std::string &reference )
{
	if ( !reference.empty() ) throw std::runtime_error("This command exports legacy Granny3D resources. The model contains a GLB/GLTF reference: " + reference);
}

std::string FileName( const std::string &text )
{
	// Reversible ASCII escaping gives portable file/URI names even for UTF-8,
	// spaces, '#' and '%'. Escaping '_' too prevents escaped-name collisions.
	std::string result;
	const char digits[] = "0123456789ABCDEF";
	for ( unsigned char c : text )
		if ( (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '.' ) result += c;
		else { result += '_'; result += digits[c >> 4]; result += digits[c & 15]; }
	return result;
}

struct SMaterials
{
	NGrannyGltf::SDocument *doc;
	const SGrannyExportOptions &options;
	std::filesystem::path destination;
	std::map<std::string, size_t> textures;
	std::vector<NGrannyGltf::SExportFile> files;

	size_t Image( const std::string &name, std::vector<std::byte> bytes, fastgltf::MimeType mime, const char *extension )
	{
		if ( !options.separateTextures ) return doc->AddImage(name, bytes, mime);
		const size_t slash = name.find_last_of("/\\");
		std::string stem = name.substr(slash == std::string::npos ? 0 : slash + 1);
		stem = stem.substr(0, stem.find_last_of('.'));
		const auto folder = std::filesystem::u8path(FileName(destination.stem().u8string()) + "_textures");
		const auto relative = folder / (std::to_string(textures.size()) + "_" + FileName(stem) + extension);
		fastgltf::Image image;
		image.name = name;
		// TGA uses a file URI without a MIME type: Blender's ordinary image
		// loader handles it. Embedded images still use standard PNG + DDS.
		image.data = fastgltf::sources::URI{0, fastgltf::URI(relative.generic_u8string()), mime};
		files.push_back({(destination.parent_path() / relative).u8string(), std::move(bytes)});
		doc->asset.images.push_back(std::move(image));
		return doc->asset.images.size() - 1;
	}

	size_t Texture( const NDb::STexture *source )
	{
		const std::string key = NDb::GetFileName(source->GetDBID());
		const auto old = textures.find(key);
		if ( old != textures.end() ) return old->second;
		const std::string path = source->szDestName;
		auto dds = Read(path);
		const bool tga = options.separateTextures && options.convertTexturesToTga;
		std::vector<std::byte> preview;
		try { preview = NGrannyGltf::DdsPreview(dds, tga); }
		catch ( const std::exception &e ) { throw std::runtime_error(path + ": " + e.what()); }
		fastgltf::Sampler sampler;
		sampler.wrapS = (source->eAddrType == NDb::STexture::CLAMP || source->eAddrType == NDb::STexture::WRAP_Y)
			? fastgltf::Wrap::ClampToEdge : fastgltf::Wrap::Repeat;
		sampler.wrapT = (source->eAddrType == NDb::STexture::CLAMP || source->eAddrType == NDb::STexture::WRAP_X)
			? fastgltf::Wrap::ClampToEdge : fastgltf::Wrap::Repeat;
		fastgltf::Texture texture;
		texture.name = path;
		texture.samplerIndex = doc->asset.samplers.size();
		doc->asset.samplers.push_back(std::move(sampler));
		texture.imageIndex = Image(path, std::move(preview), tga ? fastgltf::MimeType::None : fastgltf::MimeType::PNG, tga ? ".tga" : ".png");
		if ( !tga )
		{
			texture.ddsImageIndex = Image(path, std::move(dds), fastgltf::MimeType::DDS, ".dds");
			if ( textures.empty() ) doc->asset.extensionsUsed.push_back("MSFT_texture_dds");
		}
		const size_t index = doc->asset.textures.size();
		doc->asset.textures.push_back(std::move(texture));
		textures.emplace(key, index);
		return index;
	}

	void Add( const NDb::SMaterial *source )
	{
		fastgltf::Material material;
		material.pbrData.metallicFactor = 0;
		material.pbrData.roughnessFactor = 1;
		std::string extras = "{";
		if ( source )
		{
			material.name = NDb::GetFileName(source->GetDBID());
			material.doubleSided = source->bIs2Sided;
			if ( source->eAlphaMode == NDb::SMaterial::AM_ALPHA_TEST ) material.alphaMode = fastgltf::AlphaMode::Mask;
			else if ( source->eAlphaMode != NDb::SMaterial::AM_OPAQUE ) material.alphaMode = fastgltf::AlphaMode::Blend;
			if ( source->pTexture )
			{
				fastgltf::TextureInfo texture;
				texture.textureIndex = Texture(source->pTexture);
				material.pbrData.baseColorTexture = std::move(texture);
			}
			// Legacy bump/gloss/mirror/detail maps are not glTF metallic-roughness
			// maps. Preserve them and their roles instead of wiring incorrect PBR inputs.
			auto preserve = [&](const char *role, const NDb::STexture *texture)
			{
				if ( !texture ) return;
				if ( extras.size() > 1 ) extras += ',';
				extras += NGrannyGltf::Quote(role) + ':' + std::to_string(Texture(texture));
			};
			preserve("Bump", source->pBump);
			preserve("Gloss", source->pGloss);
			preserve("Mirror", source->pMirror);
			preserve("DetailTexture", source->pDetailTexture);
		}
		extras += '}';
		doc->extras[{fastgltf::Category::Materials, doc->asset.materials.size()}] = extras;
		doc->asset.materials.push_back(std::move(material));
	}
};
}

bool ExportGrannyModel( IManipulator *resource, const std::string &destination,
	const SGrannyExportOptions &options, std::string *error )
{
	const auto *model = resource && resource->GetObjMan()
		? dynamic_cast<const NDb::SModel *>(resource->GetObjMan()->GetObject()) : nullptr;
	return ExportGrannyModel(model, destination, options, error);
}

bool ExportGrannyModel( const NDb::SModel *model, const std::string &destination,
	const SGrannyExportOptions &options, std::string *error )
{
	try
	{
		if ( !model || !model->pGeometry ) throw std::runtime_error("Select a Model with a Geometry reference");
		RequireGranny(model->pGeometry->szModelFileRef);
		SFiles files;
		NGrannyGltf::SModel input;
		input.mirrorX = options.mirrorX;
		input.name = NDb::GetFileName(model->GetDBID());
		input.geometry = FirstModel(files.Load("Geometries", model->pGeometry.GetPtr()));
		input.materialQuantities = model->pGeometry->materialQuantities;
		input.meshAnimated = model->pGeometry->meshAnimated;
		if ( model->pSkeleton )
		{
			RequireGranny(model->pSkeleton->szModelFileRef);
			input.skeleton = FirstModel(files.Load("Skeletons", model->pSkeleton.GetPtr()))->Skeleton;
			if ( !input.skeleton ) throw std::runtime_error("Model's Skeleton resource has no skeleton");
		}
		if ( model->pGeometry->pAIGeometry )
		{
			const auto *ai = model->pGeometry->pAIGeometry.GetPtr();
			RequireGranny(ai->szModelFileRef);
			const auto *info = files.Load("AIGeometries", ai);
			if ( info->MeshCount < 1 || !info->Meshes ) throw std::runtime_error("AIGeometry contains no meshes");
			for ( int i = 0; i < info->MeshCount; ++i )
			{
				const granny_skeleton *skeleton = nullptr;
				// As in CFileSkinPointsLoadFromGranny, collision meshes bind to
				// their owning GR2 model, which can differ from the render rig.
				for ( int m = 0; m < info->ModelCount && !skeleton; ++m )
					for ( int b = 0; b < info->Models[m]->MeshBindingCount; ++b )
						if ( info->Models[m]->MeshBindings[b].Mesh == info->Meshes[i] ) skeleton = info->Models[m]->Skeleton;
				input.aiMeshes.push_back({info->Meshes[i], skeleton});
			}
		}
		std::set<std::string> seen;
		auto animations = [&](const auto &references, bool additional)
		{
			for ( const auto &reference : references )
			{
				if ( !reference ) continue;
				RequireGranny(reference->GetModelFileRef());
				const std::string name = NDb::GetFileName(reference->GetDBID());
				const bool first = seen.insert(name).second;
				if ( additional && !first ) continue;
				const auto *info = files.Load("Animations", reference.GetPtr());
				if ( info->AnimationCount < 1 || !info->Animations ) throw std::runtime_error("No Granny3D animation in " + name);
				// AnimB2 references address animation zero, as SAnimHandle does.
				input.clips.push_back({info->Animations[0], name});
			}
		};
		// Preserve the Model list's order, then append only new skeleton clips.
		animations(model->animations, false);
		if ( model->pSkeleton ) animations(model->pSkeleton->animations, true);
		NGrannyGltf::SDocument doc;
		SMaterials materials{&doc, options, std::filesystem::u8path(destination)};
		for ( const auto &material : model->materials ) materials.Add(material);
		NGrannyGltf::Convert(input, &doc);
		auto bytes = doc.Finish();
		if ( !materials.files.empty() )
			std::filesystem::create_directories(std::filesystem::u8path(materials.files.front().path).parent_path());
		materials.files.push_back({destination, std::move(bytes)});
		NGrannyGltf::SaveExportFiles(materials.files);
		NLog::Log(LT_NORMAL, "Exported Granny3D model %.450s to %.450s\n", input.name.c_str(), destination.c_str());
		return true;
	}
	catch ( const std::exception &failure )
	{
		if ( error ) *error = failure.what();
		NLog::Log(LT_ERROR, "Granny3D to GLB export failed: %.950s\n", failure.what());
		return false;
	}
}
}
