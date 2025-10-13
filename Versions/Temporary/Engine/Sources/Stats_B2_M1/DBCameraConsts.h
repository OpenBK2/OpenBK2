#pragma once

#include <cstdint>

// automatically generated file, don't change manually!

struct IXmlSaver;

namespace NDb
{

	struct SCameraLimits : public CResource
	{
		OBJECT_BASIC_METHODS( SCameraLimits )
	public:
		enum { typeID = 0x1007B4C0 };
	private:
		mutable uint32_t __dwCheckSum;
	public:

		struct SCLLimit
		{
		private:
			mutable uint32_t __dwCheckSum;
		public:
			float fMin;
			float fMax;
			float fAve;
			float fAutoSpeed;
			float fManualSpeed;
			bool bCyclic;

			SCLLimit() :
				__dwCheckSum( 0 ),
				fMin( 0.0f ),
				fMax( 0.0f ),
				fAve( 0.0f ),
				fAutoSpeed( 0.0f ),
				fManualSpeed( 0.0f ),
				bCyclic( false )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			uint32_t CalcCheckSum() const;
		};
		SCLLimit distanceLimit;
		SCLLimit pitchLimit;
		SCLLimit yawLimit;
		float fFOV;

		SCameraLimits() :
			__dwCheckSum( 0 ),
			fFOV( 26 )
		{ }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const;
	};
}

