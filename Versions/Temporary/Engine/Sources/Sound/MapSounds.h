#pragma once

#include "Misc/TypeConvertor.h"
#include "System/FreeIDs.h"
#include "Misc/2Darray.h"
#include "IntPair.h"
#include "Misc/HashFuncs.h"

#include <cstdint>

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
			std::unordered_map<uint16_t,CVec3> instanceIDs;
			int nCount;
			SMapSounds() : nCount( 0 ) {  }
		};
		struct SMaxCountPredicate
		{
			bool operator()( const std::pair<uint16_t,SMapSounds> &s1, const std::pair<uint16_t,SMapSounds> &s2 ) const
			{ return s1.second.nCount > s2.second.nCount; }
		};

		struct SPlaying
		{
			uint16_t wSoundTypeID;								// type of sound
			uint16_t wInstanceID;									// instance of sound
			uint16_t wSceneID;										// if added to scene, then scene ID
			//
			SPlaying() { Clear(); }
			void Clear() { wInstanceID = 0; wSceneID = 0; wSoundTypeID = 0; }
		};

		SPlaying playingLoopedSound;							// текущий играющий тип звука (зацикленный)
		SPlaying playingSound;										// текущий незацикленный звук

		// по типам звука списки
		typedef std::unordered_map<uint16_t, SMapSounds> CellSounds;
		CellSounds cellSounds;
		CellSounds cellLoopedSounds;
		NTimer::STime timeNextRun;			// время следующего проигрыша звука

		void RemoveSound( CellSounds *pCellSounds, const uint16_t wInstanceID );
	public:
		CMapSoundCell() : timeNextRun( 0 ) { }

		void AddSound( const uint16_t wSoundID, const CVec3 &vPos, const RegisteredSounds &registeredSounds, const uint16_t wInstanceID, const bool bLooped );
		void RemoveSound( const uint16_t wInstanceID, struct ISoundScene * pScene );
		void Update( struct ISoundScene * pScene, const RegisteredSounds &registeredSounds );
	};

	CFreeIds soundIDs;									// для регистрации звуков
	CFreeIds instanceIDs;								// каждый звук будет миеть уникальный ID
	RegisteredSounds registeredSounds;	// список названий звуков, которые есть в сцене

	// 2d map of sound cells (all sounds are assumed on ground)
	CArray2D<CMapSoundCell> mapCells;
	// cell - sound instance id
	std::unordered_map<uint16_t, SIntThree> cells;

	struct ISoundScene * pSoundScene;
	NTimer::STime timeNextUpdate;

public:
	CMapSounds() : pSoundScene( 0 ), timeNextUpdate( 0 ) {  }
	void SetSoundScene( struct ISoundScene *pSoundScene );
	void Update( const CVec3 &vListener, const float fViewRadius );
	void Clear();

	void InitSizes( const int nSizeX, const int nSizeY );
	uint16_t AddSound( const CVec3 &vPos, const NDb::SComplexSoundDesc* pDesc );
	void RemoveSound( const uint16_t wInstanceID );
};

