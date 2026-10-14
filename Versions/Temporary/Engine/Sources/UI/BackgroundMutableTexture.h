#pragma once

#include "3Dmotor/GfxBuffers.h"
#include "Background.h"
#include "Misc/2Darray.h"
#include "System/Dg.h"

#include "UI_export.h"

// Can be modified during runtime
// used to display pictures (i.e. screenshots)
// is NOT initialised by InitByDesc() and SetTexture()
class UI_EXPORT CBackgroundMutableTexture : public CBackground
{
	OBJECT_BASIC_METHODS( CBackgroundMutableTexture );

public:
	class CTextureData : public CPtrFuncBase<NGfx::CTexture>
	{
		OBJECT_NOCOPY_METHODS( CTextureData );

		// Defaults cover construction before Init/load; explicit constructor values still take precedence.
		bool bNeedUpdate = {};

	protected:
		void Recalc() override;

		bool NeedUpdate() override
		{
			return bNeedUpdate;
		}

	public:
		CArray2D<NGfx::SPixel8888> picture;

		int operator&( CStructureSaver &f ) override
		{
			f.Add( 1, &picture );
			return 0;
		}

		void Set( const CArray2D<NGfx::SPixel8888> &src );
	};

private:
	CObj<CTextureData> pData;

public:
	CBackgroundMutableTexture()
	{
		pData = new CTextureData;
	}

	void Visit( struct IUIVisitor *pVisitor ) override;
	int operator&( IBinSaver &ss ) override;
	void InitByDesc( const struct NDb::SUIDesc *pDesc ) override;

	virtual void Clear()
	{
		pData->picture.Clear();
	}

	void SetTexture( const struct NDb::STexture *_pDesc ) override
	{
		ASSERT( !_pDesc );
	}

	void Set( const CArray2D<NGfx::SPixel8888> &src );
};
