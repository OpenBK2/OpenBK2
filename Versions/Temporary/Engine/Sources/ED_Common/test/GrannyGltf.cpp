#include "../GrannyGltf.h"
#include <fastgltf/tools.hpp>
#include <gtest/gtest.h>
#include <cstring>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <memory>

namespace
{
struct SFixture
{
	struct SVertex
	{
		float position[3];
		uint8_t weights[4];
		uint8_t joints[4];
		float normal[3];
		float uv[2];
	};
	granny_data_type_definition type[6] = {};
	SVertex vertices[3] = {
		{{0,0,2}, {255,0,0,0}, {0,255,255,255}, {0,0,1}, {0,0}},
		{{1,0,2}, {128,127,0,0}, {0,1,255,255}, {0,0,1}, {1,0}},
		{{0,1,2}, {255,0,0,0}, {1,255,255,255}, {0,0,1}, {0,1}}};
	granny_vertex_data vertexData = {};
	int indices[3] = {0,1,2};
	granny_tri_material_group group = {};
	granny_tri_topology topology = {};
	granny_bone_binding bindings[2] = {};
	granny_mesh mesh = {};
	granny_model_mesh_binding meshBinding = {};
	granny_bone bones[2] = {};
	granny_skeleton skeleton = {};
	granny_model model = {};
	NGrannyGltf::SModel source;

