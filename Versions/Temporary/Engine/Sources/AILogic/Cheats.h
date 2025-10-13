#pragma once

#include <cstdint>

struct SCheats
{
	public: int operator&( IBinSaver &saver ); private:;
private:
	// просчитывать ли туман bWarFog
	bool bWarFog;
	// для кого просчитывать туман
	int nPartyForWarFog;

	// загружать статич. объекты и реки или нет
	bool bLoadObjects;
	
	bool bTurnOffWarFog;
	bool bHistoryPlaying;
	//
	// убиваема или нет данная сторона
	std::vector<uint8_t> immortals;
	// убивает ли данная сторона с первого раза
	std::vector<uint8_t> firstShoot;

	bool bPasswordOK;
public:
	SCheats();

	void Init();
	void Clear() { Init(); }

	void SetWarFog( bool bWarFog );
	bool GetWarFog() const { return bWarFog; }

	void SetNPartyForWarFog( const int nPartyForWarFog, bool bUnconditionly );
	const int GetNPartyForWarFog() const { return nPartyForWarFog; }

	void SetLoadObjects( bool bLoadObjects );
	bool GetLoadObjects() const { return bLoadObjects; }

	void SetTurnOffWarFog( bool bTurnOffWarFog );
	bool GetTurnOffWarFog() const;

	void SetImmortals( const int nPlayer, const uint8_t cValue );
	uint8_t GetImmortals( const int nPlayer ) const { return immortals[nPlayer]; }

	void SetFirstShoot( const int nPlayer, const uint8_t cValue );
	uint8_t GetFirstShoot( const int nPlayer ) const { return firstShoot[nPlayer]; }
	
	void SetHistoryPlaying( bool _bHistoryPlaying ) { bHistoryPlaying = _bHistoryPlaying; }
	bool IsHistoryPlaying() const { return bHistoryPlaying; }
	
	const unsigned long MakeCheckSum( const std::string &szPassword );
	void CheckPassword( const std::string &szPassword );
	
	bool IsPasswordOk() const { return bPasswordOK; }
};


