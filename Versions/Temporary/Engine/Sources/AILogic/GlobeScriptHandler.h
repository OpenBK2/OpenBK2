#pragma once

struct IGlobeScriptHandler : public CObjectBase
{
	virtual const int CallGlobeScriptFunction( const string &szFuncCall ) = 0;
};


