#pragma once

#include "3Dmotor_export.h"

namespace NGfx
{
	class CTexture;
	class CCubeTexture;
	struct SRenderTargetsInfo;
}
namespace NGScene
{

class _3DMOTOR_EXPORT CRTPtr
{
	CObj<NGfx::CTexture> pRes;
	ZDATA
	std::string szName;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&szName); return 0; }
	CRTPtr() {}
	CRTPtr( const std::string &_szName ) : szName(_szName) {}
	void SetName( const std::string &_szName ) { szName = _szName; pRes = 0; }
	NGfx::CTexture *GetTexture();
};

class CCubeRTPtr
{
	CObj<NGfx::CCubeTexture> pRes;
	ZDATA
	std::string szName;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&szName); return 0; }
	CCubeRTPtr() {}
	CCubeRTPtr( const std::string &_szName ) : szName(_szName) {}
	void SetName( const std::string &_szName ) { szName = _szName; pRes = 0; }
	NGfx::CCubeTexture *GetTexture();
};

struct SUserRTInfo;
void InitRTShare( const SUserRTInfo &rtInfo, NGfx::SRenderTargetsInfo *pRes );
int GetDepthTexResolution();
const int N_DEFAULT_RT_RESOLUTION = 512;
}

