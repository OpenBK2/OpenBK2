#pragma once

// automatically generated file, don't change manually!


struct IXmlSaver;

namespace NDb
{

	struct SPreLight : public CResource
	{
		OBJECT_BASIC_METHODS( SPreLight )
	public:
		enum { typeID = 0x10082C80 };
		CVec3 vLightColor;
		CVec3 vAmbientColor;
		CVec3 vShadeColor;
		CVec3 vShadeAmbientColor;
		bool bWhitening;
		float fPitch;

		#include "include_prelight.h"

		SPreLight() :
			vLightColor( VNULL3 ),
			vAmbientColor( VNULL3 ),
			vShadeColor( VNULL3 ),
			vShadeAmbientColor( VNULL3 ),
			bWhitening( true ),
			fPitch( 0.0f )
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

	struct STwoSidedLight : public CResource
	{
		OBJECT_BASIC_METHODS( STwoSidedLight )
	public:
		enum { typeID = 0x10087440 };
		CVec3 vLightColor;
		CVec3 vAmbientColor;
		CVec3 vShadeColor;
		CVec3 vShadeAmbientColor;
		bool bWhitening;
		float fPitch;
		float fYaw;

		#include "include_twosidedlight.h"

		STwoSidedLight() :
			vLightColor( VNULL3 ),
			vAmbientColor( VNULL3 ),
			vShadeColor( VNULL3 ),
			vShadeAmbientColor( VNULL3 ),
			bWhitening( true ),
			fPitch( 0.0f ),
			fYaw( 0.0f )
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

