#pragma once

#include "SFX.h"
#include "DBSound.h"
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CSubstSound;
namespace NDb
{
	enum ESoundType;
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//звук
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CSound : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( CSound );
	int wID;														// 
	CPtr<ISound> pSample;								// звук, который нужно играть
	CPtr<CSubstSound> pSubstitute;						// звук, который играется вместо pSample

	NDb::ESoundType eCombatType;				// поведение звука во время боя
	NTimer::STime timeBegin;						// начало проигрыша звука. 
	NTimer::STime timeToPlay;						// время начала звука
	NTimer::STime timeBeginDim;

	CDBPtr<NDb::SSoundDesc> pDesc;
	enum ESoundMixType eMixType;
	CVec3 vPos;
	CVec3 vSpeed;
	bool bLooped;

	int nMaxRadius, nMinRadius;

	bool bStartedMark;											// звук запустили уже
	bool bFinishedMark;											// звук уже отыграл
	bool bDimMark;												  // начать затухание звука
	NTimer::STime timeLastPosUpdate;
public:
	CSound() {  }
	CSound( int _nID, const NDb::SSoundDesc *pDesc, interface ISound *pSound, 
		      const enum ESoundMixType eMixType, const CVec3 &vPos, const bool bLooped,
					const NDb::ESoundType eCombatType, float fMinRadius, float fMaxRadius );
	~CSound();
	int operator&( IBinSaver &saver );


	// сктолько самплов уже отыграло
	unsigned int GetSamplesPassed();
	bool IsTimeToFinish() const;

	//время проигрыша этого звука
	NTimer::STime GetPlayTime() const { return timeToPlay; }

	// замена звука
	bool IsSubstituted() const { return pSubstitute != 0; }
	CSubstSound * GetSubst();
	void Substitute( CSubstSound *_pSubstitute, NTimer::STime nStartTime );
	void UnSubstitute();

	bool IsLooped() const { return bLooped; }

	//начало проигрыша этого звука
	void MarkStarted();
	bool IsMarkedStarted() const { return bStartedMark; }
	//конец проигрыша звука
	bool IsMarkedFinished() const { return bFinishedMark; }
	void MarkFinished( bool bFinished = true ) { bFinishedMark=bFinished; }
	// для затухания звука при удалении
	void MarkToDim( const NTimer::STime time );
	bool IsMarkedForDim() const { return bDimMark; }
	// из-за затухания (по времени) или из-за расстояния громкость может быть не полной.
	float GetVolume( const NTimer::STime time, const float fDist  ) const;

	void SetBeginTime( const NTimer::STime time );
	const NTimer::STime & GetBeginTime() const {return timeBegin;}


	int GetRadiusMax() const;								// дистанция (в клетках) на котрой этот звук слышен
	ISound * GetSound();
	ESoundMixType GetMixType() const { return eMixType; }
	const int GetID() { return wID; }
	const NDb::SSoundDesc* GetDesc() const;

	void SetPos( const class CVec3 & vPos );
	void SetSpeed( const CVec3 &_vSpeed );
	const CVec3& GetPos() const { return vPos; }
	const CVec3& GetSpeed() const { return vSpeed; }

	NDb::ESoundType GetCombatType() const { return eCombatType; }
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
