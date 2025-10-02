#pragma once

// automatically generated file, don't change manually!

#include "3dmotor/dbscene.h"
#include "animationtype.h"

struct IXmlSaver;

namespace NDb
{
	enum EAnimationType;

	struct SAnimAABB
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		CVec3 vCenter;
		CVec3 vHalfSize;

		SAnimAABB() :
			__dwCheckSum( 0 ),
			vCenter( VNULL3 ),
			vHalfSize( VNULL3 )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
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

		SAnimB2() :
			eType( ANIMATION_UNKNOWN ),
			nAction( 0 ),
			nLength( 0 ),
			bLooped( false ),
			nWeaponsToUseWith( 0 ),
			fMoveSpeed( 0.0700f )
		{ }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};
}

