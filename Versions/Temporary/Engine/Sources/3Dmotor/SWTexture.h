#pragma once
#include "System/GResource.h"
#include "GPixelFormat.h"
#include "Misc/2Darray.h"

namespace NGfx
{
	class CTexture;
}

namespace NDb
{
	struct STexture;
}

namespace NGScene
{

struct SBumpPixel
{
	float fDU, fDV;
};

class CSWTextureData: public CObjectBase
{
	OBJECT_BASIC_METHODS(CSWTextureData);
public:
	std::vector<CArray2D<NGfx::SPixel8888> > mips;
	std::vector<CArray2D<SBumpPixel> > bumpMips;

	void PrepareBump();
	int GetSizeX() const { return mips[0].GetSizeX(); }
	int GetSizeY() const { return mips[0].GetSizeY(); }
};

class CSWTexture : public CResourceLoader<CDBPtr<NDb::STexture>, CSWTextureData>
{
	OBJECT_BASIC_METHODS(CSWTexture);
	CObj<CFileRequest> pRequest;
	bool bIsReady;
	void LoadTexture();
protected:
	virtual void Recalc();
public:
	CSWTexture() : bIsReady(false) {}
	bool IsReady();
};

class CBilinearTexture: public CPtrFuncBase<CSWTextureData>
{
	OBJECT_BASIC_METHODS(CBilinearTexture);
	ZDATA
	CArray2D<NGfx::SPixel8888> pic;
	int nXSize, nYSize;
	ZEND int operator&( CStructureSaver &f ) { f.Add(2,&pic); f.Add(3,&nXSize); f.Add(4,&nYSize); return 0; }
protected:
	virtual void Recalc();
public:
	CBilinearTexture() {}
	CBilinearTexture( const CArray2D<NGfx::SPixel8888> &_data, int _nXSize, int _nYSize )
		: pic(_data), nXSize(_nXSize), nYSize(_nYSize) {}
};

} // namespace
