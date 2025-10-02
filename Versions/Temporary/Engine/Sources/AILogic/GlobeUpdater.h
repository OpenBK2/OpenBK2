#pragma once

class CUpdates2Globe : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( CUpdates2Globe );
	ZDATA
		list< CPtr<CObjectBase> > singleUpdates;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&singleUpdates); return 0; }
public:
	enum { tidTypeID = 0x301312C0 };

	CUpdates2Globe() { }
	void Segment();

	void AddUpdate( CObjectBase *pUpdate );
	CObjectBase* GetUpdate();
};


