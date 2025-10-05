#pragma once
#include "GObjectInfo.h"

struct granny_file;
struct granny_file_info;
struct granny_animation;

namespace NAnimation
{

class CGrannyFileInfo : public CObjectBase
{
	OBJECT_NOCOPY_METHODS(CGrannyFileInfo);
	granny_file *pFile;
	granny_file_info *pData;
public: 
	CGrannyFileInfo() : pData(0), pFile(0) {}
	CGrannyFileInfo( granny_file *_pFile ):pFile(_pFile)
	{
		InitMe();
	}
	void InitMe();
	granny_file_info* GetData() { return pData; }
};

class CGrannyBaseStuffLoader : public CHoldedPtrFuncBase<CGrannyFileInfo>
{
	typedef CHoldedPtrFuncBase<CGrannyFileInfo> TParent;
	OBJECT_BASIC_METHODS(CGrannyBaseStuffLoader);
	ZDATA
	CDGPtr<NGScene::CGrannyMemFileLoader> pGrannyFile;
	std::string szResName;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pGrannyFile); f.Add(3,&szResName); return 0; }

	void TrueSetKey( const NGScene::SResKey<NGScene::SGrannyFileLoaderInfo> &key );
protected:
	CGrannyBaseStuffLoader() {}
	bool NeedUpdate() { TParent::NeedUpdate(); return pGrannyFile.Refresh(); }
	void Recalc();
	virtual bool IsDelayedLoad() const { return true; }
public:
	CGrannyBaseStuffLoader( const std::string &_szResName ) : szResName(_szResName) {}

template<class TResource>
	void SetKey( const CDBPtr<TResource> &pResource )
	{
		if ( !pResource )
			return;
		bool bDelayedLoad = IsDelayedLoad();
		NGScene::SResKey<NGScene::SGrannyFileLoaderInfo> uidKey( pResource->uid, NGScene::SGrannyFileLoaderInfo( szResName, pResource->GetRecordID(), bDelayedLoad ) );
		TrueSetKey( uidKey );
	}
};

class CGrannyAnimationLoader : public CGrannyBaseStuffLoader
{
	OBJECT_BASIC_METHODS(CGrannyAnimationLoader);
	virtual bool IsDelayedLoad() const { return false; }
public:
	CGrannyAnimationLoader() : CGrannyBaseStuffLoader( "Animations" ) {}
};

class CGrannyAIGeomLoader : public CGrannyBaseStuffLoader
{
	OBJECT_BASIC_METHODS(CGrannyAIGeomLoader);
	virtual bool IsDelayedLoad() const { return false; }
public:
	CGrannyAIGeomLoader() : CGrannyBaseStuffLoader( "AIGeometries" ) {}
};

class CGrannySkeletonLoader : public CGrannyBaseStuffLoader
{
	OBJECT_BASIC_METHODS(CGrannySkeletonLoader);
	virtual bool IsDelayedLoad() const { return false; }
public:
	CGrannySkeletonLoader() : CGrannyBaseStuffLoader( "Skeletons" ) {}
};

} // namespace NAnimation


