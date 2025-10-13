#pragma once

#include "Sound.h"

#include <cstdint>

//клетка, содержащая звук
class CSoundCell : public CObjectBase
{
	OBJECT_BASIC_METHODS( CSoundCell );
	int nRadius;												// радиус звучания этой клетки(в клетках)
	typedef std::list< CPtr<CSound> > CSounds;
	CSounds sounds;												// звуки этой клетки
	void RecountForDelete();
	NTimer::STime timeLastCombatHear;
	bool IsSoundHearable( const CSound *pSound, const int nRadius ) const;
public:
	CSoundCell();
	void Clear();
	int operator&( IBinSaver &saver );

	int GetRadius() const { return nRadius; }
	void SetRadius( int nRad ) { nRadius = nRad; }
	void AddSound( class CSound *pSound );	// добавляет звук и пересчитывает радиус звучания
	void RemoveSound( const uint16_t wID, ISFX * pSFX =0 );					// удаляет звук и пересчитывает радиус звучания
	CSound * GetSound( const uint16_t wID );
	const CSound * GetSound( const uint16_t wID ) const;

	// удалить звуки с ID == 0 , которые завершились
	// все доигравшие звуки пометить как доигравшие
	void Update( ISFX * pSFX );

	bool HasSounds() const { return sounds.begin() != sounds.end(); }
	//		bool HearSounds() const { return hearableCells.begin() != hearableCells.end(); } 

	void SetLastHearCombat( const NTimer::STime hearTime );
	bool IsCombat() const;


	// для всех звуков, которые слышны на расстояние больше nRadius и еще не доиграли
	template <class TEnumFunc> 
		void EnumHearableSounds( int nRadius, TEnumFunc func )
	{
		for ( CSounds::iterator it = sounds.begin(); it != sounds.end(); ++it )
		{
			if ( IsSoundHearable( *it, nRadius )	)
				func( *it );
		}
	}
	// для перебора всех звуков.
	template <class TEnumFunc>
		void EnumAllSounds( TEnumFunc func, int nRadius )
	{
		if ( nRadius <= GetRadius() )
		{
			for ( CSounds::iterator it = sounds.begin(); it != sounds.end(); ++it )
			{
				if ( IsSoundHearable( *it, nRadius ) )
					func( *it );
				else
					func( *it, false );
			}
		}
		else
		{
			// значит ни один из звуков этой клетки не слышен
			for ( CSounds::iterator it = sounds.begin(); it != sounds.end(); ++it )
				func( *it, false );
		}
	}
};


