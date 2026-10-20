#pragma once

// Defensive member defaults; explicit constructor values still take precedence.
#include "System_export.h"

namespace NGlobal
{

// The global var set value handlers, which store the value through pContext in
// bool/int/float form, now live in GlobalVars.h as VarBoolHandlerFn and the
// rest, next to the VarBoolHandler tags that registrations name and to the
// RegisterVar overload that checks one against the other. They moved there
// because this header is included after that one and could not have been the
// place the check is written.

// helper class for temporary command registration
class CCmd
{
	void *pContext = nullptr;
	std::string szID;
	CmdHandler pHandler = nullptr;
	int nID = 0;

public:
	CCmd( const std::string &szID, CmdHandler pHandler, void *pContext );
	~CCmd();

	void Run( const std::vector<std::wstring> &paramsSet );
};

} // namespace NGlobal
