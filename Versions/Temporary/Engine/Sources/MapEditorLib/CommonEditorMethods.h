#if !defined(__COMMON_EDITOR_METHODS__)
#define __COMMON_EDITOR_METHODS__
#pragma once


bool SetGetEditParameters( DWORD pEditParameters, UINT nCommandHandlerType, int nCmdID );
void CreateRefKey( string *pszKey, const struct SPropertyDesc *pPropertyDesc );

#endif // !defined(__COMMON_EDITOR_METHODS__)

