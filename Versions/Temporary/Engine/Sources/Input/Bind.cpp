#include "StdAfx.h"

#include <dinput.h>
#include "../Misc/StrProc.h"
//#include "..\System\Streams.h"
//#include "..\System\BasicChunk.h"
#include "../Input/Bind.h"
#include "../Input/BindInternal.h"
using namespace NStr;

namespace NInput
{

// Global vars
typedef hash_map<int, SActionInfo> TActionsMap;
typedef hash_map<string, SCommand> TCommandsMap;

static vector<string> sections;
static list<SGameMessage> events;

TActionsMap& GetActions()
{
	static TActionsMap actionsMap;
	return actionsMap;
}
TCommandsMap& GetCommands()
{
	static TCommandsMap commandsMap;
	return commandsMap;
}

static void Update( DWORD dwTime );
static void ProcessMessage( const NInput::SMessage &mMsg );
static bool ProcessCommandMessage( const NInput::SMessage &mMsg, SCommand &cCommand );
static bool IsMappingBSubsetA( const SMapping &mSetA, const SMapping &mSetB );
static bool IsSameMappingExist( const SCommand &sCommand, const SMapping &sMapping );
static void CrossMappings( const SMapping &sBaseSet, SMapping *pSubSet );

// Public

const vector<string>& GetSections()
{
	return sections;
}

void SetSection( const string &_szSection, bool bUpdate )
{
	vector<string> temp( 1 );
	temp[0] = _szSection;
	SetSection( temp, bUpdate );
}

void SetSection( const vector<string> &_sections, bool bUpdate )
{
	if ( sections == _sections )
		return;

	sections = _sections;
	if ( bUpdate )
		UpdateBinds();
}

static bool IsSectionInSet( const string &szSection )
{
	if ( szSection.empty() )
		return true;

	return find( sections.begin(), sections.end(), szSection ) != sections.end();
}

void Bind( EMappingType eType, const string &sCmd, const string &szSection, const vector<string> &szControlsSet )
{
	SMapping sMapping;
	TActionsMap &sActions = GetActions();
	TCommandsMap &sCommandsMap = GetCommands();

	SCommand &sCommand = sCommandsMap[sCmd];
	
	if ( szControlsSet.empty() )
		return;

	sMapping.mType = eType;
	if ( eType == MTYPE_EVENT_UP )
		sMapping.mType = MTYPE_EVENT;
	if ( eType == MTYPE_SLIDER_MINUS )
		sMapping.mType = MTYPE_SLIDER;

	sMapping.nPower = 100;
	if ( eType == MTYPE_EVENT_UP )
		sMapping.bActive = true;
	if ( ( eType == MTYPE_EVENT_UP ) || ( eType == MTYPE_SLIDER_MINUS ) )
		sMapping.nPower = -sMapping.nPower;

	sMapping.szSection = szSection;
	sMapping.actionsSet.resize( szControlsSet.size() );
	sMapping.fullActionsSet.resize( szControlsSet.size() );
	for ( int nTemp = 0; nTemp < szControlsSet.size(); nTemp++ )
	{
		int nAction = GetControlID( szControlsSet[nTemp] );
		sMapping.actionsSet[nTemp] = nAction;
		sMapping.fullActionsSet[nTemp] = nAction;

		SActionInfo &sInfo = sActions[nAction];
		if ( sInfo.eState != STATE_INITIALIZED )
		{
			if ( sInfo.eState != STATE_CONFIGPRESET )
				sInfo.fCoeff = 1.0f;;

			sInfo.eState = STATE_INITIALIZED;
			sInfo.szName = szControlsSet[nTemp];
			GetControlInfo( nAction, &sInfo.eType, &sInfo.fGranularity );
		}
	}

	if ( IsSameMappingExist( sCommand, sMapping ) )
		return;

	sCommand.mappingsList.push_back( sMapping );
}

void Unbind( const string &szCmd )
{
	TCommandsMap &sCommandsMap = GetCommands();

	TCommandsMap::iterator iTemp = sCommandsMap.find( szCmd );
	if ( iTemp == sCommandsMap.end() )
		return;

	SCommand &sCommand = iTemp->second;
	for ( list<SMapping>::iterator iMapping = sCommand.mappingsList.begin(); iMapping != sCommand.mappingsList.end(); )
	{
		if ( !IsSectionInSet( iMapping->szSection ) )
		{
			iMapping++;
			continue;
		}

		if ( iMapping->mType != MTYPE_UNKNOWN )
			iMapping = sCommand.mappingsList.erase( iMapping );
		else
			iMapping++;
	}
}

void UnbindAll()
{
	TCommandsMap &sCommandsMap = GetCommands();

	for ( TCommandsMap::iterator iTemp = sCommandsMap.begin(); iTemp != sCommandsMap.end(); iTemp++ )
		Unbind( iTemp->first );
}

void GetBind( const string &szCmd, list<SBind> *pRes )
{
	TActionsMap &sActions = GetActions();
	TCommandsMap &sCommandsMap = GetCommands();

	pRes->clear();

	if ( sCommandsMap.find( szCmd ) == sCommandsMap.end() )
		return;

	SCommand &sCommand = sCommandsMap[szCmd];
	for ( list<SMapping>::iterator iTempMapping = sCommand.mappingsList.begin(); iTempMapping != sCommand.mappingsList.end(); ++iTempMapping )
	{
		SBind &sBind = *( pRes->insert( pRes->end() ) );
		sBind.eType = iTempMapping->mType;
		sBind.szSection = iTempMapping->szSection;

		for( int nTemp = 0; nTemp < iTempMapping->actionsSet.size(); nTemp++ )
		{
			const SActionInfo &sTempInfo = sActions[iTempMapping->actionsSet[nTemp]];
			sBind.controlsSet.push_back( sTempInfo.szName );
		}
	}
}

void UpdateBinds()
{
	TCommandsMap &sCommandsMap = GetCommands();

	for ( hash_map<string, SCommand>::iterator iTempCommand = sCommandsMap.begin(); iTempCommand != sCommandsMap.end(); ++iTempCommand )
	{
		SCommand &sCommand = iTempCommand->second;
		for ( list<SMapping>::iterator iTempMapping = sCommand.mappingsList.begin(); iTempMapping != sCommand.mappingsList.end(); ++iTempMapping )
			iTempMapping->blockingGroupsSet.clear();
	}

	for ( hash_map<string, SCommand>::iterator iTempCommand = sCommandsMap.begin(); iTempCommand != sCommandsMap.end(); ++iTempCommand )
	{
		SCommand &sCommand = iTempCommand->second;

		for ( list<SMapping>::iterator iTempMapping = sCommand.mappingsList.begin(); iTempMapping != sCommand.mappingsList.end(); ++iTempMapping )
		{
			iTempMapping->bDisabled = false;

			if ( !IsSectionInSet( iTempMapping->szSection ) )
			{
				iTempMapping->bActive = false;
				iTempMapping->bDisabled = true;
				iTempMapping->sAccumulator.Reset();
			}
		}
	}

	for ( hash_map<string, SCommand>::iterator iTempCommand1 = sCommandsMap.begin(); iTempCommand1 != sCommandsMap.end(); ++iTempCommand1 )
	{
		SCommand &sCommand1 = iTempCommand1->second;
		hash_map<string, SCommand>::iterator iNext = iTempCommand1;
		iNext++;
		for ( hash_map<string, SCommand>::iterator iTempCommand2 = iNext; iTempCommand2 != sCommandsMap.end(); ++iTempCommand2 )
		{
			SCommand &sCommand2 = iTempCommand2->second;

			for ( list<SMapping>::iterator iTempMapping1 = sCommand1.mappingsList.begin(); iTempMapping1 != sCommand1.mappingsList.end(); ++iTempMapping1 )
			{
				for ( list<SMapping>::iterator iTempMapping2 = sCommand2.mappingsList.begin(); iTempMapping2 != sCommand2.mappingsList.end(); ++iTempMapping2 )
				{
					if ( iTempMapping1->bDisabled || iTempMapping2->bDisabled )
						continue;

					if ( IsMappingBSubsetA( *iTempMapping1, *iTempMapping2 ) )
						CrossMappings( *iTempMapping1, &(*iTempMapping2) );
					if ( IsMappingBSubsetA( *iTempMapping2, *iTempMapping1 ) )
						CrossMappings( *iTempMapping2, &(*iTempMapping1) );
				}
			}
		}
	}
}

float GetControlCoeff( const string &szControl )
{
	TActionsMap &sActions = GetActions();

	int nAction = GetControlID( szControl );
	if ( nAction == -1 )
		return 1.0f;

	TActionsMap::iterator iTemp = sActions.find( nAction );
	if ( iTemp == sActions.end() )
		return 1.0f;

	return iTemp->second.fCoeff;
}

void SetControlCoeff( const string &szControl, float fCoeff )
{
	TActionsMap &sActions = GetActions();

	int nAction = GetControlID( szControl );
	if ( nAction == -1 )
		return;

	SActionInfo &sInfo = sActions[nAction];
	if ( sInfo.eState == STATE_DEFAULT )
		sInfo.eState = STATE_CONFIGPRESET;

	sInfo.fCoeff = fCoeff;
}

bool GetEvent( SGameMessage *pSGameMessage )
{
	NInput::SMessage msg;
	if ( events.empty() )
	{
		if ( !NInput::GetMessage( &msg ) )
		{
			Update( msg.tTime );
			pSGameMessage->commands.clear();
			pSGameMessage->mMessage = msg;
			return false;
		}
		NInput::ProcessMessage( msg );
		return GetEvent( pSGameMessage );
	}
	*pSGameMessage = events.front();
	events.pop_front();
	return true;
}

void MakeEvent( SGameMessage *pMsg,  const string &szGameMessage, int _nParam1, int _nParam2, EControlType ct )
{
	*pMsg = SGameMessage();
	TCommandsMap::iterator iTemp = GetCommands().find( szGameMessage );
	if ( iTemp != GetCommands().end() )
		pMsg->commands.push_back( &(iTemp->second) );

	pMsg->mMessage.cType = ct;
	pMsg->mMessage.tTime = GetTickCount();
	pMsg->mMessage.bState = false;
	pMsg->mMessage.nAction = -1;

#ifndef _FINALRELEASE
	pMsg->szEventName = szGameMessage;
#endif

	pMsg->nParam1 = _nParam1;
	pMsg->nParam2 = _nParam2;
}

void PostEvent( const string &szEvent, int _nParam1, int _nParam2 )
{
	SGameMessage sEvent;
	MakeEvent( &sEvent, szEvent, _nParam1, _nParam2, CT_UNKNOWN );
	events.push_back( sEvent );
}

void PostWinEvent( const string &szEvent, int _nParam1, int _nParam2 )
{
	SGameMessage sEvent;
	MakeEvent( &sEvent, szEvent, _nParam1, _nParam2, CT_WINDOWS );
	events.push_back( sEvent );
}

void PurgeEvents()
{
	PumpMessages( true );
	SGameMessage gameMessage;
	while ( GetEvent( &gameMessage ) ) {}
}

void PurgeUIEvents()
{
	PumpMessages( true );
	while ( true )
	{
		NInput::SMessage msg;
		if ( !NInput::GetMessage( &msg ) )
			break;
		NInput::ProcessMessage( msg );
	}
	for ( list<SGameMessage>::iterator it = events.begin(); it != events.end(); )
	{
		SGameMessage &msg = *it;
		if ( msg.mMessage.cType == CT_UNKNOWN )
			++it;
		else
		{
			it = events.erase( it );
		}
	}
}

float GetCommandCoeff( const string &szControl )
{
	TCommandsMap::iterator iTemp = GetCommands().find( szControl );
	if ( iTemp != GetCommands().end() )
		return iTemp->second.fCoeff;

	return 1.0f;
}

void SetCommandCoeff( const string &szControl, float fCoeff )
{
	TCommandsMap::iterator iTemp = GetCommands().find( szControl );
	if ( iTemp == GetCommands().end() )
		return;

	iTemp->second.fCoeff = fCoeff;
}

// CBind

CBind::CBind( const string &sCmd ): fDelta( 0 )
{

#ifndef _FINALRELEASE
	szBindName = sCmd;
#endif

	TCommandsMap &sCommands = GetCommands();
	pBindCommand = &sCommands[sCmd];
}

bool CBind::IsActive() const
{
	return pBindCommand->bActive;
}

float CBind::GetDelta()
{
	float fRes = fDelta;
	fDelta = 0;
	return fRes;
}

float CBind::GetSpeed() const
{
	return pBindCommand->fSpeed;
}

bool CBind::ProcessEvent( const SGameMessage &eEvent )
{
	if ( eEvent.mMessage.cType == CT_TIME )
	{
		fDelta += pBindCommand->fDelta;
		return false;
	}
	
	if ( find( eEvent.commands.begin(), eEvent.commands.end(), pBindCommand ) == eEvent.commands.end() )
		return false;

	if ( ( eEvent.mMessage.cType == CT_UNKNOWN || eEvent.mMessage.cType == CT_WINDOWS ) && ( eEvent.mMessage.nAction == -1 ) )
		return true;
	
	return ProcessCommandMessage( eEvent.mMessage, *pBindCommand );
}

//
// Internal functions
//

//! Update all commands
static void Update( DWORD dwTime )
{
	TCommandsMap &sCommands = GetCommands();
	
	for ( TCommandsMap::iterator iTempCommand = sCommands.begin(); iTempCommand != sCommands.end(); ++iTempCommand )
	{
		SCommand &sCommand = iTempCommand->second;
		
		bool bActive = false;
		int64 nValue = 0;
		for( list<SMapping>::iterator iTempMapping = sCommand.mappingsList.begin(); iTempMapping != sCommand.mappingsList.end(); ++iTempMapping )
		{
			if ( iTempMapping->bDisabled )
				continue;

			nValue += iTempMapping->sAccumulator.Sample( dwTime );
			bActive |= iTempMapping->bActive;
		}

		sCommand.bActive = bActive;
		sCommand.fDelta = nValue * sCommand.fCoeff / 100000.0f;
		sCommand.fSpeed = sCommand.fDelta / ( ( dwTime - sCommand.dwTime ) * ( 1 / 1024.0f ) );
		sCommand.dwTime = dwTime;
	}
}

static bool IsBindActive( const SMapping &sMapping )
{
	TActionsMap &sActions = GetActions();

	int nCount = 0;
	bool bSetActive = true;
	for( int nTemp = 0; nTemp < sMapping.actionsSet.size(); nTemp++ )
	{
		const SActionInfo &sTempInfo = sActions[sMapping.actionsSet[nTemp]];
		if ( sTempInfo.bActive )
			nCount++;
	}

	if ( nCount != sMapping.actionsSet.size() )
		bSetActive = false;

	if ( bSetActive )
	{
		for( list<vector<int> >::const_iterator iTempSet = sMapping.blockingGroupsSet.begin(); iTempSet != sMapping.blockingGroupsSet.end(); iTempSet++ )
		{
			int nBlockingCount = 0;
			const vector<int> &sBlockingSet = *iTempSet;

			for( int nTemp = 0; nTemp < sBlockingSet.size(); nTemp++ )
			{
				const SActionInfo &sTempInfo = sActions[sBlockingSet[nTemp]];
				
				if ( sTempInfo.bActive )
					nBlockingCount++;
				else if ( ( sTempInfo.eType == CT_AXIS ) || ( sTempInfo.eType == CT_LIMAXIS ) )
					nBlockingCount++;
			}

			if ( nBlockingCount == sBlockingSet.size() )
			{
				bSetActive = false;
				break;
			}
		}
	}

	if ( ( sMapping.mType == MTYPE_EVENT ) && ( sMapping.nPower < 0 ) )
		bSetActive = !bSetActive;

	return bSetActive;
}

//! Handle message and generate prediction
static void ProcessMessage( const NInput::SMessage &mMsg )
{
	SGameMessage gameMessage;
	TActionsMap &sActions = GetActions();
	TCommandsMap &sCommands = GetCommands();
	
	gameMessage.commands.clear();
	gameMessage.mMessage = mMsg;

	SActionInfo &sActionInfo = sActions[mMsg.nAction];
	sActionInfo.bActive = mMsg.bState;

	for ( hash_map<string, SCommand>::iterator iTempCommand = sCommands.begin(); iTempCommand != sCommands.end(); ++iTempCommand )
	{
		SCommand &sCommand = iTempCommand->second;

		for ( list<SMapping>::iterator iTempMapping = sCommand.mappingsList.begin(); iTempMapping != sCommand.mappingsList.end(); ++iTempMapping )
		{
			if ( iTempMapping->bDisabled || iTempMapping->actionsSet.empty() )
				continue;
			if ( find( iTempMapping->fullActionsSet.begin(), iTempMapping->fullActionsSet.end(), mMsg.nAction ) == iTempMapping->fullActionsSet.end() )
				continue;

			SActionInfo &sActionInfo = sActions[mMsg.nAction];
			sActionInfo.bActive = mMsg.bState;
			bool bSetActive = IsBindActive( *iTempMapping );

			if ( ( mMsg.cType == CT_AXIS ) || ( mMsg.cType == CT_LIMAXIS ) )
				sActionInfo.bActive = false;

			if ( !bSetActive )
			{
				iTempMapping->bActive = false;
				iTempMapping->sAccumulator.sKeyAccumulator.Deactivate( mMsg.tTime );
				iTempMapping->sAccumulator.sPOVAccumulator.Deactivate( mMsg.tTime );
				iTempMapping->sAccumulator.sLimAxisAccumulator.Deactivate( mMsg.tTime );
				continue;
			}
			
			if ( iTempMapping->bActive && ( iTempMapping->mType == MTYPE_EVENT ) )
				continue;

			iTempMapping->bActive = true;
			gameMessage.commands.push_back( &sCommand );
			continue;
		}
	}
	
	events.push_back( gameMessage );
}

//! Handle messages for command
static bool ProcessCommandMessage( const NInput::SMessage &mMsg, SCommand &sCommand )
{
	TActionsMap &sActions = GetActions();

	SActionInfo &sActionInfo = sActions[mMsg.nAction];
	sActionInfo.bActive = mMsg.bState;

	for( list<SMapping>::iterator iTempMapping = sCommand.mappingsList.begin(); iTempMapping != sCommand.mappingsList.end(); ++iTempMapping )
	{
		if ( iTempMapping->bDisabled || iTempMapping->actionsSet.empty() )
			continue;
		if ( find( iTempMapping->fullActionsSet.begin(), iTempMapping->fullActionsSet.end(), mMsg.nAction ) == iTempMapping->fullActionsSet.end() )
			continue;
		
		SActionInfo &sActionInfo = sActions[mMsg.nAction];
		sActionInfo.bActive = mMsg.bState;

		bool bSetActive = IsBindActive( *iTempMapping );
		
		if ( ( mMsg.cType == CT_AXIS ) || ( mMsg.cType == CT_LIMAXIS ) )
			sActionInfo.bActive = false;

		if ( !bSetActive )
			continue;

		if ( ( iTempMapping->mType == MTYPE_SLIDER ) && ( mMsg.cType == CT_KEY ) )
			iTempMapping->sAccumulator.sKeyAccumulator.Activate( iTempMapping->nPower * sActionInfo.fCoeff, mMsg.tTime );
		else if ( ( iTempMapping->mType == MTYPE_SLIDER ) && ( mMsg.cType == CT_AXIS ) )
			iTempMapping->sAccumulator.sAxisAccumulator.Add( iTempMapping->nPower * sActionInfo.fCoeff * mMsg.nParam / sActionInfo.fGranularity, mMsg.tTime );
		else if ( ( iTempMapping->mType == MTYPE_SLIDER ) && ( mMsg.cType == CT_LIMAXIS ) )
			iTempMapping->sAccumulator.sLimAxisAccumulator.Add( iTempMapping->nPower * sActionInfo.fCoeff * mMsg.nParam / sActionInfo.fGranularity, mMsg.tTime );
		else if ( ( iTempMapping->mType == MTYPE_SLIDER ) && ( mMsg.cType == CT_POV ) )
		{
			if ( mMsg.nParam != -1 )
				iTempMapping->sAccumulator.sPOVAccumulator.Activate( iTempMapping->nPower * sActionInfo.fCoeff * mMsg.nParam / sActionInfo.fGranularity, mMsg.tTime );
			else 
				iTempMapping->sAccumulator.sPOVAccumulator.Deactivate( mMsg.tTime );
		}
		else if ( ( iTempMapping->mType == MTYPE_EVENT ) && ( mMsg.cType == CT_KEY ) )
		{
			int nPower = iTempMapping->nPower * ( mMsg.bState ? 1 : -1 );
			if ( nPower < 0 )
				return false;

			iTempMapping->sAccumulator.sKeyAccumulator.Activate( 0, mMsg.tTime );
			return true;
		}
		
		return true;
	}
	
	return false;
}

//! Crossover detection
static bool IsMappingBSubsetA( const SMapping &mSetA, const SMapping &mSetB )
{
	for ( vector<int>::const_iterator iTempB = mSetB.actionsSet.begin(); iTempB != mSetB.actionsSet.end(); ++iTempB )
	{
		bool bInSet = false;
		
		for ( vector<int>::const_iterator iTempA = mSetA.actionsSet.begin(); iTempA != mSetA.actionsSet.end(); ++iTempA )
		{
			if ( *iTempB == *iTempA )
				bInSet = true;
		}

		if ( !bInSet )
			return false;
	}

	return true;
}

static bool IsSameMappingExist( const SCommand &sCommand, const SMapping &sMapping )
{
	for ( list<SMapping>::const_iterator iTempMapping = sCommand.mappingsList.begin(); iTempMapping != sCommand.mappingsList.end(); ++iTempMapping )
	{
		if ( ( sMapping.mType != iTempMapping->mType ) || ( sMapping.nPower != iTempMapping->nPower ) || ( sMapping.bActive != iTempMapping->bActive ) || ( sMapping.szSection != iTempMapping->szSection ) )
			continue;
		if ( iTempMapping->actionsSet.size() != sMapping.actionsSet.size() )
			continue;

		int nSameActionsCount = 0;
		for ( vector<int>::const_iterator iTempAction = iTempMapping->actionsSet.begin(); iTempAction != iTempMapping->actionsSet.end(); iTempAction++ )
		{
			if ( find( sMapping.actionsSet.begin(), sMapping.actionsSet.end(), *iTempAction ) != sMapping.actionsSet.end() )
				nSameActionsCount++;
		}

		if ( nSameActionsCount == sMapping.actionsSet.size() )
			return true;
	}

	return false;
}

static void CrossMappings( const SMapping &sBaseSet, SMapping *pSubSet )
{
	vector<int> blockingGroupSet;
	TActionsMap &sActions = GetActions();

	for ( int nBaseTemp = 0; nBaseTemp < sBaseSet.actionsSet.size(); nBaseTemp++ )
	{
		int nAction = sBaseSet.actionsSet[nBaseTemp];
		bool bCrossover = false;
		
		for ( int nSubTemp = 0; nSubTemp < pSubSet->actionsSet.size(); nSubTemp++ )
		{
			if ( pSubSet->actionsSet[nSubTemp] == nAction )
				bCrossover = true;
		}
		
		const SActionInfo &sTempInfo = sActions[nAction];
		if ( !bCrossover && ( sTempInfo.eType == CT_KEY ) )
		{
			blockingGroupSet.push_back( nAction );
			pSubSet->fullActionsSet.push_back( nAction );
		}
	}

	if ( !blockingGroupSet.empty() )
		pSubSet->blockingGroupsSet.push_back( blockingGroupSet );
}

}; // NAMESPACE

