#include "stdafx.h"

#include "Stats_B2_M1/DBClientConsts.h"
#include "DebugAckHelper.hpp"
#include "MapObj.h"
#include "ClientAckManagerInternal.h"

#include "Main/GameTimer.h"
#include "Sound/SoundScene.h"
#include "Misc/StrProc.h"
#include "System/Commands.h"

#include <fmt/format.h>

REGISTER_SAVELOAD_CLASS( B2_M1_WORLD, 0x110AE400, CClientAckManager);

CDBPtr<NDb::SClientGameConsts> CClientAckManager::pConsts;
IClientAckManager* AckManager() { return Singleton<IClientAckManager>(); }

const NDb::SAckParameter* GetParam( const NDb::EUnitAckType eAck, const CUnitAcksInfo &acksInfo, const NDb::SClientGameConsts * pConsts )
{
//	NI_ASSERT( acksInfo.find( eAck ) != acksInfo.end(), StrFmt( "wrong ack type %i", eAck ) );
	CUnitAcksInfo::const_iterator posAck = acksInfo.find( eAck );
	if ( posAck == acksInfo.end() )
	{
#ifndef _FINALRELEASE
		if ( NGlobal::GetVar( "m1", 0 ).GetFloat() == 1 )
		{
			static NDb::SAckParameter par1;
			return &par1;
		}
#endif // _FINALRELEASE

		return 0;
	}

	NI_ASSERT( pConsts->acksParameters.size() > posAck->second, fmt::format( "no ack in database {}", int( eAck ) ) );
	if ( !pConsts->acksParameters.empty() )
		return &pConsts->acksParameters[posAck->second];
	else
	{
		static NDb::SAckParameter par;
		return &par;
	}
}

//*******************************************************************
//*		 								   CAckPredicate															*
//*******************************************************************

bool CClientAckManager::CAckPredicate::operator()( const SAck & a ) 
{ 
	const NDb::SAckParameter *pParam = ::GetParam( a.eAck, acksInfo, pConsts );
	if ( pParam )
		return pParam->eAckClass == eType;
	return false;
}

//*******************************************************************
//*		 								   CBoredUnitsContainer												*
//*******************************************************************

int CClientAckManager::CBoredUnitsContainer::operator&( IBinSaver &saver )
{
	saver.Add( 1, &boredUnits );							
	saver.Add( 2, &nCounter );
	saver.Add( 3, &timeLastBored );
	return 0;
}

void CClientAckManager::CBoredUnitsContainer::Clear()
{
	boredUnits.clear();
	nCounter = 0;
}

CClientAckManager::CBoredUnitsContainer::CBoredUnitsContainer() 
: nCounter( 0 ), timeLastBored ( 0 )
{  
}

void CClientAckManager::CBoredUnitsContainer::Copy( const CClientAckManager::CBoredUnitsContainer &cp )
{
	nCounter = cp.nCounter;
	boredUnits = cp.boredUnits;
	timeLastBored = cp.timeLastBored;
}

void CClientAckManager::CBoredUnitsContainer::AddUnit( struct IMOUnit *pUnit )
{
	CBoredUnits::iterator it = boredUnits.find( pUnit );
	if ( it == boredUnits.end() )
	{
		boredUnits[pUnit] = true;
		++nCounter;
	}
}

void CClientAckManager::CBoredUnitsContainer::DelUnit( struct IMOUnit *pUnit )
{
	CBoredUnits::iterator it = boredUnits.find( pUnit );
	if ( it != boredUnits.end() )
	{
		boredUnits.erase( it );
		--nCounter;
	}
}

bool CClientAckManager::CBoredUnitsContainer::SendAck( const NTimer::STime curTime, 
																											const NDb::EUnitAckType eBored, 
																											IClientAckManager *pAckManager,
																											const NTimer::STime timeInterval )
{
	NI_ASSERT( nCounter >= 0, "wrong counter" );
	if ( timeLastBored == 0 )
		timeLastBored = curTime + int (timeInterval * ( 1.0f + 1.0f * rand() / RAND_MAX ) );

	if ( nCounter == 0 || timeLastBored > curTime )
	{
		return false;
	}
	else 
	{
		bool bSayAck = true;
		if ( bSayAck )
		{
			NI_ASSERT( !boredUnits.empty(), "list is empty");
			CPtr<IMOUnit> pUnit ;
			pUnit = (*boredUnits.begin()).first;
			while( !IsValid( pUnit ) && nCounter != 0 )
			{
				DelUnit( pUnit );
				pUnit = 0;
				if ( nCounter != 0 )
					pUnit = (*boredUnits.begin()).first;	
			}
			if ( IsValid( pUnit ) )
				pUnit->SendAcknowledgement( pAckManager, eBored );
			else
				bSayAck = false;

			timeLastBored = curTime + int (timeInterval * ( 1.0f + 1.0f * rand() / RAND_MAX ) );
		}
		return bSayAck;
	}
}

