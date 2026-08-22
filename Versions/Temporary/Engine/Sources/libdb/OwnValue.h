#pragma once

#include <boost/uuid/uuid.hpp>

namespace NDb
{
struct IObjMan;
namespace NBind
{
class CBindArray;

union UValue
{
	int nValue;
	float fValue;
	bool bValue;
	std::string *pString;
	std::wstring *pWString;
	boost::uuids::uuid *pGUID;
	void *pBLOB;
	CBindArray *pArray;
	IObjMan *pObjMan;

	//
	UValue(): pBLOB(0) {}
};

}
}


