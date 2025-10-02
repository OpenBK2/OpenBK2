#pragma once


// ************************************************************************************************************************ //
// **
// ** camera iterator to overview entire map in orthographic projection mode
// ** parameter FOV correspend to perspective camera FOV in desired view and identifies zoom
// **
// **
// ************************************************************************************************************************ //

class CCameraMapIterator
{
	int nScreenSizeX, nScreenSizeY;
	//
	CVec3 vStartAnchor;
	CVec3 vStepX, vStepY;
	CVec3 vCurrAnchor;
	//
	int nNumStepsX, nNumStepsY;
	int nCurrX, nCurrY;
	//
	void CalcCurrAnchor() { vCurrAnchor = vStartAnchor + vStepY*nCurrY + vStepX*nCurrX; }
public:
	CCameraMapIterator( float fFOV, float fYaw, float fPitch, float fDistance,
											const CVec2 &vScreenSize, float fMapSizeX, float fMapSizeY );
	//
	int GetImageSizeX() const { return nNumStepsX * nScreenSizeX; }
	int GetImageSizeY() const { return nNumStepsY * nScreenSizeY; }
	// iteration and iteration results
	bool Next();
	bool IsEnd() const { return nCurrY >= nNumStepsY; }
	const CVec3 &GetAnchor() const { return vCurrAnchor; }
	int GetImagePosX() const { return nCurrX * nScreenSizeX; }
	int GetImagePosY() const { return nCurrY * nScreenSizeY; }
};

