#include "../stdafx.h"

#include "../SFX.h"
#include "../SoundScene.h"
#include "Main/GameTimer.h"
#include "Main/GameTimerInternal.h"

#include <functional>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <gtest/gtest.h>

namespace
{
struct SFields
{
	std::function<void( IBinSaver & )> visit;

	int operator&( IBinSaver &saver )
	{
		visit( saver );
		return 0;
	}
};

class CTestSample : public ISound
{
	OBJECT_BASIC_METHODS( CTestSample );
	float volume = 1.0f;

public:
	int Visit( ISFXVisitor * ) override
	{
		return 0;
	}

	void SetVolume( float value ) override
	{
		volume = value;
	}

	float GetVolume() const override
	{
		return volume;
	}

	void SetVolumeType( int ) override {}

	int GetVolumeType() const override
	{
		return 2;
	}

	void SetPan( float ) override {}

	float GetPan() const override
	{
		return 0.0f;
	}

	CVec3 GetPos() const override
	{
		return VNULL3;
	}

	void SetPos( const CVec3 & ) override {}

	CVec3 GetSpeed() const override
	{
		return VNULL3;
	}

	void SetSpeed( const CVec3 & ) override {}

	void SetMinMax( float, float ) override {}

	unsigned int GetLenght() override
	{
		return 10000;
	}

	unsigned int GetSampleRate() override
	{
		return 1000;
	}
};

class CRecordingSFX : public ISFX
{
	OBJECT_BASIC_METHODS( CRecordingSFX );

public:
	std::map<ISound *, bool> playing;

	CObjectBase *QI( int ) override
	{
		return nullptr;
	}

	bool IsInitialized() override
	{
		return true;
	}

	bool Init( HWND, int, ESFXOutputType, int, int ) override
	{
		return true;
	}

	void EnableSFX( bool ) override {}

	bool IsSFXEnabled() const override
	{
		return true;
	}

	void SetDistanceFactor( float ) override {}

	void SetRolloffFactor( float ) override {}

	int PlaySample( ISound *sample, bool looped, unsigned int ) override
	{
		playing[sample] = looped;
		return 1;
	}

	void UpdateSample( ISound * ) override {}

	void StopSample( ISound *sample ) override
	{
		playing.erase( sample );
	}

	void StopChannel( int ) override {}

	void Update( const CVec3 &, const CVec3 &, NTimer::STime ) override {}

	void Set3DMode( bool ) override {}

	bool Pause( bool ) override
	{
		return false;
	}

	bool IsPaused() override
	{
		return false;
	}

	bool IsPlaying( ISound *sample ) override
	{
		return playing.find( sample ) != playing.end();
	}

	unsigned int GetCurrentPosition( ISound * ) override
	{
		return 0;
	}

	void SetCurrentPosition( ISound *, unsigned int ) override {}
};

TEST( SceneVolume, Tracked3DSamplesKeepTheirGainAndLoopModeAfterReload )
{
	// Anchor the production factories on Windows without initializing an audio device.
	CObj<ISoundScene> anchor = CreateSoundScene();
	ASSERT_EQ( CastToObjectBaseImpl<NGameTimer::CGameTimer>( nullptr, static_cast<void *>( nullptr ) ), nullptr );
	CObj<IGameTimer> timer = dynamic_cast<IGameTimer *>( NObjectFactory::MakeObject( IGameTimer::tidTypeID ) );
	ASSERT_NE( timer, nullptr );
	CObj<ISoundScene> scene = dynamic_cast<ISoundScene *>( NObjectFactory::MakeObject( 0x11190BC1 ) );
	ASSERT_NE( scene, nullptr );
	CObj<CRecordingSFX> device = new CRecordingSFX;
	CObj<CTestSample> engine = new CTestSample;
	CObj<CTestSample> transition = new CTestSample;
	std::vector<SBinSaverExternalObject> external{ { 1, timer }, { 2, device }, { 3, engine }, { 4, transition } };
	std::unordered_map<int, std::pair<CPtr<ISound>, NTimer::STime>> moving{ { 1, { engine.GetPtr(), 0 } },
	                                                                        { 2, { transition.GetPtr(), 0 } } };
	std::unordered_set<int> oneShots{ 2 };
	SFields fields{ [&]( IBinSaver &saver )
	                {
										saver.Add( 2, &device );
										saver.Add( 3, &timer );
										saver.Add( 5, &moving );
										saver.Add( 10, &oneShots );
									} };
	CMemoryStream initial;
	{
		CPtr<IBinSaver> saver = CreateBinSaver( &initial, SAVER_MODE_WRITE, external );
		saver->Add( 1, &fields );
	}
	initial.Seek( 0 );
	{
		CPtr<IBinSaver> loader = CreateBinSaver( &initial, SAVER_MODE_READ, external );
		loader->AddPolymorphicBase( 1, scene );
	}
	scene->SetSoundVolume( 1, 0.25f );
	EXPECT_FLOAT_EQ( engine->GetVolume(), 0.25f );
	EXPECT_FLOAT_EQ( transition->GetVolume(), 1.0f );
	scene->SetSoundVolume( 1, 0.0f );
	EXPECT_FLOAT_EQ( engine->GetVolume(), 0.0f );
	scene->SetSoundVolume( 1, 1.0f );
	EXPECT_FLOAT_EQ( engine->GetVolume(), 1.0f );
	scene->SetSoundVolume( 2, 0.4f );

	CMemoryStream saved;
	{
		CPtr<IBinSaver> saver = CreateBinSaver( &saved, SAVER_MODE_WRITE, external );
		saver->AddPolymorphicBase( 1, scene );
	}
	scene = dynamic_cast<ISoundScene *>( NObjectFactory::MakeObject( 0x11190BC1 ) );
	saved.Seek( 0 );
	{
		CPtr<IBinSaver> loader = CreateBinSaver( &saved, SAVER_MODE_READ, external );
		loader->AddPolymorphicBase( 1, scene );
	}
	scene->UpdateSound( VNULL3, V3_AXIS_Y, 100 );
	ASSERT_EQ( device->playing.size(), 2 );
	EXPECT_TRUE( device->playing[engine] );
	EXPECT_FALSE( device->playing[transition] );
	EXPECT_FLOAT_EQ( transition->GetVolume(), 0.4f );
	scene->RemoveSound( 2 );
	EXPECT_EQ( device->playing.size(), 1 );
	scene->ClearSounds();
	EXPECT_TRUE( device->playing.empty() );
}
} // namespace
