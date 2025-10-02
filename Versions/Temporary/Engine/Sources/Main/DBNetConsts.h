#pragma once

// automatically generated file, don't change manually!


struct IXmlSaver;

namespace NDb
{

	struct SNetGameConsts : public CResource
	{
		OBJECT_BASIC_METHODS( SNetGameConsts )
	public:
		enum { typeID = 0x300A7B40 };
		int nMaxLatency;
		int nTimeToStartLagByNoSegmentData;
		int nTimeToAllowDropByLag;
		int nTimeOutTime;
		int nTimeBWTimeOuts;
		int nPort;

		SNetGameConsts() :
			nMaxLatency( 0 ),
			nTimeToStartLagByNoSegmentData( 0 ),
			nTimeToAllowDropByLag( 0 ),
			nTimeOutTime( 0 ),
			nTimeBWTimeOuts( 0 ),
			nPort( 0 )
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

