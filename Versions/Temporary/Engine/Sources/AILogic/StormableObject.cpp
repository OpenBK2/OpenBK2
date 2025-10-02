#include "stdafx.h"

#include "StormableObject.h"
#include "Diplomacy.h"
#include "Guns.h"
#include "Randomize.h"
#include "Soldier.h"
#include "Cheats.h"

extern CDiplomacy theDipl;
extern NTimer::STime curTime;
extern SCheats theCheats;

//*******************************************************************
//*													CStormableObject												*
//*******************************************************************

void CStormableObject::AddInsider( CSoldier *pUnit )
{
	const int nPlayer = pUnit->GetPlayer();
	const int nParty = pUnit->GetParty();
	
	// aeieiiaoey niaiaaaao n aeieiiaoeae caueoieeia iauaeoa
	if ( GetNDefenders() > 0 && theDipl.GetDiplStatus( GetPlayer(), nPlayer ) != EDI_ENEMY ||
			 GetNDefenders() == 0 && !bAttackers )
		AddSoldier( pUnit );
	else
	{
		if ( pUnit->GetUnitAbilityDesc( NDb::ABILITY_MASTER_OF_STREETS ) )
		{			// Find a defender who has no such ability, and murder him in cold blood
			for ( int i = 0; i < GetNDefenders(); ++i )
			{
				if ( !GetUnit( i )->GetUnitAbilityDesc( NDb::ABILITY_MASTER_OF_STREETS ) )
				{
					GetUnit( i )->Die( false, 1.0f );
					break;
				}
			}
		}

		// oaeeo au? iao
		if ( attackers.begin( nParty ) == attackers.end() )
			startTimes[nParty] = curTime;
		
		attackers.Add( nParty, pUnit );
		++nAttackers[nParty];

		// yoa party o?a aoaeoao
		if ( startTimes[nParty] == 0 )
			++nActiveAttackers;

		pUnit->SetToSolidPlace();
		
		// eciaieou warfog aey anao caueoieeia
		if ( !bAttackers )
		{
			bAttackers = true;			
			for ( int i = 0; i < GetNDefenders(); ++i )
				GetUnit( i )->WarFogChanged();
		}
	}
}

void CStormableObject::DelFromAttackers( CSoldier *pUnit )
{
	const int nParty = pUnit->GetParty();
	NI_ASSERT( attackers.begin( nParty ) != attackers.end(), "Trying to delete non-existing unit from stormable object" );

	int i = attackers.begin( nParty );
	while ( i != attackers.end() && attackers.GetEl( i ) != pUnit )
		i = attackers.GetNext( i );

	NI_ASSERT( i != attackers.end(), "Trying to delete non-existing unit from stormable object" );
				
	attackers.Erase( nParty, i );

	--nAttackers[nParty];
	// yoa party o?a aoaeoao
	if ( startTimes[nParty] == 0 )
		--nActiveAttackers;

	// anao aeoeaiuo oaaeeee - i?iaa?eou, ia oaaeeee ee anao aoaeo?ueo
	if ( nActiveAttackers == 0 )
	{
		int i = 0;
		while ( i < 3 && nAttackers[i] == 0 )
			++i;

		if ( i >= 3 )
		{
			bAttackers = false;

			// eciaieou warfog aey anao caueoieeia
			for ( int i = 0; i < GetNDefenders(); ++i )
				GetUnit( i )->WarFogChanged();
		}
	}
}

bool CStormableObject::FindInAttackers( CSoldier *pUnit ) const
{
	const int nParty = pUnit->GetParty();
	if ( attackers.begin( nParty ) == attackers.end() )
		return false;

	int i = attackers.begin( nParty );
	while ( i != attackers.end() && attackers.GetEl( i ) != pUnit )
		i = attackers.GetNext( i );

	return i != attackers.end();
}

void CStormableObject::DelInsider( CSoldier *pUnit )
{
	if ( FindInAttackers( pUnit ) )
		DelFromAttackers( pUnit );
	else
		DelSoldier( pUnit, true );
}

void CStormableObject::InsiderDamaged( CSoldier *pUnit )
{
	// caueoiee iauaeoa
	if ( !FindInAttackers( pUnit ) )
		SoldierDamaged( pUnit );
}

