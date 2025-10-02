#pragma once

#include "../Misc/TypeConvertor.h"
#include "../System/FreeIDs.h"
#include "../Misc/2dArray.h"
#include "IntPair.h"
#include "../Misc/HashFuncs.h"

namespace NDb
{
	struct SComplexSoundDesc;
}

// для звуков, прописанных в точке карты
class CMapSounds
{
public: 
	int operator&( IBinSaver &saver ); private:;

	typedef CTypeConvertor< CDBPtr<NDb::SComplexSoundDesc>, int, SDBPtrHash > RegisteredSounds;

	//
	class CMapSoundCell
	{
	public: 
		int operator&( IBinSaver &saver ); 
	private:

		// for similar map sounds
		struct SMapSounds
		{
		public: int operator&( IBinSaver &saver ); private:;
		public:
			hash_map<WORD,CVec3> instanceIDs;
			int nCount;
			SMapSounds() : nCount( 0 ) {  }
		};
		struct SMaxCountPredicate
		{
			bool operator()( const pair<WORD,SMapSounds> &s1, const pair<WORD,SMapSounds> &s2 ) const
			{ return s1.second.nCount > s2.second.nCount; }
		};

		struct SPlaying
		{
			WORD wSoundTypeID;								// type of sound
			WORD wInstanceID;									// instance of sound
			WORD wSceneID;										// if added to scene, then scene ID
			//
			SPlaying() { Clear(); }
			void Clear() { wInstanceID = 0; wSceneID = 0; wSoundTypeID = 0; }
		};

		SPlaying playingLoopedSound;							// текущий играющий тип звука (зацикленный)
		SPlaying playingSound;										// текущий незацикленный звук

		// по типам звука списки
		typedef hash_map<WORD, SMapSounds> CellSounds;
		CellSounds cellSounds;
		CellSounds cellLoopedSounds;
		NTimer::STime timeNextRun;			// время следующего проигрыша звука

		void RemoveSound( CellSounds *pCellSounds, const WORD wInstanceID );
	public:
		CMapSoundCell() : timeNextRun( 0 ) { }

		void AddSound( const WORD wSoundID, const CVec3 &vPos, const RegisteredSounds &registeredSounds, const WORD wInstanceID, const bool bLooped );
		void RemoveSound( const WORD wInstanceID, interface ISoundScene * pScene );
		void Update( interface ISoundScene * pScene, const RegisteredSounds &registeredSounds );
	};

	CFreeIds soundIDs;									// для регистрации звуков
	CFreeIds instanceIDs;								// каждый звук будет миеть уникальный ID
	RegisteredSounds registeredSounds;	// список названий звуков, которые есть в сцене

	// 2d map of sound cells (all sounds are assumed on ground)
	CArray2D<CMapSoundCell> mapCells;
	// cell - sound instance id
	hash_map<WORD, SIntThree> cells;

	interface ISoundScene * pSoundScene; 
	NTimer::STime timeNextUpdate;

public:
	CMapSounds() : pSoundScene( 0 ), timeNextUpdate( 0 ) {  }
	void SetSoundScene( interface ISoundScene *pSoundScene );
	void Update( const CVec3 &vListener, const float fViewRadius );
	void Clear();

	void InitSizes( const int nSizeX, const int nSizeY );
	WORD AddSound( const CVec3 &vPos, const NDb::SComplexSoundDesc* pDesc );
	void RemoveSound( const WORD wInstanceID );
};