static void CommandBindInner( const string &szID, const vector<wstring> &_paramsSet )
{
	vector<wstring> paramsSet( _paramsSet );

	if ( paramsSet.size() < 2 )
	{
		csSystem << "usage: " << szID << "[+,-,!]'bind-id' 'KEY' + ..." << endl;
		csSystem << "'+' - slider plus, '-' - slider minus, '!' - on release" << endl;
		return;
	}

	vector<wstring>::const_iterator iTemp = paramsSet.begin();

	string szCommand( NStr::ToMBCS( *paramsSet.begin() ) );
	paramsSet.erase( paramsSet.begin() );
	NStr::TrimBoth( szCommand, "\t\n\r\'" );

	NInput::EMappingType eType = NInput::MTYPE_UNKNOWN;
	switch ( szCommand[0] )
	{
	case '+':
		eType = NInput::MTYPE_SLIDER;
		NStr::TrimLeft( szCommand, '+' );
		break;
	case '-':
		eType = NInput::MTYPE_SLIDER_MINUS;
		NStr::TrimLeft( szCommand, '-' );
		break;
	case '!':
		eType = NInput::MTYPE_EVENT_UP;
		NStr::TrimLeft( szCommand, '!' );
		break;
	default:
		eType = NInput::MTYPE_EVENT;
		break;
	}

	vector<string> controlsSet;
	controlsSet.reserve( paramsSet.size() );
	for ( vector<wstring>::const_iterator iTemp = paramsSet.begin(); iTemp != paramsSet.end(); iTemp++ )
	{
		if ( iTemp->compare( L"+" ) == 0 )
			continue;

		string szWord( NStr::ToMBCS( *iTemp ) );
		NStr::TrimBoth( szWord, "\t\n\r\'" );
		controlsSet.push_back( szWord );
	}

	string szSection;
	const vector<string> &sections = NInput::GetSections();
	if ( !sections.empty() )
	{
		for ( int nTemp = 0; nTemp < sections.size(); ++nTemp )
			NInput::Bind( eType, szCommand, sections[nTemp], controlsSet );
	}
	else
		NInput::Bind( eType, szCommand, "", controlsSet );
}

