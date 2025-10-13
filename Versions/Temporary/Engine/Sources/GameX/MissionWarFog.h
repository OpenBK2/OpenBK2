#pragma once

#include <cstdint>

/*struct SInterfaceMissionWarFogInfo
{
	ZDATA
	CArray2D<float> warFog1;
	CArray2D<float> warFog2;
	CArray2D<float> baseNoise1;
	CArray2D<float> baseNoise2;
	NTimer::STime warFogTime1;
	NTimer::STime warFogTime2;
	NTimer::STime baseNoiseTime1;
	NTimer::STime baseNoiseTime2;

	int nSizeX;
	int nSizeY;
	float fScale;
	bool bIsWarFog;
	bool bIsNoise;
	bool bIsWarFogOn;
	bool bIsNoiseOn;
	CVec2 vNoisePos;
	NTimer::STime noiseTime;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&warFog1); f.Add(3,&warFog2); f.Add(4,&baseNoise1); f.Add(5,&baseNoise2); f.Add(6,&warFogTime1); f.Add(7,&warFogTime2); f.Add(8,&baseNoiseTime1); f.Add(9,&baseNoiseTime2); f.Add(10,&nSizeX); f.Add(11,&nSizeY); f.Add(12,&fScale); f.Add(13,&bIsWarFog); f.Add(14,&bIsNoise); f.Add(15,&bIsWarFogOn); f.Add(16,&bIsNoiseOn); f.Add(17,&vNoisePos); f.Add(18,&noiseTime); return 0; }

	float cloudDensities[256];

	void Reset( const int nSizeX, const int nSizeY, const float fScale );
	void SetWarFog( const CArray2D<uint8_t> &warFog, float fScale, NTimer::STime nGameTime );
	void Update( NTimer::STime nGameTime );
	//
	SInterfaceMissionWarFogInfo();
private:	
	void GetWarFog( CArray2D<float> *pWarFog, NTimer::STime time );
	// возвращает переходный шум между опорными, генерирует опорные шумы при необходимости
	void GetNoise( CArray2D<float> *pNoise, NTimer::STime time );
	void Blend( CArray2D<float> *pDst, const CArray2D<float> &src1, const CArray2D<float> &src2, float fDelta );
	void CycleMove( CArray2D<float> *pDst, const CArray2D<float> &src, const CVec2 &vPos );
	void SceneSetFarFog( const CArray2D<float> &src );
	void ApplyCloudDensity( CArray2D<float> *pNoise );
	void Blur( CArray2D<float> *pDst, const CArray2D<float> &src );
};*/


