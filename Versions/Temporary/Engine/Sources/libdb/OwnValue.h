#pragma once

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
	string *pString;
	wstring *pWString;
	GUID *pGUID;
	void *pBLOB;
	CBindArray *pArray;
	IObjMan *pObjMan;

	//
	UValue(): pBLOB(0) {}
};

}
}


