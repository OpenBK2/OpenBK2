#pragma once


struct ICheckSumLog : virtual public CObjectBase
{ 
	// return false when checksum differs with loaded one
	virtual bool AddChecksumLog( const int nGameTime, const unsigned long ulChecksum, const int nEntry ) = 0;
};


