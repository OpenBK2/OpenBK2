#pragma once
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "../System/DB.h"
#include "CellsConglomerateContainer.h"
#include "MapSounds.h"
#include "TerrainSounds.h"
#include "SoundSceneUtills.h"
#include "SoundScene.h"
#include "DBSound.h"
#include "DBSoundDesc.h"
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CSoundCell;
class CSound;
namespace NDb
{
	struct SMapSoundInfo;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CSoundScene2D : public ISoundScene
{
	OBJECT_NOCOPY_METHODS( CSoundScene2D );
public:

	static const NTimer::STime &GetCurTime();		// чтобы не передавать всюду
	// для определения лежит ли звук в пределах карты
	static bool IsInBounds( const int x, const int y, const int z );
	typedef list< SUpdatedCell > CUpdatedCells; 
public:
	typedef list< CPtr<ISound> > CSamplesList;
	typedef list< CPtr<CSound> > CSoundsList;
	typedef hash_map<CDBPtr<NDb::SSoundDesc>/*subst name*/, CSoundsList, SDBPtrHash> CHearableSounds;
	typedef hash_map<CDBPtr<NDb::SSoundDesc>/*sound name*/, CDBPtr<NDb::SSoundDesc>/*subst name*/, SDBPtrHash> CSoundSubstTable;

	typedef vector< CArray2D< CPtr<CSoundCell> > > CSoundCellsInBounds;

	typedef hash_map<SIntThree, CPtr<CSoundCell>, SIntThreeHash> CSoundCellsOutOfBounds;
	typedef hash_map<SIntThree, CPtr<CSoundCell>, SIntThreeHash> CSoundCellsWithSound;

	// для сбора звуков, которые слышны в клетке и сортировки их по
	// звукам их заменяющим
	class CSoundsCollector
	{
		CSoundSubstTable	&substTable;
		CHearableSounds		&sounds;
		CSoundCellsWithSound &cellsWSound;
		CSoundsList				&muteSounds;
	public:
		CSoundsCollector(	CSoundSubstTable &substTable, CHearableSounds &sounds, CSoundCellsWithSound & cellsWSound, CSoundsList & muteSounds )
			:substTable( substTable ), sounds( sounds ), cellsWSound( cellsWSound ), muteSounds( muteSounds ) {  }
			void operator()( int nRadius, const SIntThree & vCell );
			void operator()( class CSound * sound, bool bHearable = true );

			void CalcMuteSounds();
	};

private:
	enum ESoundSceneMode eSoundSceneMode;
	bool bMapInitted;
	CFreeIds freeIDs;											// таблица ID звуков
	typedef hash_map< WORD, SIntThree > CSoundIDs;
	CSoundIDs soundIDs;			// в какой клетке находится звук.

	CPtr<ISFX> pSFX;
	CPtr<IGameTimer> pGameTimer;

	CUpdatedCells updatedCells;						// для промежуточного хранения

	NTimer::STime timeLastUpdate;
	SIntThree vFormerCameraCell;								// для отслеживания перемещения камеры
	static SIntThree vLimit;											// максимальные размеры карты
	static int nMinZ;															// minimal camera height

	CSoundSubstTable substTable;					// таблица замены звуков

	CHearableSounds interfaceSounds;					// звуки от интерфейса
	hash_set<int> finishedInterfaceSounds;
	hash_set<int> deletedInterfaceSounds;

	CTerrainSounds terrainSounds;
	CMapSounds mapSounds;

	CSoundCellsInBounds	soundCellsInBounds;								// звуки на карте
	CSoundCellsOutOfBounds soundCellsOutOfBounds;		// звуки за кратой (гл.образом самолеты)
	CSoundCellsWithSound soundCellsWithSound;				// список всех клеток со звуками

	CCellsConglomerateContainer cellsPHS;
	static NTimer::STime curTime;					// чтобы не передавать всюду
	CVec2 vFormerCameraDir;

	// helper functions
	CSoundCell * GetSoundCell( const SIntThree &vCell );

