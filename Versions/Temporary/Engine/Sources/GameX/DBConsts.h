#pragma once

// automatically generated file, don't change manually!

#include "SceneB2/DBSceneConsts.h"

#include <cstdint>

struct IXmlSaver;

namespace NDb
{
	struct SAIGameConsts;
	struct SClientGameConsts;
	struct SUIGameConsts;
	struct SSceneConsts;
	struct SMultiplayerConsts;
	struct SNetGameConsts;
	struct SMultiplayerMap;

	struct SGameConsts : public CResource
	{
		OBJECT_BASIC_METHODS( SGameConsts )
	public:
		enum { typeID = 0x11074CC1 };
	private:
		mutable uint32_t __dwCheckSum;
	public:
		CDBPtr< SAIGameConsts > pAI;
		CDBPtr< SNetGameConsts > pNet;
		CDBPtr< SClientGameConsts > pClient;
		CDBPtr< SUIGameConsts > pUI;
		CDBPtr< SSceneConsts > pScene;
		CDBPtr< SMultiplayerConsts > pMultiplayer;

		SGameConsts() :
			__dwCheckSum( 0 )
		{ }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const;
		uint32_t GetMPDataVersionChecksum() const;
		uint32_t GetMPDataVersionChecksumWithMap(CDBPtr<NDb::SMultiplayerMap> map) const;
	};
}

