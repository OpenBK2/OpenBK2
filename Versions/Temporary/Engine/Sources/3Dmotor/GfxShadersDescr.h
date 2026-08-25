#pragma once
#include <d3d9.h>

#include <cstdint>

// should be global variable
struct SVShader
{
	int nID;
	DWORD*pShader11, *pShader20;
	
	SVShader( int _nID, DWORD*_pShader11, DWORD*_pShader20 ): nID(_nID), pShader11(_pShader11), pShader20(_pShader20) {}
};

// should be global variable
struct SRenderState
{
	D3DRENDERSTATETYPE state;
	uint32_t dwVal;
};
struct STextureStageState
{
	int nStage;
	D3DTEXTURESTAGESTATETYPE state;
	uint32_t dwVal;
};
struct SPShader
{
	int nID;
	DWORD *pShader11, *pShader14, *pShader20, *pShader20a;
	SRenderState *pShaRS;
	STextureStageState *pShaTSS;
	SRenderState *pStateRS;
	STextureStageState *pStateTSS;
	
	SPShader( 
		int _nID, DWORD*_pShader11, DWORD*_pShader14, DWORD*_pShader20, DWORD*_pShader20a,
		SRenderState *_pShaRS, STextureStageState *_pShaTSS, 
		SRenderState *_pStateRS, STextureStageState *_pStateTSS )
		:nID(_nID), pShader11(_pShader11), pShader14(_pShader14), pShader20(_pShader20), pShader20a(_pShader20a),
		pShaRS(_pShaRS), pShaTSS(_pShaTSS), 
		pStateRS(_pStateRS), pStateTSS(_pStateTSS) {}
};

struct SHLSLShader
{
	int nVSShaderID, nPSShaderID;
	//int nID;
	//uint32_t *pVShader11, *pVShader20, *pPShader11, *pPShader14, *pPShader20, *pPShader20a;
	
	//SHLSLShader( int _nID,
	//	uint32_t *_pVShader11, uint32_t *_pVShader20, uint32_t *_pPShader11, uint32_t *_pPShader14, uint32_t *_pPShader20, uint32_t *_pPShader20a )
	//	: nID(_nID), pVShader11(_pVShader11), pVShader20(_pVShader20), pPShader11(_pPShader11), pPShader14(_pPShader14),
	//	pPShader20(_pPShader20), pPShader20a(_pPShader20a) {}
};


