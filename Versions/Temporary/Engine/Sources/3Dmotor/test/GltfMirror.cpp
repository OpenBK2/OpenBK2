// Exercise the real file loader and animator without game data or a graphics device.
#include "3Dmotor/stdafx.h"
#include "3Dmotor/DBScene.h"
#include "3Dmotor/GAnimation.hpp"
#include "3Dmotor/GltfFormat.h"
#include "System/VFSOperations.h"

#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <gtest/gtest.h>
#include <map>

namespace
{
using namespace fastgltf::math;

class CModelVFS : public NVFS::IVFS
{
	OBJECT_NOCOPY_METHODS(CModelVFS);
public:
	std::map<std::string, std::vector<std::byte>> files;
	CDataStream *OpenFile( const std::string &path ) override
	{
		const auto found = files.find(path);
		if ( found == files.end() ) return nullptr;
		auto *stream = new CMemoryStream;
		stream->Write(found->second.data(), found->second.size());
		stream->Seek(0);
		return stream;
	}
	bool DoesFileExist( const std::string &path ) override { return files.count(path) != 0; }
	bool GetFileStats( NVFS::SFileStats *, const std::string & ) override { return false; }
	void GetAllFileNames( std::vector<std::string> *, const std::string & ) override {}
};

DEFINE_DG_CONSTANT_NODE( CTestTime, STime );

class CTestAnimation : public NDb::SAnimBase
{
	OBJECT_NOCOPY_METHODS(CTestAnimation);
public:
	NFile::CFilePath path;
	int GetTypeID() const override { return 0; }
	const NFile::CFilePath &GetModelFileRef() const override { return path; }
};

struct SModel
{
	fastgltf::Asset asset;
	std::vector<std::byte> binary;
	const fvec3 positions[3] = {{1,2,3}, {4,2,3}, {1,6,5}};
	fastgltf::TRS first, last;
	fmat4x4 restWorld;

	template<class T> size_t Accessor( const std::vector<T> &values, fastgltf::AccessorType type )
	{
		fastgltf::BufferView view;
		view.bufferIndex = 0;
		view.byteOffset = binary.size();
		view.byteLength = values.size() * sizeof(T);
		const auto *bytes = reinterpret_cast<const std::byte *>(values.data());
		binary.insert(binary.end(), bytes, bytes + view.byteLength);
		fastgltf::Accessor accessor;
		accessor.bufferViewIndex = asset.bufferViews.size();
		accessor.componentType = fastgltf::ComponentType::Float;
		accessor.type = type;
		accessor.count = values.size();
		asset.bufferViews.push_back(std::move(view));
		asset.accessors.push_back(std::move(accessor));
		return asset.accessors.size() - 1;
	}

