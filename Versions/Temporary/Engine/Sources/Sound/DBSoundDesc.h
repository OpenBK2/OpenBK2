#pragma once

// automatically generated file, don't change manually!

#include "System/FilePath.h"

#include <cstdint>

struct IXmlSaver;

namespace NDb
{

	struct SSoundDesc : public CResource
	{
		OBJECT_BASIC_METHODS( SSoundDesc )
	public:
		enum { typeID = 0x1107BAC0 };
		NFile::CFilePath szSoundPath;

		SSoundDesc() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const { return 0; }
	};
}

