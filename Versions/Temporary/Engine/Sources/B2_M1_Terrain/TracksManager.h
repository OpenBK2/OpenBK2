#pragma once

#include "B2_M1_Terrain_export.h"


#include "3Dmotor/GView.h"
#include "PatchHolder.h"

class CTracksManager;
class CCSBound;

class CTrackObjInfo : public CPtrFuncBase<NGScene::CObjectInfo>
{
  OBJECT_NOCOPY_METHODS( CTrackObjInfo );
  //
  CDGPtr< CFuncBase<STime> > pTimer;
  CPtr<CTracksManager> pTrackManager;
  int nID;
  NGScene::CObjectInfo::SData data;
  std::vector<NMeshData::SMeshData> tracksBuf;
  std::vector<int> startTimes;
  int nStartInd, nFinalInd;
  float fFadingSpeed;
  bool bNeedUpdate;
  bool bFirstUpdate;
protected:
  void Recalc();
  bool NeedUpdate() { if ( bNeedUpdate ) return pTimer.Refresh(); pTimer.Refresh(); return false; }
  void InitData();
  //
  CTrackObjInfo() {}
public:
  CTrackObjInfo( CFuncBase<STime> *_pTimer, CTracksManager *_pTrackManager, const int _nID );
  void UpdateNode( const NMeshData::SMeshData &meshData );
  void SetFadingSpeed( const float _fFadingSpeed ) { fFadingSpeed = _fFadingSpeed; }
  void AttachTimer( CFuncBase<STime> *_pTimer ) { pTimer = _pTimer; }
  //
	int operator&( IBinSaver &saver )
	{
		saver.Add( 1, &pTrackManager );
		saver.Add( 2, &nID );
		saver.Add( 3, &data );
		saver.Add( 4, &tracksBuf );
		saver.Add( 5, &startTimes );
		saver.Add( 6, &nStartInd );
		saver.Add( 7, &nFinalInd );
		saver.Add( 8, &fFadingSpeed );
		saver.Add( 9, &bNeedUpdate );
		saver.Add( 10, &bFirstUpdate );
		return 0;
	}
};

typedef NMeshData::SPatchHolder<CTrackObjInfo> CTrackObjHolder;
struct STrackObj
{
  CVec3 vMin, vMax;
  CObj<CCSBound> pBound;
  CTrackObjHolder trackHolder;
	int operator&( IBinSaver &saver )
	{
		saver.Add( 1, &vMin );
		saver.Add( 2, &vMax );
		saver.Add( 3, &pBound );
		saver.Add( 4, &trackHolder.pPatch );
		return 0;
	}
};

class B2_M1_TERRAIN_EXPORT CTracksManager : public CObjectBase
{
  OBJECT_NOCOPY_METHODS( CTracksManager );
  //
  CDGPtr< CFuncBase<STime> > pTimer;
  CObj<NGScene::IGameView> pGScene;
  //
  std::vector<int> freeTracks;
  std::vector<STrackObj> tracks;
  std::vector<int> delQueue;
  std::vector<int> updatedTracks;
  //
  typedef det_map<int, int> CTracksMap;
  CTracksMap tracksMap;
	//
	CDBPtr<NDb::SMaterial> pTracksMaterial;
	//
  CTracksManager() {}
public:
  CTracksManager( CFuncBase<STime> *_pTimer, NGScene::IGameView *_pGScene );
	//
	void SetTracksMaterial( const NDb::SMaterial *_pTracksMaterial ) { pTracksMaterial = _pTracksMaterial; }
	//
  void AddTrack( const int nID, const float fFadingSpeed, const NMeshData::SMeshData &data,
                 const CVec3 &vMin, const CVec3 &vMax );
  void FreeTrackBuffer( const int nFreeID );
  void ProcessDelQueue();
  //
  void AfterLoad( CFuncBase<STime> *_pTimer, NGScene::IGameView *_pGScene );
  //
	int operator&( IBinSaver &saver )
	{
		saver.Add( 1, &freeTracks );
		saver.Add( 2, &tracks );
		saver.Add( 3, &delQueue );
		saver.Add( 4, &tracksMap );
    if ( !( saver.IsReading() ) )
    {
      updatedTracks.resize( 0 );
      for ( int i = 0; i < tracks.size(); ++i )
      {
        if ( tracks[i].trackHolder.pHolder )
          updatedTracks.push_back( i );
      }
    }
    saver.Add( 5, &updatedTracks );
    saver.Add( 6, &pTracksMaterial );
		return 0;
	}
};


