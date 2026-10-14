// Pasted into the generated SGameConsts (DBConsts.h) by the #include in its
// .cll: the checksum of the data a multiplayer game must agree on, defined in
// include_GameConsts_cpp.h.
private:
// Runtime-only snapshots: rebuilding the database creates a fresh cache.
mutable bool bMPDataVersionChecksumValid = false;
mutable uint32_t nMPDataVersionChecksum = 0;
mutable bool bMPMapChecksumValid = false;
mutable CDBPtr<NDb::SMultiplayerMap> pMPChecksumMap;
mutable uint32_t nMPDataVersionChecksumWithMap = 0;

public:
// Menu reads reuse the snapshot. Startup, mod reloads and MP attempts refresh it.
GAMEX_EXPORT uint32_t GetMPDataVersionChecksum() const;
GAMEX_EXPORT uint32_t RecalculateMPDataVersionChecksum() const;
GAMEX_EXPORT uint32_t GetMPDataVersionChecksumWithMap( CDBPtr<NDb::SMultiplayerMap> map ) const;
