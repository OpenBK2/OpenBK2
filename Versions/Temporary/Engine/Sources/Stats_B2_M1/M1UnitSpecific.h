#pragma once

// automatically generated file, don't change manually!

#include "rpgstats.h"

struct IXmlSaver;

namespace NDb
{
	struct SVisObj;

	struct SM1UnitSpecific : public CResource
	{
	public:
	private:
		mutable DWORD __dwCheckSum;
	public:

		SM1UnitSpecific() :
			__dwCheckSum( 0 )
		{ }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SM1UnitHelicopter : public SM1UnitSpecific
	{
		OBJECT_BASIC_METHODS( SM1UnitHelicopter )
	public:
		enum { typeID = 0x31197340 };
	private:
		mutable DWORD __dwCheckSum;
	public:

		struct SHelicopterAxis
		{
		private:
			mutable DWORD __dwCheckSum;
		public:
			CDBPtr< SVisObj > pScaled;
			CDBPtr< SVisObj > pDynamic;
			string szLocatorName;
			float fStartScaleSpeed;
			float fHideStaticSpeed;

			SHelicopterAxis() :
				__dwCheckSum( 0 ),
				fStartScaleSpeed( 0.0f ),
				fHideStaticSpeed( 0.0f )
			{ }
			//
			void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			DWORD CalcCheckSum() const;
		};
		vector< SHelicopterAxis > axes;
		float fFlyingHeight;
		float fMaxSpeed;
		float fMaxAcceleration;
		float fMaxTilt;
		float fRotationSpeed;
		float fFullSpinTime;

		SM1UnitHelicopter() :
			__dwCheckSum( 0 ),
			fFlyingHeight( 300 ),
			fMaxSpeed( 500 ),
			fMaxAcceleration( 200 ),
			fMaxTilt( 0.7850f ),
			fRotationSpeed( 0.0980f ),
			fFullSpinTime( 2 )
		{ }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};
}

