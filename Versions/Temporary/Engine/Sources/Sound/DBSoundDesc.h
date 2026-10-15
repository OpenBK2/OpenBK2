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
		int GetTypeID() const override { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver ) override;
		int operator&( IXmlSaver &saver ) override;
		uint32_t CalcCheckSum() const override { return 0; }
	};
}
