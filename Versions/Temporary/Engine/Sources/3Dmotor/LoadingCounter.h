#pragma once

class ILoadingCounter: public CObjectBase
{
public:
	virtual void LeftToLoad( int nCount ) = 0;
	virtual void SetTotalCount( int nTotalCount ) = 0;
	virtual void Step() = 0;
};

