#include "stdafx.h"

#include "AnimationMnemonics.h"
#include "../libdb/Manipulator.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


const char CMayaAnimationMnemonics::DECIMAL_NUMBERS[] = "0123456789";


CMayaAnimationMnemonics::CMayaAnimationMnemonics() : CMnemonicsCollector<int>( NDb::ANIMATION_UNKNOWN, "" )
{
	Insert( NDb::ANIMATION_IDLE,										"IDLE" );
	Insert( NDb::ANIMATION_IDLE_DOWN,							"IDLE_DOWN" );
	Insert( NDb::ANIMATION_IDLE_REST,							"IDLE_REST" );
	Insert( NDb::ANIMATION_IDLE_DIVING,						"IDLE_DIVING" );
	Insert( NDb::ANIMATION_MOVE,										"MOVE" );
	Insert( NDb::ANIMATION_MARCH,									"MARCH" );
	Insert( NDb::ANIMATION_WALK,										"WALK" );
	Insert( NDb::ANIMATION_CRAWL,									"CRAWL" );
	Insert( NDb::ANIMATION_DIVING,									"DIVING" );
	Insert( NDb::ANIMATION_LIE,										"LIE" );
	Insert( NDb::ANIMATION_STAND,									"STAND" );
	Insert( NDb::ANIMATION_WEAPON_HIDE,						"WEAPON_HIDE" );
	Insert( NDb::ANIMATION_WEAPON_SHOW,						"WEAPON_SHOW" );
	Insert( NDb::ANIMATION_SHOOT,									"SHOOT" );
	Insert( NDb::ANIMATION_SHOOT_DOWN,							"SHOOT_DOWN" );
	Insert( NDb::ANIMATION_SHOOT_TRENCH,						"SHOOT_TRENCH" );
	Insert( NDb::ANIMATION_THROW,									"THROW" );
	Insert( NDb::ANIMATION_THROW_TRENCH,						"THROW_TRENCH" );
	Insert( NDb::ANIMATION_THROW_DOWN,							"THROW_DOWN" );
	Insert( NDb::ANIMATION_DEATH,									"DEATH" );
	Insert( NDb::ANIMATION_DEATH_DOWN,							"DEATH_DOWN" );
	Insert( NDb::ANIMATION_DEATH_DIVING,						"DEATH_DIVING" );
	Insert( NDb::ANIMATION_DEATH_FATALITY,					"FATALITY" );
	Insert( NDb::ANIMATION_DEATH_FATALITY_FLOOR1,	"FATALITY_FLOOR1" );
	Insert( NDb::ANIMATION_DEATH_FATALITY_FLOOR2,	"FATALITY_FLOOR2" );
	Insert( NDb::ANIMATION_USE,										"USE" );
	Insert( NDb::ANIMATION_USE_DOWN,								"USE_DOWN" );
	Insert( NDb::ANIMATION_USE_LIE,								"USE_LIE" );
	Insert( NDb::ANIMATION_ENTRENCH,								"ENTRENCH" );
	Insert( NDb::ANIMATION_POINT,									"POINT" );
	Insert( NDb::ANIMATION_POINT_DOWN,							"POINT_DOWN" );
	Insert( NDb::ANIMATION_BINOCULARS,							"BINOCULARS" );
	Insert( NDb::ANIMATION_BINOCULARS_DOWN,				"BINOCULARS_DOWN" );
	Insert( NDb::ANIMATION_AIMING,									"AIMING" );
	Insert( NDb::ANIMATION_AIMING_TRENCH,					"AIMING_TRENCH" );
	Insert( NDb::ANIMATION_AIMING_DOWN,						"AIMING_DOWN" );
	Insert( NDb::ANIMATION_INSTALL,								"INSTALL_ATTACK" );
	Insert( NDb::ANIMATION_UNINSTALL,							"UNINSTALL_ATTACK" );
	Insert( NDb::ANIMATION_INSTALL_ROT,						"INSTALL_ROTATE" );
	Insert( NDb::ANIMATION_UNINSTALL_ROT,					"UNINSTALL_ROTATE" );
	Insert( NDb::ANIMATION_INSTALL_PUSH,						"INSTALL_PUSH" );
	Insert( NDb::ANIMATION_UNINSTALL_PUSH,					"UNINSTALL_PUSH" );
};


