#include "stdafx.h"

#include "DifficultyLevel.h"
#include "Diplomacy.h"
#include "..\Misc\2Darray.h"

CDifficultyLevel theDifficultyLevel;
extern CDiplomacy theDipl;

void CDifficultyLevel::Init()
{
	levelsNames.resize( 3 );
	levelsNames[0] = "Easy";
	levelsNames[1] = "Normal";
	levelsNames[2] = "Hard";

	coeffNames.resize( EM_MAX_NUM );
	coeffNames[0] = "Silhouette";
	coeffNames[1] = "Piercing";
	coeffNames[2] = "Damage";
	coeffNames[3] = "RotateSpeed";
	coeffNames[4] = "Dispersion";

	partiesNames.resize( 2 );
	partiesNames[0] = "Friends";
	partiesNames[1] = "Enemies";
	
	coeff.resize( 3 );
	coeff[0].SetSizes( EM_MAX_NUM, levelsNames.size() );
	coeff[1].SetSizes( EM_MAX_NUM, levelsNames.size() );
	coeff[2].SetSizes( EM_MAX_NUM, levelsNames.size() );

	coeff[2].FillEvery( 1.0f );

	for ( int nLevel = 0; nLevel < levelsNames.size(); ++nLevel )
	{
		for ( int nParty = 0; nParty < 2; ++nParty )
		{
			for ( int nCoeff = 0; nCoeff < coeffNames.size(); ++nCoeff )
			{
				//CRAP{ UNTILL CONSTST SYSTEM
				//const string szEntryName = "AI.Levels." + levelsNames[nLevel] + "." + partiesNames[nParty] + "." + coeffNames[nCoeff];
				//coeff[nParty][nLevel][nCoeff] = GetGlobalVar( szEntryName, 1.0f );
				coeff[nParty][nLevel][nCoeff] = 1.0f;
				//CRAP}
			}
		}
	}
	if ( theDipl.IsNetGame() )
		nLevel = 1;

	nCheatLevel = 255;
}

void CDifficultyLevel::SetLevel( const int _nLevel )
{ 
	nLevel = _nLevel;
}

void CDifficultyLevel::OnSerialize( IBinSaver &saver )
{
	if ( saver.IsReading() )
		Init();
}