// ************************************************************************************************************************ //
// ** particular actions
// ************************************************************************************************************************ //

int CClientAckManager::operator&(IBinSaver &saver) 
{
	saver.Add( 1, &pConsts );
	if ( saver.IsReading() )
	{
		Init();
	}
	saver.Add( 2, &unitAcks );
	saver.Add( 3, &pLastSelected );
	saver.Add( 4, &nSelectionCounter );
	saver.Add( 5, &boredUnits );	
	saver.Add( 7, &deathAcks );
	saver.Add( 8, &timeLastDeath );	
	saver.Add( 9, &acksPresence );
	return 0;
}

void CClientAckManager::SetClientConsts( const NDb::SClientGameConsts *_pConsts )
{
	pConsts = _pConsts;
	InitConsts();
}

int CClientAckManager::SDeathAck::operator& (IBinSaver &saver) 
{
	saver.Add( 1, &pSound );
	saver.Add( 2, &vPos );
	return 0;
}

int CClientAckManager::SUnitAck::operator& (IBinSaver &saver) 
{
	saver.Add( 1, &acks );
	saver.Add( 2, &wSoundID );
	saver.Add( 3, &timeRun );
	saver.Add( 4, &eCurrentAck );
	return 0;
}

int CClientAckManager::SAck::operator& (IBinSaver &saver) 
{
	saver.Add( 1, &eMixType );
	saver.Add( 2, &pSound );
	saver.Add( 3, &eAck );
	return 0;
}
const NDb::SAckParameter* CClientAckManager::GetParam( const NDb::EUnitAckType eAck ) const
{
	return ::GetParam( eAck, acksInfo, pConsts );
}

bool CClientAckManager::IsNegative( const NDb::EUnitAckType eAck )
{
	return GetParam( eAck ) && GetParam( eAck )->eAckClass == NDb::ACKT_NEGATIVE;
}

CClientAckManager::CClientAckManager() 
: TIME_ACK_WAIT( 0 ), 
	nSelectionCounter( 0 )
{
}

void CClientAckManager::InitConsts()
{
	TIME_ACK_WAIT = 666;
	NUM_SELECTIONS_BEFORE_F_OFF = 3;

	ACK_BORED_INTERVAL = 5000;
	ACK_BORED_INTERVAL_RANDOM = 5000;

	const std::vector<NDb::SAckParameter> &params ( pConsts->acksParameters );
	for ( int i = 0; i < params.size(); ++i )
		acksInfo[params[i].eAckType] = i;
}

void CClientAckManager::Init()
{
	InitConsts();
	pConsoleBuffer = Singleton<IConsoleBuffer>();
	pGameTimer = Singleton<IGameTimer>();
	timeLastDeath = 0;
}

void CClientAckManager::Clear()
{
	unitAcks.clear();
	pLastSelected = 0;
	boredUnits.clear();
	nSelectionCounter = 0;
	boredUnits.clear();
	acksPresence.clear();
}

void CClientAckManager::AddDeathAcknowledgement( const CVec3 &vPos, const NDb::SComplexSoundDesc *pSound, const unsigned int timeSinceStart )
{
	if ( pGameTimer->GetPauseType() != -1 ) 
		return;

	//NI_ASSERT( timeSinceStart < pGameTimer->GetAbsTime(), StrFmt( "somebody dead before times with sound %s, timeSinceStart = %d, absTime =%d", sound.c_str(), timeSinceStart, pGameTimer->GetGameTimer() ) );

	if ( deathAcks.empty() || ( timeSinceStart < pGameTimer->GetAbsTime() &&timeLastDeath < pGameTimer->GetAbsTime() - timeSinceStart) )
	{
		const NDb::SAckParameter* pUnitDiedParam = GetParam( NDb::ACK_UNIT_DIED );
		if ( pUnitDiedParam )
		{
			timeLastDeath = pGameTimer->GetAbsTime() + pUnitDiedParam->nTimeAfterPrevious - timeSinceStart;
			deathAcks.push_back( SDeathAck( vPos, pSound, timeSinceStart ) );
		}
	}
}

