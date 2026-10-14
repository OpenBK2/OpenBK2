#pragma once

#include "Window.h"

class CWindowProgressBar : public CWindow, public IProgressBar
{
	OBJECT_BASIC_METHODS( CWindowProgressBar )

	CObj<IWindowPart> pForward;  // show increasing progress
	CObj<IWindowPart> pBackward; // show decreasing part
	CObj<IWindowPart> pGlow;     // texture drawn over border between increasing and decreasing parts

	CPtr<NDb::SWindowProgressBar> pInstance;
	CDBPtr<NDb::SWindowProgressBarShared> pShared;

	float fStepSize;
	bool bShowFirstElement;

protected:
	NDb::SWindow *GetInstance() override
	{
		return pInstance;
	}

public:
	int operator&( IBinSaver &saver ) override;
	void InitByDesc( const struct NDb::SUIDesc *pDesc ) override;
	void Reposition( const CTRect<float> &parentRect ) override;

	void Visit( struct IUIVisitor *pVisitor ) override;

	//IProgressBar{
	void SetPosition( const float fPos ) override;

	float GetPosition() const override
	{
		return pInstance->fProgress;
	}

	void ShowFirstElement( bool bShow ) override;
	void SetForward( const NDb::SBackground *pForward ) override;
	//IProgressBar}
};

class CWindowMultiTextureProgressBar : public CWindow, public IMultiTextureProgressBar
{
	OBJECT_BASIC_METHODS( CWindowMultiTextureProgressBar )

	std::vector<CObj<IWindowPart>> parts;

	CPtr<NDb::SWindowMultiTextureProgressBar> pInstance;
	CDBPtr<NDb::SWindowMultiTextureProgressBarShared> pShared;

protected:
	NDb::SWindow *GetInstance() override
	{
		return pInstance;
	}

	int FindStateIndex( float fProgress );

public:
	int operator&( IBinSaver &saver ) override;
	void InitByDesc( const struct NDb::SUIDesc *pDesc ) override;
	void Reposition( const CTRect<float> &parentRect ) override;

	void Visit( struct IUIVisitor *pVisitor ) override;

	//IMultiTextureProgressBar{
	bool IsSolid() const override;
	void GetPositions( std::vector<float> *pPositions ) const override;
	void SetPositions( const std::vector<float> &positions, bool bSolid ) override;
	//IMultiTextureProgressBar}
};
