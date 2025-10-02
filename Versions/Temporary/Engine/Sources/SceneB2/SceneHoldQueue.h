#pragma once


void SetToSceneHoldQueue( CObjectBase *p, bool bSerialize = false );
void ClearSceneHoldQueue();
void StepSceneHoldQueue( const NTimer::STime timeCurrTime );
void SerializeSceneHoldQueue( IBinSaver::chunk_id chunkID, IBinSaver &saver );
void GetSceneHoldedObjects( list<CObjectBase*> *pObjects, bool bSerializeable );

