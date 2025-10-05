#pragma once
#include "GPixelFormat.h"

namespace NGScene
{

struct SUserRTInfo
{
	struct STex
	{
		int nResolution;
		std::string szName;
		int nPixelFormatID;
		
		STex() {}
		STex( int _nResolution, const std::string &_szName, int _nPixelFormatID )
			: nResolution(_nResolution), szName(_szName), nPixelFormatID(_nPixelFormatID) {}
	};
	std::vector<STex> tex, cubeTex;

	void AddTex( int _nResolution, const std::string &_szName, int nFormatID = NGfx::SPixel8888::ID )
	{ 
		tex.push_back( STex( _nResolution, _szName, nFormatID ) ); 
	}
	void AddCubeTex( int _nResolution, const std::string &_szName, int nFormatID = NGfx::SPixel8888::ID )
	{ 
		cubeTex.push_back( STex( _nResolution, _szName, nFormatID ) ); 
	}
};

}