static void CommandBind( const string &szID, const vector<wstring> &_paramsSet, void *pContext )
{
	vector<wstring> paramsSet( _paramsSet );
	for ( int k = 1; k < paramsSet.size(); ++k )
	{
		if ( paramsSet[k] == wstring(L"'CTRL'") )
		{
			paramsSet[k] = L"'LCTRL'";
			CommandBind( szID, paramsSet, pContext );
			paramsSet[k] = L"'RCTRL'";
			CommandBind( szID, paramsSet, pContext );
			return;
		}
		if ( paramsSet[k] == L"'ALT'" )
		{
			paramsSet[k] = L"'LALT'";
			CommandBind( szID, paramsSet, pContext );
			paramsSet[k] = L"'RALT'";
			CommandBind( szID, paramsSet, pContext );
			return;
		}
		if ( paramsSet[k] == L"'SHIFT'" )
		{
			paramsSet[k] = L"'LSHIFT'";
			CommandBind( szID, paramsSet, pContext );
			paramsSet[k] = L"'RSHIFT'";
			CommandBind( szID, paramsSet, pContext );
			return;
		}
	}
	CommandBindInner( szID, paramsSet );
}

static void CommandUnbind( const string &szID, const vector<wstring> &paramsSet, void *pContext )
{
	if ( paramsSet.size() < 1 )
	{
		csSystem << "usage: " << szID << "'bind-id'" << endl;
		return;
	}

	NInput::Unbind( NStr::ToMBCS( paramsSet.front() ) );
}

