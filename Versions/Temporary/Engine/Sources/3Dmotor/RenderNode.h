#pragma once

namespace NGScene
{

template<class T>
class CObjectSet : public CObjectBase
{
public:
	std::vector< CObj<T> > parts;
	//
	void AddPart( T *pPart ) { parts.push_back( pPart ); }
	int operator&( CStructureSaver &f ) { f.Add( 1, &parts );	return 0; }
};

class CRenderNode : public CObjectSet<CObjectBase>
{
	OBJECT_BASIC_METHODS(CRenderNode);
public:
	ZDATA_(CObjectSet<CObjectBase>)
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CObjectSet<CObjectBase>*)this); return 0; }
};

} // namespace NGScene