void CClientAckManager::AddAcknowledgement( struct IMOUnit *pUnit, const NDb::EUnitAckType eAck, const NDb::SComplexSoundDesc *pSound, const int nSet, const unsigned int nTimeSinceStart )
{
	if ( GetParam( eAck ) == 0 )
		return;
	NDb::EAckClass eType = GetParam( eAck )->eAckClass;
	if ( pGameTimer->GetPauseType() != -1 && NDb::ACKT_NOTIFY != eType ) 
		return;


	if ( NDb::ACK_UNIT_DIED == eAck )
	{
		if ( deathAcks.empty() || 
			( nTimeSinceStart < pGameTimer->GetAbsTime() && timeLastDeath < pGameTimer->GetAbsTime() - nTimeSinceStart) )
		{
		}
		else
			return;
	}

	//выяснить какой тип у этого аска
	//NI_ASSERT( acksInfo.find( eAck ) != acksInfo.end(), StrFmt( "unredistered Ack %d, ignored", eAck ) );
	NI_ASSERT( !pUnit->IsRefInvalid(), "added ack from invalid unit" );

	if ( acksInfo.find( eAck ) == acksInfo.end() )
		return;

	SAck ack;
	ack.pSound = pSound;
	ack.eAck = eAck ;
	ack.eMixType = GetParam( eAck )->ePosition == NDb::ACK_POS_INTERFACE ? SFX_INTERFACE : SFX_MIX_IF_TIME_EQUALS;

	switch( eType )
	{
	case NDb::ACKT_POSITIVE:
		if ( unitAcks[pUnit].timeRun < pGameTimer->GetAbsTime() && 0 == unitAcks[pUnit].wSoundID )
		{
			unitAcks[pUnit].timeRun = pGameTimer->GetAbsTime() + TIME_ACK_WAIT;
			unitAcks[pUnit].acks.push_back( ack );
		}

		break;
	case NDb::ACKT_NEGATIVE:
		{
			// найти все позитивы в очереди и убрать.
			CAckPredicate  pr( NDb::ACKT_POSITIVE, acksInfo, pConsts );
			CAcks::iterator positives = remove_if( unitAcks[pUnit].acks.begin(), unitAcks[pUnit].acks.end(), pr );
			if ( positives == unitAcks[pUnit].acks.end() ) 
			{
				// ни одного Positive, Negative игнорировать
			}
			else
			{
				unitAcks[pUnit].acks.erase( positives, unitAcks[pUnit].acks.end() );
				// добавить этот аск в очередь
				unitAcks[pUnit].acks.push_back( ack );
			}
		}

		break;
	case NDb::ACKT_SELECTION:
		if ( 0 == unitAcks[pUnit].wSoundID )
		{
			if ( pLastSelected == pUnit )
				++nSelectionCounter;
			else
				nSelectionCounter = 0;
			pLastSelected = pUnit;
			if ( nSelectionCounter >= NUM_SELECTIONS_BEFORE_F_OFF )
			{
				if ( eAck != NDb::ACK_SELECTION_TO_MUCH )
				{
					pUnit->AIUpdateAcknowledgement( NDb::ACK_SELECTION_TO_MUCH, this, nSet );
					pLastSelected = 0;
				}
				else
					unitAcks[pUnit].acks.push_back( ack );
			}
			else
				unitAcks[pUnit].acks.push_back( ack );
		}

		break;
	case NDb::ACKT_NOTIFY:
		// check if the same ack exists in the list
		{
			if ( unitAcks[pUnit].eCurrentAck != ack.eAck &&
				unitAcks[pUnit].acks.end() == find( unitAcks[pUnit].acks.begin(), unitAcks[pUnit].acks.end(), ack ) )
			{
				unitAcks[pUnit].acks.push_back( ack );
			}
		}
		// 

		break;
	case NDb::ACKT_BORED:
		unitAcks[pUnit].acks.push_back( ack );

		break;
	}
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CClientAckManager::RegisterAck( SUnitAck *ack, const NTimer::STime curTime )
{
	ack->eCurrentAck = (*ack->acks.begin()).eAck;
	acksPresence[ack->eCurrentAck] = curTime;
}

void CClientAckManager::UnregisterAck( SUnitAck *ack )
{
	ack->eCurrentAck = -1;
}

void CClientAckManager::Update( struct ISoundScene *pSoundScene )
{
	NTimer::STime curTime = pGameTimer->GetAbsTime();

	if ( pGameTimer->GetPauseType() == -1 )
	{
		for ( std::unordered_map<int, CBoredUnitsContainer>::iterator it = boredUnits.begin(); it != boredUnits.end(); ++it )
		{
			const NDb::EUnitAckType eType = static_cast<NDb::EUnitAckType>( (*it).first );
			if ( GetParam( eType ) )
				(*it).second.SendAck( curTime, eType, this, GetParam( eType )->nTimeAfterPrevious  );
		}
	}

	//death acks
	for ( CDeathAcks::iterator it = deathAcks.begin(); it != deathAcks.end(); )
	{
		pSoundScene->AddSound( it->pSound, it->vPos, SFX_MIX_IF_TIME_EQUALS, SAM_ADD_N_FORGET, it->timeSinceStart, 1 );
		it = deathAcks.erase( it );
	}

	for ( CUnitsAcks::iterator it = unitAcks.begin(); it != unitAcks.end(); ++it )
	{
		IMOUnit * pUnit = (*it).first;
		SUnitAck &ack = (*it).second;
		if ( 0 != ack.wSoundID && pSoundScene->IsSoundFinished( ack.wSoundID ) )
		{
			pSoundScene->RemoveSound( ack.wSoundID );
			ack.wSoundID = 0;
			UnregisterAck( &ack );
		}

		if ( 0 == ack.wSoundID )
		{
			// run next acknowledgement
			if ( ack.acks.begin() != ack.acks.end() )
			{
				SAck &addedAck = *ack.acks.begin();
				const NDb::SAckParameter *pCurrentAskInfo = GetParam( addedAck.eAck );

				// если время для позитивного не пришло
				if ( !pCurrentAskInfo || pCurrentAskInfo->eAckClass == NDb::ACKT_POSITIVE && ack.timeRun > curTime )
					continue;

				//проверить не запущен ли уже Ack данного типа.
				// если играется, то звук не запускать.
				if ( acksPresence.find(addedAck.eAck) == acksPresence.end() ||
					curTime - acksPresence[int(addedAck.eAck)] >= pCurrentAskInfo->nTimeAfterPrevious )
				{
					//run acknowledgement sound
					if ( addedAck.pSound )
					{
						const CVec3 vPos( pUnit->GetCenter() );
						ack.wSoundID = pSoundScene->AddSound( addedAck.pSound, vPos, addedAck.eMixType, SAM_NEED_ID, 0, 1 );
						RegisterAck( &ack, curTime );
					}
#ifndef _FINALRELEASE
					if ( NGlobal::GetVar( "m1", 0 ).GetFloat() == 1 && NGlobal::GetVar( "no_visual_acks", 0 ).GetFloat() == 0 )
					{
						int i = 0;
						while ( debugAckNames[i].eAckType != NDb::ACK_NONE && debugAckNames[i].eAckType != addedAck.eAck )
							++i;

						std::string szMessage;
						if ( debugAckNames[i].eAckType != addedAck.eAck )
							szMessage = fmt::format( "unknown ack {}", static_cast<int>(addedAck.eAck) );
						else
							szMessage = debugAckNames[i].pszName;
						szMessage = "														ack " + szMessage;

						WriteToPipe( PIPE_CHAT, szMessage.c_str(), 0xff00ff00 );
					}
#endif
				}
				ack.acks.pop_front();
			}
		}
	}
}

void CClientAckManager::UnitDead( struct IMOUnit *pUnit, struct ISoundScene *pSoundScene )
{
	IMOUnit * pTipaUnit = pUnit;
	CUnitsAcks::iterator it = unitAcks.find( pTipaUnit ) ;

	if ( it != unitAcks.end() )
	{
		SUnitAck &ack = (*it).second;
		if ( 0 != ack.wSoundID )
		{
			UnregisterAck( &ack );
			pSoundScene->RemoveSound( ack.wSoundID );
		}
		unitAcks.erase(it);
	}
	

	for ( BoredUnits::iterator it = boredUnits.begin(); it != boredUnits.end(); ++it )
		it->second.DelUnit( pTipaUnit );

}

void CClientAckManager::RegisterAsBored( const NDb::EUnitAckType eBored, struct IMOUnit *pObject )
{
	NI_ASSERT( acksInfo.find( eBored ) != acksInfo.end(), fmt::format( "unredistered Ack {}", int( eBored ) ) );
	NI_ASSERT( !pObject->IsRefInvalid(), "added ack from invalid unit" );

	boredUnits[eBored].AddUnit( pObject );
}

void CClientAckManager::UnRegisterAsBored( const NDb::EUnitAckType eBored, struct IMOUnit *pObject )
{
	NI_ASSERT( acksInfo.find( eBored ) != acksInfo.end(), fmt::format( "unredistered Ack {}", int( eBored ) ) );
	boredUnits[eBored].DelUnit( pObject );
}

IClientAckManager *CreateAckManager()
{
	return new CClientAckManager();
}


