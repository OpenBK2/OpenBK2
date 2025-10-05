#include "stdafx.h"
#include "TracksManager.h"

#define DEF_MAX_TRACK_BUFFERS 128
#define DEF_MAX_TRACKS_IN_BUFFER 32
#define DEF_MAX_VERTS_PER_TRACK 8
#define DEF_MAX_TRGS_PER_TRACK 6

DEFINE_DG_CONSTANT_NODE( CCSBound, SBound );

inline DWORD PackAndDupBYTE2DWORD( const BYTE elem )
{
  return ( (DWORD)elem << 24 ) + ( (DWORD)elem << 16 ) + ( (DWORD)elem << 8 ) + elem;
}

CTrackObjInfo::CTrackObjInfo( CFuncBase<STime> *_pTimer, CTracksManager *_pTrackManager, const int _nID )
: pTimer( _pTimer ), pTrackManager( _pTrackManager ), nID( _nID )
{
  tracksBuf.resize( DEF_MAX_TRACKS_IN_BUFFER );
  for ( std::vector<NMeshData::SMeshData>::iterator it = tracksBuf.begin(); it != tracksBuf.end(); ++it )
  {
    it->vertices.reserve( DEF_MAX_VERTS_PER_TRACK );
    it->vertices.resize( 0 );
    it->triangles.reserve( DEF_MAX_TRGS_PER_TRACK );
    it->triangles.resize( 0 );
  }

  InitData();

  startTimes.resize( DEF_MAX_TRACKS_IN_BUFFER );
  fill( startTimes.begin(), startTimes.end(), 0 );
  nStartInd = 0;
  nFinalInd = 0;
  fFadingSpeed = 0.0f;
  bNeedUpdate = false;
  bFirstUpdate = true;
}

void CTrackObjInfo::InitData()
{
  data.verts.reserve( DEF_MAX_TRACKS_IN_BUFFER * DEF_MAX_VERTS_PER_TRACK );
  data.verts.resize( 0 );
	data.geometry.resize( DEF_MAX_TRACKS_IN_BUFFER * DEF_MAX_TRGS_PER_TRACK );
	data.geometry.resize( 0 );
}

void CTrackObjInfo::Recalc()
{
  if ( pValue == 0 )
    pValue = new NGScene::CObjectInfo();
  if ( pTimer ) 
  {
    data.verts.resize( 0 );
		data.geometry.resize(0);
    std::vector<NMeshData::SMeshData>::const_iterator itCur = tracksBuf.begin() + nStartInd;
    std::vector<NMeshData::SMeshData>::const_iterator itFinal = tracksBuf.begin() + nFinalInd;
    const int nCurTime = pTimer->GetValue();
    bool bDelFlag;
    int nTrackCount = nStartInd;
    while ( itCur != itFinal )
    {
      bDelFlag = false;
      const int nVertsOffs = data.verts.size();
			const int nFadingCoeff = Float2Int( (nCurTime - startTimes[nTrackCount]) * fFadingSpeed );
      for ( std::vector<NGScene::SVertex>::const_iterator it = itCur->vertices.begin(); it != itCur->vertices.end(); ++it )
      {
				data.verts.push_back( *it );
        const int nCol = ( data.verts.back().texU.dw & 0xff ) + nFadingCoeff;

        if ( nCol > 0xff )
        {
          bDelFlag = true;
          data.verts.back().texU.dw = 0xffffffff;
        }
        else
          data.verts.back().texU.dw = PackAndDupBYTE2DWORD( nCol );
      }

			for ( std::vector<STriangle>::const_iterator it = itCur->triangles.begin(); it != itCur->triangles.end(); ++it )
				data.geometry.push_back( STriangle( nVertsOffs + it->i1, nVertsOffs + it->i2, nVertsOffs + it->i3 ) );

			if ( bDelFlag )
      {
        if ( ++nStartInd >= DEF_MAX_TRACKS_IN_BUFFER )
          nStartInd = 0;
        if ( nStartInd == nFinalInd )
        {
          pTrackManager->FreeTrackBuffer( nID );
          bNeedUpdate = false;
          return;
        }
      }
      ++nTrackCount;
      ++itCur;
      if ( itCur == tracksBuf.end() )
      {
        itCur = tracksBuf.begin();
        nTrackCount = 0;
      }
    }
    pValue->AssignFast( &data );

    if ( bFirstUpdate )
    {
      InitData();
      bFirstUpdate = false;
    }
  }
}

void CTrackObjInfo::UpdateNode( const NMeshData::SMeshData &meshData )
{
  NI_ASSERT( meshData.vertices.size() <= DEF_MAX_VERTS_PER_TRACK, "Too many vertices in source geometry" );
  NI_ASSERT( meshData.triangles.size() <= DEF_MAX_TRGS_PER_TRACK, "Too many triangles in source geometry" );

  NMeshData::SMeshData &curMeshData = tracksBuf[nFinalInd];

  curMeshData.vertices = meshData.vertices;
  curMeshData.triangles = meshData.triangles;

	pTimer.Refresh();
  startTimes[nFinalInd] = pTimer->GetValue();

  if ( ++nFinalInd >= DEF_MAX_TRACKS_IN_BUFFER )
    nFinalInd = 0;

  if ( nFinalInd == nStartInd )
  {
    if ( ++nStartInd >= DEF_MAX_TRACKS_IN_BUFFER )
      nStartInd = 0;
  }
  bNeedUpdate = true;
}