	SFixture()
	{
		type[0].Type = GrannyReal32Member; type[0].Name = "Position"; type[0].ArrayWidth = 3;
		type[1].Type = GrannyNormalUInt8Member; type[1].Name = "BoneWeights"; type[1].ArrayWidth = 4;
		type[2].Type = GrannyUInt8Member; type[2].Name = "BoneIndices"; type[2].ArrayWidth = 4;
		type[3].Type = GrannyReal32Member; type[3].Name = "Normal"; type[3].ArrayWidth = 3;
		type[4].Type = GrannyReal32Member; type[4].Name = "TextureCoordinates0"; type[4].ArrayWidth = 2;
		vertexData.VertexType = type; vertexData.VertexCount = 3;
		vertexData.Vertices = reinterpret_cast<uint8_t *>(vertices);
		group.TriCount = 1;
		topology.GroupCount = 1; topology.Groups = &group;
		topology.IndexCount = 3; topology.Indices = indices;
		bindings[0].BoneName = "child"; bindings[1].BoneName = "root";
		mesh.Name = "Mesh"; mesh.PrimaryVertexData = &vertexData; mesh.PrimaryTopology = &topology;
		mesh.BoneBindingCount = 2; mesh.BoneBindings = bindings;
		meshBinding.Mesh = &mesh;
		bones[0].Name = "root"; bones[0].ParentIndex = -1;
		bones[1].Name = "child"; bones[1].ParentIndex = 0;
		for ( auto &bone : bones )
		{
			GrannyMakeIdentity(&bone.LocalTransform);
			for ( int i = 0; i < 4; ++i ) bone.InverseWorld4x4[i][i] = 1;
		}
		bones[1].LocalTransform.Flags = GrannyHasPosition;
		bones[1].LocalTransform.Position[2] = 2;
		bones[1].InverseWorld4x4[3][2] = -2;
		skeleton.Name = "Skeleton"; skeleton.BoneCount = 2; skeleton.Bones = bones;
		model.Name = "Model"; model.Skeleton = &skeleton;
		model.MeshBindingCount = 1; model.MeshBindings = &meshBinding;
		source.name = "TestModel"; source.geometry = &model;
	}
};

fastgltf::Asset RoundTrip( NGrannyGltf::SDocument *doc )
{
	const auto bytes = doc->Finish();
	auto data = fastgltf::GltfDataBuffer::FromBytes(bytes.data(), bytes.size());
	if ( !data ) throw std::runtime_error("Invalid GLB bytes");
	fastgltf::Parser parser(fastgltf::Extensions::MSFT_texture_dds);
	auto asset = parser.loadGltfBinary(data.get(), {}, fastgltf::Options::LoadGLBBuffers);
	if ( !asset ) throw std::runtime_error("Cannot parse exported GLB");
	EXPECT_EQ(fastgltf::validate(asset.get()), fastgltf::Error::None);
	return std::move(asset.get());
}

void CheckSkinPose( const granny_skeleton *skeleton, const fastgltf::Asset &asset )
{
	// Compare against Granny's evaluated world and composite matrices, so a
	// transpose/handedness error cannot pass merely by producing valid glTF.
	std::unique_ptr<granny_local_pose, decltype(&GrannyFreeLocalPose)> local(GrannyNewLocalPose(skeleton->BoneCount), GrannyFreeLocalPose);
	std::unique_ptr<granny_world_pose, decltype(&GrannyFreeWorldPose)> world(GrannyNewWorldPose(skeleton->BoneCount), GrannyFreeWorldPose);
	ASSERT_TRUE(local); ASSERT_TRUE(world);
	for ( int i = 0; i < skeleton->BoneCount; ++i ) *GrannyGetLocalPoseTransform(local.get(), i) = skeleton->Bones[i].LocalTransform;
	GrannyBuildWorldPose(skeleton, 0, skeleton->BoneCount, local.get(), nullptr, world.get());
	std::vector<fastgltf::math::fmat4x4> transforms(asset.nodes.size());
	fastgltf::iterateSceneNodes(asset, 0, fastgltf::math::fmat4x4(), [&](const fastgltf::Node &node, const auto &matrix)
	{
		transforms[&node - asset.nodes.data()] = matrix;
	});
	const auto &skin = asset.skins[0];
	const auto binds = fastgltf::iterateAccessor<fastgltf::math::fmat4x4>(asset, asset.accessors[*skin.inverseBindMatrices]);
	auto bind = binds.begin();
	const int axes[] = {0,2,1,3};
	for ( int bone = 0; bone < skeleton->BoneCount; ++bone, ++bind )
	{
		SCOPED_TRACE(bone);
		const auto &matrix = transforms[skin.joints[bone]];
		const auto composite = matrix * *bind;
		for ( int col = 0; col < 4; ++col )
			for ( int row = 0; row < 4; ++row )
			{
				EXPECT_NEAR(matrix[col][row], GrannyGetWorldPose4x4(world.get(), bone)[axes[col] * 4 + axes[row]], 1e-3f);
				EXPECT_NEAR(composite[col][row], GrannyGetWorldPoseComposite4x4(world.get(), bone)[axes[col] * 4 + axes[row]], 1e-3f);
			}
	}
}

std::vector<fastgltf::math::fvec4> SkinnedVertices( fastgltf::Asset &asset, size_t frame )
{
	// Evaluate the exported glTF channels and skin matrices independently of
	// the converter, as an importer would, including the separate AI scene.
	for ( const auto &channel : asset.animations[0].channels )
	{
		const auto &sampler = asset.animations[0].samplers[channel.samplerIndex];
		const auto &accessor = asset.accessors[sampler.outputAccessor];
		auto &trs = std::get<fastgltf::TRS>(asset.nodes[*channel.nodeIndex].transform);
		if ( channel.path == fastgltf::AnimationPath::Translation ) trs.translation = fastgltf::getAccessorElement<fastgltf::math::fvec3>(asset, accessor, frame);
		else if ( channel.path == fastgltf::AnimationPath::Scale ) trs.scale = fastgltf::getAccessorElement<fastgltf::math::fvec3>(asset, accessor, frame);
		else if ( channel.path == fastgltf::AnimationPath::Rotation )
		{
			const auto q = fastgltf::getAccessorElement<fastgltf::math::fvec4>(asset, accessor, frame);
			trs.rotation = fastgltf::math::fquat(q[0], q[1], q[2], q[3]);
		}
	}
	std::vector<fastgltf::math::fmat4x4> world(asset.nodes.size());
	for ( size_t scene = 0; scene < asset.scenes.size(); ++scene )
		fastgltf::iterateSceneNodes(asset, scene, fastgltf::math::fmat4x4(), [&](const fastgltf::Node &node, const auto &matrix)
		{
			world[&node - asset.nodes.data()] = matrix;
		});
	std::vector<fastgltf::math::fvec4> result;
	for ( const auto &node : asset.nodes )
	{
		if ( !node.skinIndex || !node.meshIndex ) continue;
		const auto &skin = asset.skins[*node.skinIndex];
		for ( const auto &primitive : asset.meshes[*node.meshIndex].primitives )
		{
			const auto &positions = asset.accessors[primitive.findAttribute("POSITION")->accessorIndex];
			for ( size_t vertex = 0; vertex < positions.count; ++vertex )
			{
				const auto p = fastgltf::getAccessorElement<fastgltf::math::fvec3>(asset, positions, vertex);
				const auto joints = fastgltf::getAccessorElement<fastgltf::math::uvec4>(asset, asset.accessors[primitive.findAttribute("JOINTS_0")->accessorIndex], vertex);
				const auto weights = fastgltf::getAccessorElement<fastgltf::math::fvec4>(asset, asset.accessors[primitive.findAttribute("WEIGHTS_0")->accessorIndex], vertex);
				fastgltf::math::fvec4 point(0.f);
				for ( int k = 0; k < 4; ++k )
				{
					const auto inverse = fastgltf::getAccessorElement<fastgltf::math::fmat4x4>(asset, asset.accessors[*skin.inverseBindMatrices], joints[k]);
					point += (world[skin.joints[joints[k]]] * inverse * fastgltf::math::fvec4(p[0], p[1], p[2], 1)) * weights[k];
				}
				result.push_back(point);
			}
		}
	}
	return result;
}
}

