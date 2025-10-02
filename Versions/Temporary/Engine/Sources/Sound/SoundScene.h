#pragma once

#include "Sound_export.h"


enum ESoundSceneMode
{
	ESSM_INTERMISSION_INTERFACE,
	ESSM_INGAME,
	ESSM_CLEAR_SOUNDS,
};

// scene classes IDs
enum
{
	SFX_PLAY_LIST = 0x11079300,
};

enum ESoundMixType
{
	SFX_MIX_IF_TIME_EQUALS,
	SFX_MIX_SUBSTITUTE,
	SFX_MIX_ALWAYS,
	SFX_INTERFACE,

	SFX_MIX_ALL = 0x7fffffff,
};

enum ESoundAddMode
{
	SAM_LOOPED_NEED_ID,										// вернется ID звука, звук будет зацикленный
	SAM_NEED_ID,													// вернется ID звука
	SAM_ADD_N_FORGET,											// вернется 0, сцена сама удалит звук.
};
interface ISFX;
namespace NDb
{
	struct SComplexSoundDesc;
}

interface ISoundScene : public CObjectBase
{
	enum { tidTypeID = 0x1107C480 };
	//
	virtual void Init() = 0;
	// set terrain sizes
	virtual void Init( const int _nMaxX, const int _nMaxY, const int _nMinZ, const int _nMaxZ, const int _VIS_TILE_SIZE ) = 0;
	//virtual void InitMusic( const string &_szPartyName ) = 0;
	// SS mode
	virtual void SetSoundSceneMode( const enum ESoundSceneMode eSoundSceneMode ) = 0;
	//add - remove sound. return sound ID
	// nVolumeType =  0 - Music, 1-Voice, 2-SFX
	virtual WORD AddSound( const struct NDb::SComplexSoundDesc *pStats,
		const CVec3 &vPos, // AI pixels
		const ESoundMixType eMixType,
		const ESoundAddMode eAddMode,
		const unsigned int nTimeAfterStart,
		int nVolumeType ) = 0;

	virtual WORD AddSound( const struct NDb::SComplexSoundDesc *pStats,
		CFuncBase<CVec3> *pPos, // AI pixels
		const ESoundMixType eMixType,
		const ESoundAddMode eAddMode,
		const unsigned int nTimeAfterStart,
		int nVolumeType ) { return 0; };

	virtual void RemoveSound( const WORD wID ) = 0;
	// sound management (vPos in AI pixels)
	virtual void SetSoundPos( const WORD wID, const CVec3 &vPos ) = 0;
	virtual bool IsSoundFinished( const WORD wID ) const = 0;
	// map sounds (vPos in AI pixels)
	virtual WORD AddSoundToMap( const NDb::SComplexSoundDesc* pDesc, const CVec3 &vPos ) = 0;
	virtual void RemoveSoundFromMap( const WORD	wInstanceID ) = 0;
	// segment 
	virtual void UpdateSound( const CVec3 &vListener, const CVec3 &vCameraDir, const float fViewRadius ) = 0;
	// clear all map sounds. 
	virtual void ClearSounds() = 0;
};
inline ISoundScene *SoundScene() { return Singleton<ISoundScene>(); }
SOUND_EXPORT ISoundScene* CreateSoundScene();