CTracksManager::CTracksManager( CFuncBase<STime> *_pTimer, NGScene::IGameView *_pGScene ) : pTimer( _pTimer ), pGScene( _pGScene )
{
  tracks.resize( DEF_MAX_TRACK_BUFFERS );
  freeTracks.reserve( tracks.size() );
  freeTracks.resize( 0 );
  SBound bound;
  bound.BoxInit( CVec3(0, 0, 0), CVec3(0, 0, 0) );
  int nCount = 0;
  NGScene::SFullRoomInfo room( NGScene::SRoomInfo( NGScene::LF_SKIP_LIGHTING, -100 ), 0, 0 );
  for ( std::vector<STrackObj>::iterator it = tracks.begin(); it != tracks.end(); ++it )
  {
    it->vMin.Set(FP_MAX_VALUE, FP_MAX_VALUE, FP_MAX_VALUE);
    it->vMax.Set(-FP_MAX_VALUE, -FP_MAX_VALUE, -FP_MAX_VALUE);
    it->pBound = new CCSBound();
    it->pBound->Set( bound );
    it->trackHolder.pPatch = new CTrackObjInfo( pTimer, this, nCount );
    freeTracks.push_back( nCount );
    ++nCount;
  }
  delQueue.reserve( tracks.size() );
  delQueue.resize( 0 );
  tracksMap.clear();
}

void CTracksManager::AddTrack( const int nID, const float fFadingSpeed, const NMeshData::SMeshData &data,
                              const CVec3 &vMin, const CVec3 &vMax )
{
  int nTrackID = -1;
  CTracksMap::const_iterator pos = tracksMap.find( nID );
  if ( pos == tracksMap.end() ) 
  {
    // assign new free track ID for it
    if ( !freeTracks.empty() )
    {
      nTrackID = freeTracks.back();
      tracksMap[nID] = nTrackID;
      freeTracks.resize( freeTracks.size() - 1 );
      //
      tracks[nTrackID].vMin.Set(FP_MAX_VALUE, FP_MAX_VALUE, FP_MAX_VALUE);
      tracks[nTrackID].vMax.Set(-FP_MAX_VALUE, -FP_MAX_VALUE, -FP_MAX_VALUE);
      tracks[nTrackID].trackHolder.pPatch->SetFadingSpeed( fFadingSpeed );
      NGScene::SFullRoomInfo room( NGScene::SRoomInfo( NGScene::LF_SKIP_LIGHTING, -100 ), 0, 0 );
			//
//			NI_ASSERT( pTracksMaterial, "Tracks material are empty" );
			if ( !pTracksMaterial )
				return;
			//
      tracks[nTrackID].trackHolder.pHolder = pGScene->CreateDynamicMesh( 
				pGScene->MakeMeshInfo( tracks[nTrackID].trackHolder.pPatch, pTracksMaterial ), 
				0, tracks[nTrackID].pBound, NGScene::MakeLargeHintBound(), room );
    }
  }
  else
  {
    nTrackID = pos->second;
  }
  // use nTrackID here
  if ( nTrackID != -1 ) 
  {
    STrackObj &trackObj = tracks[nTrackID];
    trackObj.vMin.Minimize( vMin );
    trackObj.vMax.Maximize( vMax );
    SBound bound;
    bound.BoxInit( trackObj.vMin, trackObj.vMax );
    trackObj.pBound->Set( bound );
    trackObj.trackHolder.pPatch->UpdateNode( data );
  }
}

void CTracksManager::FreeTrackBuffer( const int nFreeID )
{
  freeTracks.push_back( nFreeID );
  delQueue.push_back( nFreeID );
  for ( CTracksMap::iterator it = tracksMap.begin(); it != tracksMap.end(); ++it )
  {
    if ( it->second == nFreeID ) 
    {
      tracksMap.erase( it );
      break;
    }
  }
}

void CTracksManager::ProcessDelQueue()
{
  for ( std::vector<int>::const_iterator it = delQueue.begin(); it != delQueue.end(); ++it )
  {
    tracks[*it].trackHolder.pHolder = 0;
  }
  delQueue.resize( 0 );
}

void CTracksManager::AfterLoad( CFuncBase<STime> *_pTimer, NGScene::IGameView *_pGScene )
{
  pGScene = _pGScene;
  pTimer = _pTimer;
  for ( std::vector<STrackObj>::iterator it = tracks.begin(); it != tracks.end(); ++it )
    it->trackHolder.pPatch->AttachTimer( pTimer );

  NGScene::SFullRoomInfo room( NGScene::SRoomInfo( NGScene::LF_SKIP_LIGHTING, -100 ), 0, 0 );
  for ( std::vector<int>::const_iterator it = updatedTracks.begin(); it != updatedTracks.end(); ++it )
  {
    tracks[*it].trackHolder.pHolder = pGScene->CreateDynamicMesh( 
      pGScene->MakeMeshInfo( tracks[*it].trackHolder.pPatch, pTracksMaterial ), 0, tracks[*it].pBound, NGScene::MakeLargeHintBound(), room );
  }
}

REGISTER_SAVELOAD_CLASS( 0x130AB3C0, CTracksManager );
REGISTER_SAVELOAD_CLASS( 0x110B8C00, CTrackObjInfo );

