#pragma once

interface IXmlSaver;

class CXmlResource : public CObjectBase
{
public:
	virtual int operator&( IXmlSaver &saver ) { return 0; }
};

template <class TYPE>
struct SKnownEnum
{
	enum { isKnown = 0 };
};

