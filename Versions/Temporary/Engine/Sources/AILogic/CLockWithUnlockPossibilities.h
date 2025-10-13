#pragma once

#include <cstdint>

//в заданном тайл сете запоминает состояние залоченности
// и сохраняет его. 
// после этого есть возможность восстановить начальное состояние залоченности
class CAIUnit;
class CLockWithUnlockPossibilities
{
	
	void Unlock();
	void Lock();

	ZDATA
	std::vector<uint8_t> formerTilesType;
	std::list<SVector> pathTiles;

	bool bLocked;
	uint8_t bAIClass; //
protected:
	SRect bigRect; // весь этот Rect будет пройден танком.
	public: ZEND int operator&( IBinSaver &f ) { f.Add(2,&formerTilesType); f.Add(3,&pathTiles); f.Add(4,&bLocked); f.Add(5,&bAIClass); f.Add(6,&bigRect); return 0; }
protected:
	bool TryLockAlongTheWay( const bool bLock, const uint8_t _bAIClass ) ;

public:
	CLockWithUnlockPossibilities()
		: bLocked(false){}

	bool LockRect( const SRect &rect, const uint8_t _bAIClass )
	{
		if ( bLocked )	
		{
			TryLockAlongTheWay(false, bAIClass );
		}
		bigRect = rect;
		bLocked = TryLockAlongTheWay( true, _bAIClass );
		return bLocked ;
	}
	void UnlockIfLocked()
	{
		if ( bLocked )
		{
			bLocked = false;
			TryLockAlongTheWay(false, bAIClass);
		}
	}
};