TEST( GrannyGltf, PreservesBindPoseAndRemapsSkinByBoneName )
{
	SFixture f;
	NGrannyGltf::SDocument doc;
	NGrannyGltf::Convert(f.source, &doc);
	auto asset = RoundTrip(&doc);
	const auto &primitive = asset.meshes[0].primitives[0];
	const auto positions = fastgltf::iterateAccessor<fastgltf::math::fvec3>(asset, asset.accessors[primitive.findAttribute("POSITION")->accessorIndex]);
	EXPECT_EQ((*positions.begin()), fastgltf::math::fvec3(0,2,0));
	const auto joints = fastgltf::iterateAccessor<fastgltf::math::uvec4>(asset, asset.accessors[primitive.findAttribute("JOINTS_0")->accessorIndex]);
	EXPECT_EQ((*joints.begin())[0], 1u);
	const auto indices = fastgltf::iterateAccessor<uint32_t>(asset, asset.accessors[*primitive.indicesAccessor]);
	std::vector<uint32_t> order;
	for ( auto index : indices ) order.push_back(index);
	EXPECT_EQ(order, (std::vector<uint32_t>{0,2,1}));
	const auto binds = fastgltf::iterateAccessor<fastgltf::math::fmat4x4>(asset, asset.accessors[*asset.skins[0].inverseBindMatrices]);
	auto it = binds.begin(); ++it;
	EXPECT_FLOAT_EQ((*it)[3][1], -2);
	EXPECT_EQ(std::get<fastgltf::TRS>(asset.nodes[1].transform).translation, fastgltf::math::fvec3(0,2,0));
}

TEST( GrannyGltf, ConcatenatesClipsAndResetsUntrackedBones )
{
	SFixture f;
	float knots[2] = {0,1};
	float positions[6] = {0,0,2, 0,0,5};
	granny_curve_data_da_k32f_c32f curve = {};
	curve.CurveDataHeader.Format = GrannyCurveDataDaK32fC32fFormat;
	curve.CurveDataHeader.Degree = 1;
	curve.KnotCount = 2; curve.Knots = knots; curve.ControlCount = 6; curve.Controls = positions;
	granny_transform_track track = {};
	track.Name = "child"; track.PositionCurve.CurveData.Object = &curve;
	granny_track_group moving = {}, rest = {};
	moving.TransformTrackCount = 1; moving.TransformTracks = &track;
	granny_track_group *movingPtr = &moving, *restPtr = &rest;
	granny_animation a = {}, b = {};
	a.Duration = b.Duration = 1; a.TrackGroupCount = b.TrackGroupCount = 1;
	a.TrackGroups = &movingPtr; b.TrackGroups = &restPtr;
	f.source.clips = {{&a,"move"}, {&b,"rest"}};
	NGrannyGltf::SDocument doc;
	NGrannyGltf::Convert(f.source, &doc);
	auto asset = RoundTrip(&doc);
	ASSERT_EQ(asset.animations.size(), 1u);
	const auto &animation = asset.animations[0];
	ASSERT_EQ(animation.channels.size(), 6u);
	const auto &sampler = animation.samplers[animation.channels[3].samplerIndex];
	std::vector<fastgltf::math::fvec3> values;
	for ( auto value : fastgltf::iterateAccessor<fastgltf::math::fvec3>(asset, asset.accessors[sampler.outputAccessor]) ) values.push_back(value);
	ASSERT_EQ(values.size(), 62u);
	EXPECT_NEAR(values[15][1], 3.5f, 1e-5f);
	EXPECT_NEAR(values[30][1], 5.0f, 1e-5f);
	EXPECT_NEAR(values[31][1], 2.0f, 1e-5f);
	float previous = -1;
	for ( auto time : fastgltf::iterateAccessor<float>(asset, asset.accessors[sampler.inputAccessor]) )
	{
		EXPECT_GT(time, previous); previous = time;
	}
	EXPECT_NEAR(previous, 61.0f / 30, 1e-5f);
	EXPECT_NE(doc.extras.at({fastgltf::Category::Animations, 0}).find("rest"), std::string::npos);
}

