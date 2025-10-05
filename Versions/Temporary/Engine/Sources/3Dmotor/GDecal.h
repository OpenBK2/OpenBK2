#pragma once
#include "GDecalInfo.h"

namespace NGScene
{
class CDecalTarget;
struct SDecalTargetPart
{
	ZDATA
	CPtr<CObjectBase> pUser;
	int nUserID;
	ZEND int operator&( CStructureSaver &f ) { f.Add(2,&pUser); f.Add(3,&nUserID); return 0; }

	SDecalTargetPart() : nUserID(0) {}
	SDecalTargetPart( CObjectBase *_pUser, int _nUserID ) : pUser(_pUser), nUserID(_nUserID) {}
	bool operator==( const SDecalTargetPart &a ) const { return pUser == a.pUser && nUserID == a.nUserID; }
};
struct SDecalTargetPartHash
{
	int operator()( const SDecalTargetPart &p ) const { return (int)p.pUser.GetPtr() ^ p.nUserID;}
};
struct SSrcPosInfo
{
	ZDATA
	CPtr<CObjectBase> pUser;
	int nUserID;
	CPtr<CObjectBase> pSource;
	ZEND int operator&( CStructureSaver &f ) { f.Add(2,&pUser); f.Add(3,&nUserID); f.Add(4,&pSource); return 0; }

	SSrcPosInfo() : nUserID(0) {}
	SSrcPosInfo( CObjectBase *_pUser, int _nUserID, CObjectBase *_pSource ) : pUser(_pUser), nUserID(_nUserID), pSource(_pSource) {}
	bool operator==( const SSrcPosInfo &a ) const { return pUser == a.pUser && nUserID == a.nUserID && pSource == a.pSource; }
};
struct SSrcPosInfoHash
{
	int operator()( const SSrcPosInfo &p ) const { return (int)p.pUser.GetPtr() ^ p.nUserID ^ (int)p.pSource.GetPtr(); }
};

class ISomePart;
class CDecalTarget : public CObjectBase
{
	OBJECT_NOCOPY_METHODS(CDecalTarget);
public:
	typedef std::unordered_map<SSrcPosInfo, std::vector<CVec3>, SSrcPosInfoHash > CSrcPosHash;
	ZDATA
	SDecalMappingInfo mapInfo;
	std::vector<SDecalTargetPart> targetParts;
	std::vector<CPtr<ISomePart> > parts;
	CSrcPosHash srcPositions;
	ZEND int operator&( CStructureSaver &f ) { f.Add(2,&mapInfo); f.Add(3,&targetParts); f.Add(4,&parts); f.Add(5,&srcPositions); return 0; }
	CDecalTarget() {}
	CDecalTarget( const SDecalMappingInfo &_mapInfo ) : mapInfo(_mapInfo) {};
};

class CDecalsManager;
class IDecalQuery;
class IMaterial;
class CDecal : public CObjectBase
{
	OBJECT_NOCOPY_METHODS(CDecal);
	ZDATA
	CPtr<CDecalsManager> pOwner;
	CObj<CDecalTarget> pTarget;
	std::vector<CMObj<CObjectBase> > decals;
	CObj<IMaterial> pMaterial;
	ZEND int operator&( CStructureSaver &f ) { f.Add(2,&pOwner); f.Add(3,&pTarget); f.Add(4,&decals); f.Add(5,&pMaterial); return 0; }
public:
	CDecal() {}
	CDecal( CDecalsManager *_pOwner, CDecalTarget *_pTarget, IMaterial *_pMaterial );
	~CDecal();
	bool OnCreate( IDecalQuery *pScene, ISomePart *pNew, const SSrcPosInfo &tp );
	void Walk();
	std::vector<CMObj<CObjectBase> > &GetDecals() { return decals; }
};

typedef std::unordered_map<CPtr<CObjectBase>, bool, SPtrHash> CObjectBaseSet;
class IDecalQuery : virtual public CObjectBase
{
public:
	virtual CObjectBase* CreateDecal( ISomePart *pTarget, const std::vector<CVec3> &srcPositions, const SDecalMappingInfo &_info, IMaterial *pMaterial ) = 0;
	virtual void GetPartsList( const SDecalMappingInfo &_info, const CObjectBaseSet &targets, std::vector<CPtr<ISomePart> > *pRes ) = 0;
};

class CDecalsManager : public CObjectBase
{
	OBJECT_NOCOPY_METHODS(CDecalsManager);
	typedef std::unordered_map<SDecalTargetPart, std::vector<CPtr<CDecal> >, SDecalTargetPartHash> CPerUserHash;
	ZDATA
	CPtr<IDecalQuery> pScene;
	CPerUserHash decalsPerUser;
public:
	ZEND int operator&( CStructureSaver &f ) { f.Add(2,&pScene); f.Add(3,&decalsPerUser); return 0; }
public:
	CDecalsManager( IDecalQuery *_pScene = 0 ) : pScene(_pScene) {}
	IDecalQuery* GetScene() const { return pScene; }
	void OnCreate( ISomePart *pNew );
	// CreateDecal should be called right after CreateDecalTarget or some target parts may get removed
	// and information about them will be lost
	CDecalTarget* CreateDecalTarget( const std::vector<CObjectBase*> &targets, const SDecalMappingInfo &_info );
	CDecal* CreateDecal( CDecalTarget *pTarget, IMaterial *pMaterial );
	void Register( CDecal *pDecal, CDecalTarget *pTarget );
	void Unregister( CDecal *pDecal, CDecalTarget *pTarget );
	void Walk();
};
}


