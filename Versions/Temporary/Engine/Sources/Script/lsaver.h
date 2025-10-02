#pragma once

#include "lstate.h"

typedef string CLuaFuncID; 
extern lua_State *pLUASaverState;

void lua_RegisterFunc( lua_CFunction func, const CLuaFuncID& id );
void lua_RegisterFunc( lua_Hook func, const CLuaFuncID& id );

int lua_AddCFunc( lua_CFunction *pFunc, IBinSaver &f, int nChunk );
int lua_AddHook( lua_Hook *pHook, IBinSaver &f, int nChunk );
void lua_AddString( IBinSaver &f, IBinSaver::chunk_id idChunk, TString **ppszStr, int nChunk =	1 );

void lua_StartSerialize( lua_State *pL ); 