TEST( GrannyGltf, MirrorReflectsAnimatedSkinnedVerticesAndCollisionGeometry )
{
	SFixture f;
	f.bones[1].LocalTransform.Position[0] = 1;
	f.bones[1].InverseWorld4x4[3][0] = -1;
	for ( auto &vertex : f.vertices ) { vertex.normal[0] = 1; vertex.normal[2] = 0; }
	float knots[2] = {0,1};
	float positions[6] = {0,0,0, 3,4,5};
	float rotations[8] = {0,0,0,1, 0.2f,0.3f,0.4f,std::sqrt(0.71f)};
	float scales[18] = {1,0,0, 0,1,0, 0,0,1, 1.5f,0,0, 0,0.75f,0, 0,0,2};
	granny_curve_data_da_k32f_c32f curves[3] = {};
	for ( auto &curve : curves )
	{
		curve.CurveDataHeader.Format = GrannyCurveDataDaK32fC32fFormat;
		curve.CurveDataHeader.Degree = 1;
		curve.KnotCount = 2; curve.Knots = knots;
	}
	curves[0].ControlCount = 6; curves[0].Controls = positions;
	curves[1].ControlCount = 8; curves[1].Controls = rotations;
	curves[2].ControlCount = 18; curves[2].Controls = scales;
	granny_transform_track track = {};
	track.Name = "root";
	track.PositionCurve.CurveData.Object = &curves[0];
	track.OrientationCurve.CurveData.Object = &curves[1];
	track.ScaleShearCurve.CurveData.Object = &curves[2];
	granny_track_group moving = {}, rest = {};
	moving.TransformTrackCount = 1; moving.TransformTracks = &track;
	granny_track_group *movingPtr = &moving, *restPtr = &rest;
	granny_animation a = {}, b = {};
	a.Duration = b.Duration = 1; a.TrackGroupCount = b.TrackGroupCount = 1;
	a.TrackGroups = &movingPtr; b.TrackGroups = &restPtr;
	f.source.clips = {{&a,"move rotate scale"}, {&b,"rest"}};
	f.source.aiMeshes.push_back({&f.mesh, &f.skeleton});
	NGrannyGltf::SDocument originalDoc, mirroredDoc;
	NGrannyGltf::Convert(f.source, &originalDoc);
	f.source.mirrorX = true;
	NGrannyGltf::Convert(f.source, &mirroredDoc);
	auto original = RoundTrip(&originalDoc), mirrored = RoundTrip(&mirroredDoc);
	for ( size_t frame : {0u, 15u, 30u, 31u, 61u} )
	{
		SCOPED_TRACE(frame);
		const auto a = SkinnedVertices(original, frame), b = SkinnedVertices(mirrored, frame);
		ASSERT_EQ(a.size(), 6u); ASSERT_EQ(b.size(), a.size());
		for ( size_t i = 0; i < a.size(); ++i )
			for ( int axis = 0; axis < 4; ++axis ) EXPECT_NEAR(b[i][axis], a[i][axis] * (axis == 0 ? -1 : 1), 1e-4f);
	}
	for ( const auto &mesh : mirrored.meshes )
	{
		const auto &primitive = mesh.primitives[0];
		std::vector<uint32_t> indices;
		for ( auto index : fastgltf::iterateAccessor<uint32_t>(mirrored, mirrored.accessors[*primitive.indicesAccessor]) ) indices.push_back(index);
		EXPECT_EQ(indices, (std::vector<uint32_t>{0,1,2}));
		const auto normal = fastgltf::getAccessorElement<fastgltf::math::fvec3>(mirrored, mirrored.accessors[primitive.findAttribute("NORMAL")->accessorIndex], 0);
		EXPECT_EQ(normal, fastgltf::math::fvec3(-1,0,0));
	}
}

TEST( GrannyGltf, RejectsInvalidIndicesAndBoneHierarchies )
{
	SFixture f;
	NGrannyGltf::SDocument doc;
	f.indices[2] = 3;
	EXPECT_THROW(NGrannyGltf::Convert(f.source, &doc), std::runtime_error);
	f.indices[2] = 2;
	f.bones[0].ParentIndex = 1;
	NGrannyGltf::SDocument cyclic;
	EXPECT_THROW(NGrannyGltf::Convert(f.source, &cyclic), std::runtime_error);
}

