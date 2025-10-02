#pragma once

#include "UI_export.h"

#include "background.h"
#include "../Misc/2Darray.h"
#include "../3DMotor/GFXBuffers.h"
#include "../System/DG.h"


// Can be modified during runtime
// used to display pictures (i.e. screenshots)
// is NOT initialised by InitByDesc() and SetTexture()
class UI_EXPORT CBackgroundMutableTexture : public CBackground
{
	OBJECT_BASIC_METHODS(CBackgroundMutableTexture);
	
public:
	class CTextureData: public CPtrFuncBase<NGfx::CTexture>
	{
		OBJECT_NOCOPY_METHODS(CTextureData);

		bool bNeedUpdate;
	protected:
		void Recalc();
		bool NeedUpdate() { return bNeedUpdate; }
	public:
		CArray2D<NGfx::SPixel8888> picture;

		int operator&( CStructureSaver &f ) { f.Add( 1, &picture); return 0; }
		void Set( const CArray2D<NGfx::SPixel8888> &src );
	};
private:
	CObj<CTextureData> pData;

public:
	CBackgroundMutableTexture() 
	{
		pData = new CTextureData;
	}
	virtual void Visit( struct IUIVisitor * pVisitor );
	virtual int operator&( IBinSaver &ss );
	virtual void InitByDesc( const struct NDb::SUIDesc *pDesc );
	virtual void Clear() { pData->picture.Clear(); }
	virtual void SetTexture( const struct NDb::STexture *_pDesc ) { ASSERT(!_pDesc); }
	void Set( const CArray2D<NGfx::SPixel8888> &src );
};


