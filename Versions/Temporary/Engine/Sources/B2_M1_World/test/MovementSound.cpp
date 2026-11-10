#include "../stdafx.h"

#include "../MapObj.h"
#include "Sound/SoundScene.h"

#include <vector>

#include <gtest/gtest.h>

namespace
{
class CRecordingSoundScene : public ISoundScene
{
	OBJECT_BASIC_METHODS( CRecordingSoundScene );

public:
	struct SSound
	{
		CVec3 position;
		float volume;
		ESoundAddMode mode;
		bool finished = false;
		bool removed = false;
	};

	std::vector<SSound> sounds;

	void Init() override {}

	void Init( int, int, int, int, int ) override {}

	void SetSoundSceneMode( ESoundSceneMode ) override {}

	uint16_t AddSound( const NDb::SComplexSoundDesc *,
	                   const CVec3 &position,
	                   ESoundMixType,
	                   ESoundAddMode mode,
	                   unsigned int,
	                   int,
	                   float volume ) override
	{
		sounds.push_back( { position, volume, mode } );
		return mode == SAM_ADD_N_FORGET ? 0 : uint16_t( sounds.size() );
	}

	void RemoveSound( uint16_t id ) override
	{
		sounds.at( id - 1 ).removed = true;
	}

	void SetSoundPos( uint16_t id, const CVec3 &position ) override
	{
		sounds.at( id - 1 ).position = position;
	}

	void SetSoundVolume( uint16_t id, float volume ) override
	{
		sounds.at( id - 1 ).volume = volume;
	}

	bool IsSoundFinished( uint16_t id ) const override
	{
		return sounds.at( id - 1 ).finished;
	}

	uint16_t AddSoundToMap( const NDb::SComplexSoundDesc *, const CVec3 & ) override
	{
		return 0;
	}

	void RemoveSoundFromMap( uint16_t ) override {}

	void UpdateSound( const CVec3 &, const CVec3 &, float ) override {}

	void ClearSounds() override {}
};

class CTestMapObject : public CMapObj
{
	OBJECT_NOCOPY_METHODS( CTestMapObject );

public:
	bool CreateSceneObject( int, const SAINewUnitUpdate *, NDb::ESeason, bool ) override
	{
		return false;
	}

	IClientUpdatableProcess *AIUpdateRPGStats( const SAINotifyRPGStats &, IClientAckManager *, NDb::ESeason ) override
	{
		return nullptr;
	}

	void GetActions( CUserActions *, EActionsType ) const override {}

	void GetDisabledActions( CUserActions *, EActionsType ) const override {}

	void AttachLoopsAndCombatEffects()
	{
		AttachSound( EAST_MOVEMENT, nullptr, true );
		AttachSound( EAST_IDLE, nullptr, true );
		AttachSound( EAST_FALLING, nullptr, false );
		AttachSound( EAST_PLANE_DIVE, nullptr, false );
	}

	void StartOrStopEngine()
	{
		PlayMovementSound( nullptr, SoundScene() );
	}
};

class MovementSound : public testing::Test
{
protected:
	CObj<CRecordingSoundScene> scene;
	CObj<CTestMapObject> unit;

	void SetUp() override
	{
		scene = new CRecordingSoundScene;
		NSingleton::RegisterSingleton( scene, ISoundScene::tidTypeID );
		unit = new CTestMapObject;
	}

	void TearDown() override
	{
		unit = nullptr;
		NSingleton::UnRegisterSingleton( ISoundScene::tidTypeID );
		scene = nullptr;
	}

	void Refresh( float volume, float x = 100 )
	{
		SAINotifyPlacement placement;
		placement.bNewFormat = true;
		placement.vPlacement = CVec3( x, 200, 10 );
		unit->UpdateMovementSound( placement, volume );
	}
};

TEST_F( MovementSound, RefreshesExistingLoopsAndTransitionsWithoutChangingCombatEffects )
{
	Refresh( 0.55f );
	unit->AttachLoopsAndCombatEffects();
	unit->StartOrStopEngine();
	ASSERT_EQ( scene->sounds.size(), 5 );
	EXPECT_FLOAT_EQ( scene->sounds[0].volume, 0.55f );
	EXPECT_FLOAT_EQ( scene->sounds[1].volume, 0.55f );
	EXPECT_FLOAT_EQ( scene->sounds[4].volume, 0.55f );
	const CVec3 visualPosition = unit->GetCenter();
	Refresh( 0.0f, 300 );
	for ( int i : { 0, 1, 4 } )
	{
		EXPECT_FLOAT_EQ( scene->sounds[i].volume, 0.0f );
		EXPECT_FLOAT_EQ( scene->sounds[i].position.x, 300.0f );
	}
	for ( int i : { 2, 3 } )
	{
		EXPECT_FLOAT_EQ( scene->sounds[i].volume, 1.0f );
	}
	EXPECT_EQ( unit->GetCenter(), visualPosition );
	unit->StartOrStopEngine();
	EXPECT_EQ( scene->sounds.size(), 5 );
	Refresh( 1.0f );
	EXPECT_FLOAT_EQ( scene->sounds[0].volume, 1.0f );
	EXPECT_FLOAT_EQ( scene->sounds[1].volume, 1.0f );
	EXPECT_EQ( scene->sounds.size(), 5 );
	scene->sounds[4].finished = true;
	Refresh( 0.5f );
	EXPECT_TRUE( scene->sounds[4].removed );
}

TEST_F( MovementSound, HiddenSpawnStartsSilentAndDefaultKeepsLegacySounds )
{
	unit->AttachLoopsAndCombatEffects();
	unit->StartOrStopEngine();
	EXPECT_FLOAT_EQ( scene->sounds[0].volume, 1.0f );
	EXPECT_EQ( scene->sounds[4].mode, SAM_ADD_N_FORGET );
	unit = new CTestMapObject;
	Refresh( 0.0f );
	unit->AttachLoopsAndCombatEffects();
	EXPECT_FLOAT_EQ( scene->sounds[5].volume, 0.0f );
	EXPECT_FLOAT_EQ( scene->sounds[6].volume, 0.0f );
	EXPECT_FLOAT_EQ( scene->sounds[7].volume, 1.0f );
	EXPECT_FLOAT_EQ( scene->sounds[8].volume, 1.0f );
}

TEST_F( MovementSound, SavedGainAndAudioPositionApplyToNewMovementSounds )
{
	Refresh( 0.3f, 700 );
	CMemoryStream stream;
	{
		CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
		saver->AddPolymorphicBase( 1, unit );
	}
	unit = new CTestMapObject;
	stream.Seek( 0 );
	{
		CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ );
		loader->AddPolymorphicBase( 1, unit );
	}
	unit->AttachLoopsAndCombatEffects();
	unit->StartOrStopEngine();
	EXPECT_FLOAT_EQ( scene->sounds[0].volume, 0.3f );
	EXPECT_FLOAT_EQ( scene->sounds[0].position.x, 700.0f );
	EXPECT_FLOAT_EQ( scene->sounds[4].volume, 0.3f );
}
} // namespace
