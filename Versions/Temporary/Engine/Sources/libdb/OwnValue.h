#pragma once

#include <string>

#include <boost/uuid/uuid.hpp>

class CDBID;

namespace NDb
{
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
		// Keep a reference's identity when its target is removed from the editor database.
		CDBID *pDBID;

		//
		UValue()
			: pBLOB( 0 )
		{
		}
	};

} // namespace NBind
} // namespace NDb