CAnimationMnemonics::CAnimationMnemonics() : CMnemonicsCollector<int>( NDb::ANIMATION_UNKNOWN, "" )
{
	Insert( NDb::ANIMATION_IDLE,										"ANIMATION_IDLE" );
	Insert( NDb::ANIMATION_IDLE_DOWN,							"ANIMATION_IDLE_DOWN" );
	Insert( NDb::ANIMATION_IDLE_REST,							"ANIMATION_IDLE_REST" );
	Insert( NDb::ANIMATION_IDLE_DIVING,						"ANIMATION_IDLE_DIVING" );
	Insert( NDb::ANIMATION_MOVE,										"ANIMATION_MOVE" );
	Insert( NDb::ANIMATION_MARCH,									"ANIMATION_MARCH" );
	Insert( NDb::ANIMATION_WALK,										"ANIMATION_WALK" );
	Insert( NDb::ANIMATION_CRAWL,									"ANIMATION_CRAWL" );
	Insert( NDb::ANIMATION_DIVING,									"ANIMATION_DIVING" );
	Insert( NDb::ANIMATION_LIE,										"ANIMATION_LIE" );
	Insert( NDb::ANIMATION_STAND,									"ANIMATION_STAND" );
	Insert( NDb::ANIMATION_WEAPON_HIDE,						"ANIMATION_WEAPON_HIDE" );
	Insert( NDb::ANIMATION_WEAPON_SHOW,						"ANIMATION_WEAPON_SHOW" );
	Insert( NDb::ANIMATION_SHOOT,									"ANIMATION_SHOOT" );
	Insert( NDb::ANIMATION_SHOOT_DOWN,							"ANIMATION_SHOOT_DOWN" );
	Insert( NDb::ANIMATION_SHOOT_TRENCH,						"ANIMATION_SHOOT_TRENCH" );
	Insert( NDb::ANIMATION_THROW,									"ANIMATION_THROW" );
	Insert( NDb::ANIMATION_THROW_TRENCH,						"ANIMATION_THROW_TRENCH" );
	Insert( NDb::ANIMATION_THROW_DOWN,							"ANIMATION_THROW_DOWN" );
	Insert( NDb::ANIMATION_DEATH,									"ANIMATION_DEATH" );
	Insert( NDb::ANIMATION_DEATH_DOWN,							"ANIMATION_DEATH_DOWN" );
	Insert( NDb::ANIMATION_DEATH_DIVING,						"ANIMATION_DEATH_DIVING" );
	Insert( NDb::ANIMATION_DEATH_FATALITY,					"ANIMATION_DEATH_FATALITY" );
	Insert( NDb::ANIMATION_DEATH_FATALITY_FLOOR1,	"ANIMATION_DEATH_FATALITY_FLOOR1" );
	Insert( NDb::ANIMATION_DEATH_FATALITY_FLOOR2,	"ANIMATION_DEATH_FATALITY_FLOOR2" );
	Insert( NDb::ANIMATION_USE,										"ANIMATION_USE" );
	Insert( NDb::ANIMATION_USE_DOWN,								"ANIMATION_USE_DOWN" );
	Insert( NDb::ANIMATION_USE_LIE,								"ANIMATION_USE_LIE" );
	Insert( NDb::ANIMATION_ENTRENCH,								"ANIMATION_ENTRENCH" );
	Insert( NDb::ANIMATION_POINT,									"ANIMATION_POINT" );
	Insert( NDb::ANIMATION_POINT_DOWN,							"ANIMATION_POINT_DOWN" );
	Insert( NDb::ANIMATION_BINOCULARS,							"ANIMATION_BINOCULARS" );
	Insert( NDb::ANIMATION_BINOCULARS_DOWN,				"ANIMATION_BINOCULARS_DOWN" );
	Insert( NDb::ANIMATION_AIMING,									"ANIMATION_AIMING" );
	Insert( NDb::ANIMATION_AIMING_TRENCH,					"ANIMATION_AIMING_TRENCH" );
	Insert( NDb::ANIMATION_AIMING_DOWN,						"ANIMATION_AIMING_DOWN" );
	Insert( NDb::ANIMATION_INSTALL,								"ANIMATION_INSTALL" );
	Insert( NDb::ANIMATION_UNINSTALL,							"ANIMATION_UNINSTALL" );
	Insert( NDb::ANIMATION_INSTALL_ROT,						"ANIMATION_INSTALL_ROT" );
	Insert( NDb::ANIMATION_UNINSTALL_ROT,					"ANIMATION_UNINSTALL_ROT" );
	Insert( NDb::ANIMATION_INSTALL_PUSH,						"ANIMATION_INSTALL_PUSH" );
	Insert( NDb::ANIMATION_UNINSTALL_PUSH,					"ANIMATION_UNINSTALL_PUSH" );
};


NDb::EAnimationType CMayaAnimationMnemonics::Get( const string &rszMnemonicType, string *pszMnemonicLabel, UINT *pnNumber )
{
	int nPos = 0;
	while( nPos != string::npos )
	{
		nPos = rszMnemonicType.find_first_of( DECIMAL_NUMBERS, nPos );
		const string szMnemonic = rszMnemonicType.substr( 0, nPos );
		NDb::EAnimationType animationType = (NDb::EAnimationType)( GetValue( szMnemonic ) );
		if ( animationType != NDb::ANIMATION_UNKNOWN )
		{
			if ( pnNumber )
			{
				if ( nPos != -1 ) 
				{
					const string szNumber = rszMnemonicType.substr( nPos );

					UINT nNumber = INVALID_NODE_ID;
					if ( sscanf( szNumber.c_str(), "%d", &nNumber ) == 1 )
					{
						( *pnNumber ) = nNumber;
					}
				}
				else
					*pnNumber = 0;
			}
			if ( pszMnemonicLabel )
			{
				( *pszMnemonicLabel ) = szMnemonic;
			}
			return animationType;
		}
		if ( nPos != string::npos )
			++nPos;
	}
	//
	return NDb::ANIMATION_UNKNOWN;
}


CMayaAnimationMnemonics typeMayaAnimationMnemonics;
CAnimationMnemonics typeAnimationMnemonics;

// basement storage  