TEST( GrannyGltf, CollisionGeometryIsOutsideTheDefaultScene )
{
	SFixture f;
	f.source.aiMeshes.push_back({&f.mesh, &f.skeleton});
	NGrannyGltf::SDocument doc;
	NGrannyGltf::Convert(f.source, &doc);
	auto asset = RoundTrip(&doc);
	ASSERT_EQ(asset.scenes.size(), 2u);
	EXPECT_EQ(*asset.defaultScene, 0u);
	const size_t visible = asset.scenes[0].nodeIndices.back();
	const size_t hidden = asset.scenes[1].nodeIndices.front();
	EXPECT_EQ(asset.nodes[visible].name, "Geometry");
	EXPECT_EQ(asset.nodes[hidden].name, "AIGeometry");
	EXPECT_NE(visible, hidden);
	ASSERT_EQ(asset.nodes[hidden].children.size(), 1u);
	EXPECT_TRUE(asset.nodes[asset.nodes[hidden].children[0]].meshIndex.has_value());
}

TEST( GrannyGltf, ConvertsRepositoryMineTankAndInfantry )
{
	// Real exported resources catch format/layout cases that synthetic meshes do
	// not. Skip on source-only checkouts without the optional game-data corpus.
	using TFile = std::unique_ptr<granny_file, decltype(&GrannyFreeFile)>;
	std::vector<TFile> files;
	auto load = [&](const char *folder, const char *uid) -> granny_file_info *
	{
		const auto path = std::filesystem::path(OBK2_DATA_DIR) / "bin" / folder / uid;
		std::ifstream stream(path, std::ios::binary | std::ios::ate);
		if ( !stream ) return nullptr;
		std::vector<char> bytes(static_cast<size_t>(stream.tellg()));
		stream.seekg(0); stream.read(bytes.data(), bytes.size());
		TFile file(GrannyReadEntireFileFromMemory(static_cast<int>(bytes.size()), bytes.data()), GrannyFreeFile);
		auto *info = file ? GrannyGetFileInfo(file.get()) : nullptr;
		files.push_back(std::move(file));
		return info;
	};
	const char *samples[][3] = {
		{"E1035CC6-4B39-4795-844A-4C5DFA512984", "4C329AF5-8401-44EC-92CC-6B02728EEB93", "2CCBC0E7-75C7-4FE8-A225-D29AB73D6468"},
		{"C2CE8035-833A-4248-9AD5-EADAEADBE8F5", "3A6C0E99-30FC-427E-A663-4F1D9A14C918", "9698E788-8238-42C4-897A-AC405284F7D2"},
		{"E0645027-7D83-470F-82E1-73A00E3B8812", "203D3B34-3C78-42A7-B430-528C47B3F53C", "C5559132-D506-46AA-ADE6-0D2E8A3FCCE0"}};
	for ( int sample = 0; sample < 3; ++sample )
	{
		SCOPED_TRACE(sample);
		auto *geometry = load("Geometries", samples[sample][0]);
		auto *skeleton = load("Skeletons", samples[sample][1]);
		auto *ai = load("AIGeometries", samples[sample][2]);
		if ( !geometry || !skeleton || !ai ) GTEST_SKIP() << "Optional repository GR2 samples are unavailable";
		NGrannyGltf::SModel source;
		source.name = "Repository sample";
		ASSERT_GT(geometry->ModelCount, 0); ASSERT_GT(skeleton->ModelCount, 0);
		source.geometry = geometry->Models[0]; source.skeleton = skeleton->Models[0]->Skeleton;
		for ( int i = 0; i < ai->MeshCount; ++i )
		{
			const granny_skeleton *rig = nullptr;
			for ( int m = 0; m < ai->ModelCount && !rig; ++m )
				for ( int b = 0; b < ai->Models[m]->MeshBindingCount; ++b )
					if ( ai->Models[m]->MeshBindings[b].Mesh == ai->Meshes[i] ) rig = ai->Models[m]->Skeleton;
			source.aiMeshes.push_back({ai->Meshes[i], rig});
		}
		if ( sample == 2 )
			for ( const char *uid : {"6758C21E-E017-458A-97A7-EF0901CD3ECF", "75C5382A-4C00-4A83-B11E-FC1A994D16EC", "F2C157EF-32F0-47DA-A621-554093706117"} )
			{
				auto *animation = load("Animations", uid);
				ASSERT_TRUE(animation); ASSERT_GT(animation->AnimationCount, 0);
				source.clips.push_back({animation->Animations[0], uid});
			}
		NGrannyGltf::SDocument doc;
		NGrannyGltf::Convert(source, &doc);
		auto asset = RoundTrip(&doc);
		CheckSkinPose(source.skeleton, asset);
		EXPECT_EQ(asset.meshes.size(), size_t(source.geometry->MeshBindingCount + ai->MeshCount));
		EXPECT_EQ(asset.animations.size(), sample == 2 ? 1u : 0u);
	}
}