	void AddSound( const SIntThree &vCell, CSound *s );
	void To2DSoundPos( const CVec3 &vPos, CVec3 *pv2DPos );
	// ordinary update
	void UpdatePHSMap( const SIntThree &vCell, const int nFormerRadius, const int nNewRadius );

	void UpdateCombatMap( const SIntThree &vCell, CSound *pSound );
	void CalcVolNPan( float *fVolume, float *fPan, const CVec3 &vSound, const float fMaxHear );
	void MuteSounds( CSoundsList	* muteSounds );

	void MixInterfaceSounds();
	void MixMixedAlways( CSoundScene2D::CHearableSounds & sounds, const CVec3 & vCameraPos );
	void MixSingle( CSoundScene2D::CHearableSounds & sounds, const CVec3 & vCameraPos );
	void MixMixedWithDelta( CSoundScene2D::CHearableSounds & sounds, const CVec3 & vCameraPos );
	void Mix(	CSoundsList & curSounds,
		const CSoundsList::iterator begin_iter,
		const CSoundsList::iterator end_iter,
		const NDb::SSoundDesc *pSubsts,
		const CVec3 &vCameraPos,
		const enum ESoundMixType eMixType,
		const int nMixMinimum,
		bool bDelete = true );

	void PlaySubstSound( interface ISound *pSubstSound, unsigned int nStartSample, unsigned int nSoundLenght, bool bLooped );

	void InitConsts();
	~CSoundScene2D();
public:

	CSoundScene2D();
	int operator&( IBinSaver &saver );
	//
	virtual void Init() ;
	//

	// must be called when new terrain was loaded
	void Init( const int _nMaxX, const int _nMaxY, const int _nMinZ, const int _nMaxZ, const int _VIS_TILE_SIZE );
	void SetTerrain( interface ITerrainSounds * pTerrain );

	// если начался или идет бой - то вызывать эу функцию
	void MuteTerrain( const bool bMute );
	WORD AddSound( 	const NDb::SComplexSoundDesc *pStats,
		const CVec3 &vPos,
		const enum ESoundMixType eMixMode,
		const enum ESoundAddMode eAddMode,
		const unsigned int nTimeAfterStart,
		int nVolumeType );

	//удаляет звук из сцены. ID становится инвалидным
	void RemoveSound( const WORD wID );
	// задает новую позицию звуку.
	void SetSoundPos( const WORD wID, const class CVec3 &vPos );
	bool IsSoundFinished( const WORD wID ) const;
	void UpdateSound( const CVec3 &vListener, const CVec3 &vCameraDir, const float fViewRadius );

	void SetSoundSceneMode( const enum ESoundSceneMode eSoundSceneMode );
	enum ESoundSceneMode GetMode() const { return eSoundSceneMode; };

	WORD AddSoundToMap( const NDb::SComplexSoundDesc* pDesc, const CVec3 &vPos );
	void RemoveSoundFromMap( const WORD	wInstanceID );
	void ClearSounds();
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CSoundScene3D : public ISoundScene
{
	OBJECT_NOCOPY_METHODS( CSoundScene3D );


	typedef pair<CPtr<ISound>, NTimer::STime> CSoundWithStartTime;
	typedef hash_map<int, CSoundWithStartTime > CMovingSounds;
	typedef hash_map<int, CSoundWithStartTime > CInterfaceSounds;
	typedef list<CSoundWithStartTime > CForgottenSounds;

	CForgottenSounds::iterator currentUnderUpdate;

	ZDATA
	CPtr<ISFX> pSFX;
	CPtr<IGameTimer> pGameTimer;

	CFreeIds freeIDs;
	CMovingSounds movingSounds;
	CInterfaceSounds interfaceSounds;
	CForgottenSounds forgottenSounds;
	NTimer::STime timeLastPosUpdate;
	bool bLaunchAfterLoad;

	ZONSERIALIZE
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pSFX); f.Add(3,&pGameTimer); f.Add(4,&freeIDs); f.Add(5,&movingSounds); f.Add(6,&interfaceSounds); f.Add(7,&forgottenSounds); f.Add(8,&timeLastPosUpdate); f.Add(9,&bLaunchAfterLoad); OnSerialize( f ); return 0; }

