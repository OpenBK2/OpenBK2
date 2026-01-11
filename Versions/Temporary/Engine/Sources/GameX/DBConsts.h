#pragma once

// automatically generated file, don't change manually!

#include "SceneB2/dbsceneconsts.h"

struct IXmlSaver;

namespace NDb
{
	struct SAIGameConsts;
	struct SClientGameConsts;
	struct SUIGameConsts;
	struct SSceneConsts;
	struct SMultiplayerConsts;
	struct SNetGameConsts;

	struct SGameConsts : public CResource
	{
		OBJECT_BASIC_METHODS( SGameConsts )
	public:
		enum { typeID = 0x11074CC1 };
	private:
		mutable DWORD __dwCheckSum;
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
		DWORD CalcCheckSum() const;
		DWORD GetMPDataVersionChecksum() const;
	};
}