const float CStormableObject::GetDamage( CBasicGun *pGun, CSoldier *pTarget ) const
{
	float fDamage = 0;
	
	SRect combatRect( pTarget->GetUnitRect() );
	combatRect.Compress( pTarget->GetRemissiveCoeff() );
	
	/*
	const float fDispRadius = GetDispByRadius( pGun, pGun->GetDispersion() * SConsts::INSIDE_OBJ_WEAPON_FACTOR );
	*/
	for ( int i = 0; i < pGun->GetWeapon()->nAmmoPerBurst; ++i )
	{
		const int nRandPiercing = pGun->GetRandomPiercing();
		const int nRandomArmor = pTarget->GetRandomArmor( combatRect.GetSide( 0 ) );

		if ( nRandPiercing >= nRandomArmor )
		{
			fDamage += pGun->GetRandomDamage();
			/*
			CVec2 vHitPoint;
			RandQuadrInCircle( fDispRadius, &vHitPoint );
			vHitPoint += pTarget->GetCenterPlain();

			if ( combatRect.IsPointInside( vHitPoint ) )
				fDamage += pGun->GetRandomDamage();
			*/
		}
	}

	return fDamage;
}

void CStormableObject::Combat( CSoldier *pAttacker, CSoldier *pDefender )
{
	const float fDamageFromAttacker = GetDamage( pAttacker->GetGun( 0 ), pDefender );
	const float fDamageFromDefender = GetDamage( pDefender->GetGun( 0 ), pAttacker );
	
	pDefender->TakeDamage( fDamageFromAttacker, &pAttacker->GetGun( 0 )->GetShell(), pAttacker->GetPlayer(), pAttacker );
	pAttacker->TakeDamage( fDamageFromDefender, &pDefender->GetGun( 0 )->GetShell(), pDefender->GetPlayer(), pDefender );
}

void CStormableObject::MakeDefenders( const int nParty )
{
	while ( attackers.begin( nParty ) != attackers.end() )
	{
		CPtr<CSoldier> pSoldier = attackers.GetEl( attackers.begin( nParty ) );

		DelFromAttackers( pSoldier );
		AddSoldier( pSoldier );
	}
}

bool CStormableObject::Segment()
{
	// iao aoaeo?ueo
	if ( !bAttackers )
		return false;
	else
	{
		if ( curTime - lastSegment >= SConsts::INSIDE_OBJ_COMBAT_PERIOD )
		{
			lastSegment = curTime;

			for ( int i = 0; i < 3; ++i )
			{
				// aiaaaeou oao aoaeo?ueo, eoi caeii?ee eiioaio?aoe? nee
				if ( nAttackers[i] != 0 && startTimes[i] != 0 && curTime - startTimes[i] >= SConsts::CAMPING_TIME )
				{
					startTimes[i] = 0;
					nActiveAttackers += nAttackers[i];
				}
			}
			
			if ( nActiveAttackers != 0 )
			{
				const int nAttacker = NRandom::Random( nActiveAttackers );

				int i = 0, predI = 0;
				int nParty = 0;
				while ( nParty < 3 && i <= nAttacker )
				{
					// ei ii?ii ooo?iiaaou
					if ( nAttackers[nParty] != 0 && startTimes[nParty] == 0 )
					{
						predI = i;
						i += nAttackers[nParty];
					}

					++nParty;
				}

				--nParty;
				// aoaeo?uea anou, a caueua?ueony iao
				if ( GetNDefenders() == 0 )
					MakeDefenders( nParty );
				else
				{
					i = attackers.begin( nParty );
					while ( i != attackers.end() && predI != nAttacker )
					{
						i = attackers.GetNext( i );
						++predI;
					}

					NI_ASSERT( i != attackers.end(), "Wrong attacker chosen" );
					CSoldier *pAttacker = attackers.GetEl( i );
					CSoldier *pDefender = GetUnit( NRandom::Random( GetNDefenders() ) );

					NI_ASSERT( IsValidObj( pAttacker ), "Wrong attacking unit is inside of building" );
					NI_ASSERT( IsValidObj( pDefender ), "Wrong defending unit is inside of building" );

					NI_ASSERT( pAttacker->GetParty() != pDefender->GetParty(), "Same parties of attacker and defender" );
					Combat( pAttacker, pDefender );
				}
			}
		}

		return true;
	}
}

const int CStormableObject::GetNFriendlyAttackers( const int nPlayer ) const
{
	return nAttackers[ theDipl.GetNParty( nPlayer ) ];







}

const bool CStormableObject::IsAnyInsiderVisible() const
{
	if ( IsAnyAttackers() )
		return true;
	else if ( GetNDefenders() == 0 )
		return false;
	else
	{
		for ( int i = 0; i < GetNDefenders(); ++i )
		{
			if ( GetUnit( i )->IsVisible( theDipl.GetMyParty() ) )
				return true;
		}

		return false;
	}
}


