#pragma once

#include "ActionCommand.h"
#include "System/XmlSaver.h"

namespace NDb
{
	struct SHPObjectRPGStats;
}

struct SAIUnitCmd
{
	ZDATA
		EActionCommand nCmdType;								// command type
		CVec2 vPos;														// for ground pointing commands
		int nObjectID;														// when client send object pointing command this filed is object's id
		bool bFromExplosion;										// for death from explosion
		// команда ACTION_COMMAND_CALL_BOMBERS - число бомберов
		// команда ACTION_COMMAND_ENTER:		 0 - войти в здание, 1 - войти в окоп
		// команда ACTION_COMMAND_ATTACK_OBJECT: 0 - атаковать не окоп, 1 - атаковать окоп
		// ACTION_COMMAND_DROP_BOMBS_TO_TARGET: 0 - unit, 1 - building
		float fNumber;
		int nNumber;
		bool bFromAI;			// если true, то команда пришла от клиента или от генерала
		// CRAP{ may be here we can act in other way - our goal is to get rid of old int-ids
		// CDBPtr<NDb::SHPObjectRPGStats> pTarget; 
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nCmdType); f.Add(3,&vPos); f.Add(4,&nObjectID); f.Add(5,&bFromExplosion); f.Add(6,&fNumber); f.Add(7,&nNumber); f.Add(8,&bFromAI); return 0; }
private:
	// forbidden
	SAIUnitCmd( const EActionCommand &_nCmdType, int, int );
	SAIUnitCmd( const EActionCommand &_nCmdType, CObjectBase*, int );
	SAIUnitCmd( const EActionCommand &_nCmdType, float, float, int );
public:
	SAIUnitCmd() : nCmdType ( ACTION_COMMAND_MOVE_TO ), vPos( -1, -1 ), bFromAI( true ), nObjectID( 0 ), fNumber( 0 ), bFromExplosion( 0 ), nNumber( 0 ) { }
	explicit SAIUnitCmd( const EActionCommand &_nCmdType )
		: nCmdType( _nCmdType ), vPos( -1, -1 ), fNumber( 0 ), bFromExplosion( false ), bFromAI(true), nObjectID( 0 ), nNumber( 0 ) { }

	SAIUnitCmd( const EActionCommand &_nCmdType, const int _nObjectID )
		: nCmdType( _nCmdType ), nObjectID( _nObjectID ), vPos( -1, -1 ), fNumber( 0 ), bFromExplosion( false ), bFromAI(true), nNumber( 0 ) { }

	SAIUnitCmd( const EActionCommand &_nCmdType, const int _nObjectID, const float _fNumber )
		: nCmdType( _nCmdType ), nObjectID( _nObjectID ), fNumber( _fNumber ), vPos( -1, -1 ), bFromExplosion( false ), bFromAI(true), nNumber( 0 ) { }

	SAIUnitCmd( const EActionCommand &_nCmdType, const CVec2 &_vPos )
		: nCmdType( _nCmdType ), vPos( _vPos ), fNumber( 0 ), bFromExplosion( false ), bFromAI(true), nObjectID( 0 ), nNumber( 0 ) { }
	SAIUnitCmd( const EActionCommand &_nCmdType, const float x, const float y )
		: nCmdType( _nCmdType ), vPos( x, y ), fNumber( 0 ), bFromExplosion( false ), bFromAI(true), nObjectID( 0 ), nNumber( 0 ) { }

	SAIUnitCmd( const EActionCommand &_nCmdType, const CVec2 &_vPos, const float _fNumber )
		: nCmdType( _nCmdType ), vPos( _vPos ), fNumber( _fNumber ), bFromExplosion( false ), bFromAI(true), nObjectID( 0 ), nNumber( 0 ) { }

	SAIUnitCmd( const EActionCommand &_nCmdType, const float x, const float y, const float _fNumber )
		: nCmdType( _nCmdType ), vPos( x, y ), fNumber( _fNumber ), bFromExplosion( false ), bFromAI(true), nObjectID( 0 ), nNumber( 0 ) { }

	SAIUnitCmd( const EActionCommand &_nCmdType, const float _fNumber )
		: nCmdType( _nCmdType ), fNumber( _fNumber ), vPos( -1, -1 ), bFromExplosion( false ), bFromAI(true), nObjectID( 0 ), nNumber( 0 ) { }

	SAIUnitCmd( const EActionCommand &_nCmdType, bool _fromExpl )
		: nCmdType( _nCmdType ), bFromExplosion( _fromExpl ), vPos( -1, -1 ), fNumber( 0 ), bFromAI(true), nObjectID( 0 ), nNumber( 0 ) { }

	SAIUnitCmd( const EActionCommand &_nCmdType, const float _fNumber, bool _fromExpl )
		: nCmdType( _nCmdType ), bFromExplosion( _fromExpl ), vPos( -1, -1 ), fNumber( _fNumber ), bFromAI(true), nObjectID( 0 ), nNumber( 0 ) { }

	SAIUnitCmd( const EActionCommand &_nCmdType, const CVec2 &_vPos, const float _fNumber, const bool _bFromExplosion )
		: nCmdType( _nCmdType ), vPos( _vPos ), fNumber( _fNumber ), bFromExplosion( _bFromExplosion ), bFromAI(true), nObjectID(  ), nNumber( 0 ) { }
	//
	int operator&( IXmlSaver &saver )
	{
		saver.Add( "Command", &nCmdType );
		saver.Add( "Position", &vPos );
		saver.Add( "bFromExplosionFlag", &bFromExplosion );
		saver.Add( "NumberFlag", &fNumber );
		saver.Add( "NumberFlagInt", &nNumber );
		saver.Add( "FromAIFlag", &bFromAI );
		saver.Add( "ObjectID", &nObjectID );

		return 0;
	}
};


