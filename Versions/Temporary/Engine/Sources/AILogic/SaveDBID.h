#ifndef __SAVE_DBID_H__
#define __SAVE_DBID_H__

#pragma ONCE

/*
inline void SaveDBID( IBinSaver *pSaver, const int nChunk )
{
	if ( dbID >= 0 )
	{
		CDBPtr<SGDBObjectDesc> pDesc = Singleton<IObjectsDB>()->GetDesc( dbID );
		NI_VERIFY( pDesc != 0, StrFmt( "Can't find DB description with index %d", dbID ), return );
		
		pSaver->Add( nChunk, &pDesc );
	}
}

inline void LoadDBID( IBinSaver *pSaver, const int nChunk, int *pDBID )
{
	CDBPtr<SGDBObjectDesc> pDesc;
	pSaver->Add( nChunk, &pDesc );

	*pDBID = Singleton<IObjectsDB>()->GetIndex( pDesc );
}
*/

#endif // __SAVE_DBID_H__