	void LaunchAfterLoad();
	void OnSerialize( IBinSaver &f )
	{
		if ( f.IsReading() )
		{
			currentUnderUpdate = forgottenSounds.end();
			bLaunchAfterLoad = true;
		}
	}
public:

	CSoundScene3D() : bLaunchAfterLoad( false ) {  }
	//
	void Init() ;
	//

	// must be called when new terrain was loaded
	void Init( const int _nMaxX, const int _nMaxY, const int _nMinZ, const int _nMaxZ, const int _VIS_TILE_SIZE );

	WORD AddSound( 	const NDb::SComplexSoundDesc *pStats,
		const CVec3 &vPos,
		const enum ESoundMixType eMixMode,
		const enum ESoundAddMode eAddMode,
		const unsigned int nTimeAfterStart,
		int nVolumeType );
	// удаляет звук из сцены. ID становится инвалидным
	void RemoveSound( const WORD wID );
	// задает новую позицию звуку.
	void SetSoundPos( const WORD wID, const class CVec3 &vPos );
	bool IsSoundFinished( const WORD wID ) const;
	void UpdateSound( const CVec3 &vListener, const CVec3 &vCameraDir, const float fViewRadius );

	void SetSoundSceneMode( const enum ESoundSceneMode eSoundSceneMode );
	enum ESoundSceneMode GetMode() const;

	WORD AddSoundToMap( const NDb::SComplexSoundDesc* pDesc, const CVec3 &vPos );
	void RemoveSoundFromMap( const WORD	wInstanceID );
	void ClearSounds();
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CSoundScene : public ISoundScene
{
	OBJECT_NOCOPY_METHODS( CSoundScene );

	static bool b3DSound;
	ZDATA
	CPtr<ISoundScene> pScene;
	ESoundSceneMode eMode;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pScene); f.Add(3,&eMode); return 0; }

	typedef hash_map<WORD, CDGPtr<CFuncBase<CVec3>, CPtr<CFuncBase<CVec3> > > > CDynamicSoundsMap;
	CDynamicSoundsMap dynamicSounds;

public:

	CSoundScene() {  }
	//
	void Init() ;
	//

	// must be called when new terrain was loaded
	void Init( const int _nMaxX, const int _nMaxY, const int _nMinZ, const int _nMaxZ, const int _VIS_TILE_SIZE );

	WORD AddSound( 	const NDb::SComplexSoundDesc *pStats,
		const CVec3 &vPos,
		const enum ESoundMixType eMixMode,
		const enum ESoundAddMode eAddMode,
		const unsigned int nTimeAfterStart,
		int nVolumeType );

	WORD AddSound( 	const NDb::SComplexSoundDesc *pStats,
		CFuncBase<CVec3> *pPos, // AI pixels
		const enum ESoundMixType eMixMode,
		const enum ESoundAddMode eAddMode,
		const unsigned int nTimeAfterStart,
		int nVolumeType );

	//удаляет звук из сцены. ID становится инвалидным
	void RemoveSound( const WORD wID );
	// задает новую позицию звуку.
	void SetSoundPos( const WORD wID, const class CVec3 &vPos );
	bool IsSoundFinished( const WORD wID ) const;
	void UpdateSound( const CVec3 &vListener, const CVec3 &vCameraDir, const float fViewRadius );

	void SetSoundSceneMode( const enum ESoundSceneMode eSoundSceneMode );
	enum ESoundSceneMode GetMode() const;

	WORD AddSoundToMap( const NDb::SComplexSoundDesc* pDesc, const CVec3 &vPos );
	void RemoveSoundFromMap( const WORD	wInstanceID );
	void ClearSounds();
	
	static bool Is3D();

};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