	SModel( bool skinned, fastgltf::AnimationInterpolation interpolation )
	{
		first.translation = {3,4,5};
		first.rotation = fquat(0.2f, 0.3f, 0.4f, std::sqrt(0.71f));
		last.translation = {-2,7,9};
		last.rotation = fquat(-0.3f, 0.1f, 0.2f, std::sqrt(0.86f));
		last.scale = {1.5f,0.75f,2};
		asset.nodes.resize(2);
		asset.nodes[0].name = "root";
		asset.nodes[0].transform = first;
		asset.nodes[0].children.push_back(1);
		asset.nodes[1].name = "mesh";
		fastgltf::TRS child;
		child.translation = {1,2,3};
		child.rotation = fquat(0.f, std::sqrt(0.5f), 0.f, std::sqrt(0.5f));
		asset.nodes[1].transform = child;
		asset.nodes[1].meshIndex = 0;
		const auto rootWorld = fastgltf::getTransformMatrix(asset.nodes[0]);
		restWorld = fastgltf::getTransformMatrix(asset.nodes[1], rootWorld);
		asset.scenes.emplace_back().nodeIndices.push_back(0);
		asset.defaultScene = 0;
		fastgltf::Primitive primitive;
		primitive.attributes.emplace_back(fastgltf::Attribute{"POSITION", Accessor(std::vector<fvec3>(positions, positions + 3), fastgltf::AccessorType::Vec3)});
		asset.meshes.emplace_back().primitives.push_back(std::move(primitive));
		if ( skinned )
		{
			asset.nodes[1].skinIndex = 0;
			auto &skin = asset.skins.emplace_back();
			skin.joints = {0,1};
			skin.inverseBindMatrices = Accessor(std::vector<fmat4x4>{affineInverse(rootWorld), affineInverse(restWorld)}, fastgltf::AccessorType::Mat4);
		}
		const size_t times = Accessor(std::vector<float>{0,1}, fastgltf::AccessorType::Scalar);
		auto &animation = asset.animations.emplace_back();
		auto channel = [&](auto start, auto end, fastgltf::AccessorType type, fastgltf::AnimationPath path)
		{
			using T = decltype(start);
			// Zero cubic tangents give an independently predictable smoothstep curve.
			std::vector<T> values = interpolation == fastgltf::AnimationInterpolation::CubicSpline
				? std::vector<T>{T(0.f), start, T(0.f), T(0.f), end, T(0.f)} : std::vector<T>{start, end};
			animation.channels.push_back({animation.samplers.size(), 0, path});
			animation.samplers.push_back({times, Accessor(values, type), interpolation});
		};
		channel(first.translation, last.translation, fastgltf::AccessorType::Vec3, fastgltf::AnimationPath::Translation);
		channel(fvec4(first.rotation.x(), first.rotation.y(), first.rotation.z(), first.rotation.w()),
			fvec4(last.rotation.x(), last.rotation.y(), last.rotation.z(), last.rotation.w()),
			fastgltf::AccessorType::Vec4, fastgltf::AnimationPath::Rotation);
		channel(first.scale, last.scale, fastgltf::AccessorType::Vec3, fastgltf::AnimationPath::Scale);
		fastgltf::Buffer buffer;
		buffer.byteLength = binary.size();
		buffer.data = fastgltf::sources::Vector{binary, fastgltf::MimeType::None};
		asset.buffers.push_back(std::move(buffer));
	}

	fmat4x4 SampleWorld( float t, fastgltf::AnimationInterpolation interpolation ) const
	{
		if ( interpolation == fastgltf::AnimationInterpolation::Step ) t = t < 1 ? 0 : 1;
		if ( interpolation == fastgltf::AnimationInterpolation::CubicSpline ) t = t * t * (3 - 2 * t);
		fastgltf::TRS sample;
		sample.translation = first.translation * (1 - t) + last.translation * t;
		sample.scale = first.scale * (1 - t) + last.scale * t;
		if ( interpolation == fastgltf::AnimationInterpolation::CubicSpline )
		{
			const auto q = first.rotation * (1 - t) + last.rotation * t;
			// Normalize explicitly: this fastgltf version's quaternion / operator multiplies.
			sample.rotation = q * (1.f / std::sqrt(dot(q, q)));
		}
		else
			sample.rotation = slerp(first.rotation, last.rotation, t);
		fastgltf::Node root;
		root.transform = sample;
		return fastgltf::getTransformMatrix(asset.nodes[1], fastgltf::getTransformMatrix(root));
	}
};

void ExpectPoint( const CVec3 &actual, const fvec4 &source )
{
	// Assert final engine-space points, independently of NGltf's conversion helpers.
	EXPECT_NEAR(actual.x, -source.x(), 0.0001f);
	EXPECT_NEAR(actual.y, source.z(), 0.0001f);
	EXPECT_NEAR(actual.z, source.y(), 0.0001f);
}

class GltfMirror : public testing::TestWithParam<std::tuple<bool, bool, fastgltf::AnimationInterpolation>>
{
	CObj<NVFS::IVFS> savedVFS = NVFS::GetMainVFS();
protected:
	CObj<CModelVFS> vfs = new CModelVFS;
	void SetUp() override { NVFS::SetMainVFS(vfs); }
	void TearDown() override { NVFS::SetMainVFS(savedVFS); }
};
}

