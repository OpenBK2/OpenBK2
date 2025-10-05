#pragma once

#include "GPixelFormat.h"

namespace NGfx
{
	struct SPixel8888;
	class CTexture;
}

namespace NGScene
{
	class IRender;
	struct IGScene;
	class CLightmapsHolder;

	class CLightmapsLoader: public CObjectBase
	{
		OBJECT_NOCOPY_METHODS( CLightmapsLoader );
		ZDATA
		std::string szName;
		int   nUseCount;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&szName); f.Add(3,&nUseCount); return 0; }
		
		CObj<NGScene::CLightmapsHolder> pLM;
	public:
		const CArray2D< NGfx::SPixel8888 > &GetTexture(int UID);
		CLightmapsLoader(){};
		CLightmapsHolder *GetHolder();
		CLightmapsLoader(const std::string &_szName):szName(_szName){};
		void ReleaseHint();

	};

};
