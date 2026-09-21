#pragma once

struct IMultiTester : virtual public CObjectBase
{
	virtual void Init( const std::string& _szServerAddress, const int _nNetVersion, const int _nServerPort, const int _nTimeOut,
		const std::string &_szName, const std::string &_szPassword, const std::string &_szCDKey, const int _nTestMode ) = 0;
	virtual bool Segment() = 0;
	virtual bool IsActive() const = 0;
	virtual bool IsCancelled() const = 0;

};

enum EMultiTestModes
{
	MTM_CHAT = 0x01,
	MTM_LADDER = 0x02
};

IMultiTester *CreateMultiTester();