static void CommandUnbindAll( const string &szID, const vector<wstring> &paramsSet, void *pContext )
{
	NInput::UnbindAll();
}

static void CommandBindSection( const string &szID, const vector<wstring> &paramsSet, void *pContext )
{
	string szSection;

	if ( paramsSet.size() >= 1 )
		szSection = NStr::ToMBCS( paramsSet.front() );

	NInput::SetSection( szSection, false );
}

static void CommandBindConfigure( const string &szID, const vector<wstring> &paramsSet, void *pContext )
{
	if ( paramsSet.size() < 2 )
	{
		csSystem << "usage: " << szID << "'bind-id' #coef" << endl;
		return;
	}

	NInput::SetControlCoeff( NStr::ToMBCS( paramsSet[0] ), _wtof( paramsSet[1].c_str() ) );
}

static void CommandBindUpdate( const string &szID, const vector<wstring> &paramsSet, void *pContext )
{
	NInput::UpdateBinds();
}

static void CommandShowBind( const string &szID, const vector<wstring> &paramsSet, void *pContext )
{
	if ( paramsSet.size() < 1 )
	{
		csSystem << "usage: " << szID << "'bind-id'" << endl;
		return;
	}

	list<NInput::SBind> bindsList;
	NInput::GetBind( NStr::ToMBCS( paramsSet.front() ), &bindsList );

	csSystem << "Bind '" << paramsSet.front() << "' :" << endl;
	for ( list<NInput::SBind>::const_iterator iTemp = bindsList.begin(); iTemp != bindsList.end(); iTemp++ )
	{
		for ( int nTemp = 0; nTemp < iTemp->controlsSet.size(); nTemp++ )
			csSystem << iTemp->controlsSet[nTemp] << " ";
		csSystem << endl;
	}
}

START_REGISTER(InputBind)
	REGISTER_CMD( "bind", CommandBind )
	REGISTER_CMD( "unbind", CommandUnbind )
	REGISTER_CMD( "unbindall", CommandUnbindAll )
	REGISTER_CMD( "bindsection", CommandBindSection )
	REGISTER_CMD( "bindconfigure", CommandBindConfigure )
	REGISTER_CMD( "bind_update", CommandBindUpdate )
	REGISTER_CMD( "showbind", CommandShowBind )
FINISH_REGISTER


