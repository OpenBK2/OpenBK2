#pragma once

#include "window.h"



class CWindowProgressBar : public CWindow, public IProgressBar
{
	OBJECT_BASIC_METHODS(CWindowProgressBar)

	CObj<IWindowPart> pForward;						// show increasing progress
	CObj<IWindowPart> pBackward;						// show decreasing part
	CObj<IWindowPart> pGlow;// texture drawn over border between increasing and decreasing parts
	
	CPtr<NDb::SWindowProgressBar> pInstance;
	CDBPtr<NDb::SWindowProgressBarShared> pShared;
	
	float fStepSize;
	bool bShowFirstElement;
protected:
	virtual NDb::SWindow* GetInstance() { return pInstance; }

public:
	int operator&( IBinSaver &saver );
	virtual void InitByDesc( const struct NDb::SUIDesc *pDesc );
	virtual void Reposition( const CTRect<float> &parentRect );

	virtual void Visit( interface IUIVisitor *pVisitor );
	
	//IProgressBar{
	virtual void SetPosition( const float fPos );
	virtual float GetPosition() const { return pInstance->fProgress; }
	void ShowFirstElement( bool bShow );
	void SetForward( const NDb::SBackground *pForward );
	//IProgressBar}
};

class CWindowMultiTextureProgressBar : public CWindow, public IMultiTextureProgressBar
{
	OBJECT_BASIC_METHODS(CWindowMultiTextureProgressBar)

	vector< CObj<IWindowPart> > parts;

	CPtr<NDb::SWindowMultiTextureProgressBar> pInstance;
	CDBPtr<NDb::SWindowMultiTextureProgressBarShared> pShared;
protected:
	virtual NDb::SWindow* GetInstance() { return pInstance; }
	
	int FindStateIndex( float fProgress );
public:
	int operator&( IBinSaver &saver );
	virtual void InitByDesc( const struct NDb::SUIDesc *pDesc );
	virtual void Reposition( const CTRect<float> &parentRect );

	virtual void Visit( interface IUIVisitor *pVisitor );
	
	//IMultiTextureProgressBar{
	virtual bool IsSolid() const;
	virtual void GetPositions( vector<float> *pPositions ) const;
	virtual void SetPositions( const vector<float> &positions, bool bSolid );
	//IMultiTextureProgressBar}
};

