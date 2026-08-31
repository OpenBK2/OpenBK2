#pragma once

// automatically generated file, don't change manually!

#include "3Dmotor/DBScene.h"
#include "AnimationType.h"

#include <cstdint>

struct IXmlSaver;

namespace NDb
{
	enum EAnimationType : int;

	struct SAnimAABB
	{
	private:
		mutable uint32_t __dwCheckSum;
	public:
		CVec3 vCenter;
		CVec3 vHalfSize;

		SAnimAABB() :
			__dwCheckSum( 0 ),
			vCenter( VNULL3 ),
			vHalfSize( VNULL3 )
		{ }
		//
		void ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const;
	};

	struct SAnimB2 : public SAnimBase
	{
		OBJECT_BASIC_METHODS( SAnimB2 )
	public:
		enum { typeID = 0x10093480 };
		EAnimationType eType;
		int nAction;
		int nLength;
		bool bLooped;
		int nWeaponsToUseWith;
		SAnimAABB aabb_a;
		SAnimAABB aabb_d;
		float fMoveSpeed;
		NFile::CFilePath szModelFileRef;
		int nFirstFrame;
		int nLastFrame;
		std::string szClipName;

		SAnimB2() :
			eType( ANIMATION_UNKNOWN ),
			nAction( 0 ),
			nLength( 0 ),
			bLooped( false ),
			nWeaponsToUseWith( 0 ),
			fMoveSpeed( 0.0700f ),
			nFirstFrame( 0 ),
			nLastFrame( 0 )
		{ }
		//
		int GetTypeID() const { return typeID; }
		const NFile::CFilePath &GetModelFileRef() const override { return szModelFileRef; }
		const std::string &GetClipName() const override { return szClipName; }
		int GetFirstFrame() const override { return nFirstFrame; }
		int GetLastFrame() const override { return nLastFrame; }
		int GetLengthMilliseconds() const override { return nLength; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const { return 0; }
	};
}