TEST_P( GltfMirror, ReflectsBoundsBindPoseAndAnimatedVertices )
{
	const auto [skinned, glb, interpolation] = GetParam();
	SModel model(skinned, interpolation);
	// Unique paths keep the production file cache from reusing another fixture.
	const std::string path = "mirror_" + std::to_string(skinned) + "_" + std::to_string(int(interpolation)) + (glb ? ".glb" : ".gltf");
	fastgltf::Exporter exporter;
	if ( glb )
	{
		auto result = exporter.writeGltfBinary(model.asset);
		ASSERT_TRUE(result);
		vfs->files[path] = std::move(result.get().output);
	}
	else
	{
		model.asset.buffers[0].data = fastgltf::sources::URI{0, fastgltf::URI(std::string("mirror.bin")), fastgltf::MimeType::None};
		auto result = exporter.writeGltfJson(model.asset);
		ASSERT_TRUE(result);
		const auto &json = result.get().output;
		const auto *bytes = reinterpret_cast<const std::byte *>(json.data());
		vfs->files[path] = std::vector<std::byte>(bytes, bytes + json.size());
		vfs->files["mirror.bin"] = model.binary;
	}
	const auto file = NGltf::LoadFile(nullptr, path);
	ASSERT_TRUE(file);
	for ( bool applySkinTransform : {false, true} )
	{
		CVec3 minimum, maximum;
		ASSERT_TRUE(NGltf::GetMeshBoundingBox(file, "mesh", applySkinTransform, &minimum, &maximum));
		fvec3 lo(std::numeric_limits<float>::max()), hi(std::numeric_limits<float>::lowest());
		for ( const auto &vertex : model.positions )
		{
			const auto p = (!skinned || applySkinTransform ? model.restWorld : fmat4x4()) * fvec4(vertex.x(), vertex.y(), vertex.z(), 1);
			for ( int axis = 0; axis < 3; ++axis ) { lo[axis] = (std::min)(lo[axis], p[axis]); hi[axis] = (std::max)(hi[axis], p[axis]); }
		}
		ExpectPoint(minimum, fvec4(hi.x(), lo.y(), lo.z(), 1));
		ExpectPoint(maximum, fvec4(lo.x(), hi.y(), hi.z(), 1));
	}
	CObj<NDb::SSkeleton> skeleton = dynamic_cast<NDb::SSkeleton *>(NObjectFactory::MakeObject(NDb::SSkeleton::typeID));
	ASSERT_TRUE(skeleton);
	skeleton->szModelFileRef = path;
	skeleton->szRootJoint = "root";
	CObj<CTestTime> time = new CTestTime(0);
	CDGPtr<NAnimation::ISkeletonAnimator> animator = NAnimation::CreateSkeletonAnimator(NAnimation::SSkeletonHandle(skeleton, 0), time);
	ASSERT_TRUE(animator);
	MarkNewDGFrame();
	animator.Refresh();
	const auto &rest = animator->GetValue();
	ASSERT_EQ(rest.worldPose.size(), 2u);
	for ( const auto &vertex : model.positions )
	{
		const auto p = (skinned ? fmat4x4() : model.restWorld) * fvec4(vertex.x(), vertex.y(), vertex.z(), 1);
		CVec3 actual;
		rest.compositePose[1].RotateHVector(&actual, CVec3(-p.x(), p.z(), p.y()));
		ExpectPoint(actual, p);
	}
	CObj<CTestAnimation> animation = new CTestAnimation;
	animation->path = path;
	ASSERT_GE(animator->AddAnimation(0, NAnimation::SAnimHandle(animation, 0), false), 0);
	for ( STime ms : {0u, 250u, 500u, 750u, 1000u} )
	{
		SCOPED_TRACE(ms);
		time->Set(ms);
		MarkNewDGFrame();
		animator.Refresh();
		const auto &pose = animator->GetValue();
		const auto world = model.SampleWorld(ms * 0.001f, interpolation);
		for ( const auto &vertex : model.positions )
		{
			CVec3 actual;
			pose.worldPose[1].RotateHVector(&actual, CVec3(-vertex.x(), vertex.z(), vertex.y()));
			const fvec4 source(vertex.x(), vertex.y(), vertex.z(), 1);
			ExpectPoint(actual, world * source);
			const auto bindVertex = (skinned ? fmat4x4() : model.restWorld) * source;
			pose.compositePose[1].RotateHVector(&actual, CVec3(-bindVertex.x(), bindVertex.z(), bindVertex.y()));
			ExpectPoint(actual, world * (skinned ? affineInverse(model.restWorld) : fmat4x4()) * source);
		}
	}
}

INSTANTIATE_TEST_SUITE_P( FormatsAndInterpolation, GltfMirror,
	testing::Combine(testing::Bool(), testing::Bool(), testing::Values(
		fastgltf::AnimationInterpolation::Linear, fastgltf::AnimationInterpolation::Step,
		fastgltf::AnimationInterpolation::CubicSpline)) );
