#pragma once

#include "Input.h"

#include "Input_export.h"

#include <cstdint>

namespace NInput
{
	struct SCommand;
}

struct SGameMessage
{
	NInput::SMessage mMessage;
	std::vector<NInput::SCommand*> commands;
#ifndef _FINALRELEASE
	std::string szEventName;
#endif
	// general-purpose params
	struct  
	{
		int nParam1;//union { int nParam1; uint32_t dwParam1; float fParam1; };
		int nParam2; //union { int nParam2; uint32_t dwParam2; float fParam2; };
	};
	SGameMessage() : nParam1(0), nParam2(0) {}
};

namespace NInput
{

enum EMappingType
{
	MTYPE_EVENT,
	MTYPE_EVENT_UP,
	MTYPE_SLIDER,
	MTYPE_SLIDER_MINUS,
	MTYPE_UNKNOWN
};

struct SBind
{
	std::string szSection;
	EMappingType eType;
	std::vector<std::string> controlsSet;
};
class INPUT_EXPORT CBind
{
	float fDelta;
	SCommand* pBindCommand;
#ifndef _FINALRELEASE
	std::string szBindName;
#endif
public:
	CBind( const std::string &sCmd );

	bool IsActive() const;
	float GetDelta();
	float GetSpeed() const;

	bool ProcessEvent( const SGameMessage &eEvent );
};

const std::vector<std::string>& GetSections();
INPUT_EXPORT void SetSection( const std::string &_szSection, bool bUpdate = true );
INPUT_EXPORT void SetSection( const std::vector<std::string> &sections, bool bUpdate = true );

void Bind( const std::string &szCmd, const SBind &sCmdBind );
void Unbind( const std::string &szCmd );
INPUT_EXPORT void GetBind( const std::string &szCmd, std::list<SBind> *pRes );
void UpdateBinds();

float GetControlCoeff( const std::string &szControl );
void SetControlCoeff( const std::string &szControl, float fCoeff );

float GetCommandCoeff( const std::string &szControl );
void SetCommandCoeff( const std::string &szControl, float fCoeff );

INPUT_EXPORT bool GetEvent( SGameMessage *pGameMessage );
void MakeEvent( SGameMessage *pMSG,  const std::string &szGameMessage, int nParam1, int nParam2, EControlType ct );
INPUT_EXPORT void PostEvent( const std::string &szGameMessage, int nParam1, int nParam2 );
INPUT_EXPORT void PostWinEvent( const std::string &szGameMessage, int nParam1, int nParam2 );
void PurgeEvents();
INPUT_EXPORT void PurgeUIEvents();

};


