#pragma once

struct IGlobeScriptHandler : public CObjectBase
{
	virtual const int CallGlobeScriptFunction( const std::string &szFuncCall ) = 0;
};


