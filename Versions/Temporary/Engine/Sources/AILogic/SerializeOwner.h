#ifndef __SERIALIZE_OWNER_H__
#define __SERIALIZE_OWNER_H__

#pragma ONCE

template <class T>
void SerializeOwner( const int nChunk, T **pOwner, IBinSaver* pSaver )
{
	CPtr<T> pSaverUnit;
	if ( pSaver->IsReading() )
	{
		pSaver->Add( nChunk, &pSaverUnit );
		*pOwner = pSaverUnit;
	}
	else
	{
		pSaverUnit = *pOwner;
		pSaver->Add( nChunk, &pSaverUnit );
	}
}

#endif // __SERIALIZE_OWNER_H__
