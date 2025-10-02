#include "StdAfx.h"
#include "CommonEditorMethods.h"

#include "../libdb/Manipulator.h"
#include "Interface_CommandHandler.h"

bool SetGetEditParameters( DWORD pEditParameters, UINT nCommandHandlerType, int nCmdID )
{
	NI_ASSERT( pEditParameters != 0, "CObjectState::GetEditParameters(): pEditParameters == 0" );
	return Singleton<ICommandHandlerContainer>()->HandleCommand( nCommandHandlerType, nCmdID, pEditParameters );
}


void CreateRefKey( string *pszKey, const SPropertyDesc *pPropertyDesc )
{
	if ( ( pszKey != 0 ) && ( pPropertyDesc != 0 ) )
	{
		list<string> refTypeList;
		for ( SPropertyDesc::CTypesMap::const_iterator itRefType = pPropertyDesc->refTypes.begin(); itRefType != pPropertyDesc->refTypes.end(); ++itRefType )
		{
			refTypeList.push_back( itRefType->first );
		}
		refTypeList.sort();
		pszKey->clear();
		for ( list<string>::const_iterator itRefType = refTypeList.begin(); itRefType != refTypeList.end(); ++itRefType )
		{
			if ( pszKey->empty() )
			{
				( *pszKey ) = ( *itRefType );
			}
			else
			{
				( *pszKey ) += StrFmt( "%c%s", TYPE_SEPARATOR_CHAR, ( *itRefType ) );
			}
		}
	}
}


