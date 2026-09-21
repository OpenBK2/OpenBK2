// dbstruct.sql translated to SQLite, generated rather than written by hand.
// Included as the body of a string literal by DatabaseSqlite.cpp.
R"SQL(
CREATE TABLE IF NOT EXISTS `cdkeys` (
  `cdkeyID` INTEGER PRIMARY KEY AUTOINCREMENT,
  `CDKey` TEXT NOT NULL DEFAULT '',
  `banned` INTEGER NOT NULL DEFAULT '0'
);

CREATE TABLE IF NOT EXISTS `friendlist` (
  `uid` INTEGER PRIMARY KEY AUTOINCREMENT,
  `player` INTEGER NOT NULL DEFAULT '0',
  `notifier` INTEGER NOT NULL DEFAULT '0'
);

CREATE TABLE IF NOT EXISTS `gamestats` (
  `statsID` INTEGER PRIMARY KEY AUTOINCREMENT,
  `keypointeff` INTEGER NOT NULL DEFAULT '0',
  `level` INTEGER NOT NULL DEFAULT '0',
  `lossesagainst0` INTEGER NOT NULL DEFAULT '0',
  `lossesagainst1` INTEGER NOT NULL DEFAULT '0',
  `lossesagainst2` INTEGER NOT NULL DEFAULT '0',
  `lossesagainst3` INTEGER NOT NULL DEFAULT '0',
  `mapsplayed0` INTEGER NOT NULL DEFAULT '0',
  `mapsplayed1` INTEGER NOT NULL DEFAULT '0',
  `mapsplayed2` INTEGER NOT NULL DEFAULT '0',
  `mapsplayed3` INTEGER NOT NULL DEFAULT '0',
  `mapsplayed4` INTEGER NOT NULL DEFAULT '0',
  `mapsplayed5` INTEGER NOT NULL DEFAULT '0',
  `mapsplayed6` INTEGER NOT NULL DEFAULT '0',
  `mapsplayed7` INTEGER NOT NULL DEFAULT '0',
  `maxxpearned` INTEGER NOT NULL DEFAULT '0',
  `maxxplost` INTEGER NOT NULL DEFAULT '0',
  `racelossessolo0` INTEGER NOT NULL DEFAULT '0',
  `racelossessolo1` INTEGER NOT NULL DEFAULT '0',
  `racelossessolo2` INTEGER NOT NULL DEFAULT '0',
  `racelossessolo3` INTEGER NOT NULL DEFAULT '0',
  `racelossesteam0` INTEGER NOT NULL DEFAULT '0',
  `racelossesteam1` INTEGER NOT NULL DEFAULT '0',
  `racelossesteam2` INTEGER NOT NULL DEFAULT '0',
  `racelossesteam3` INTEGER NOT NULL DEFAULT '0',
  `racewinssolo0` INTEGER NOT NULL DEFAULT '0',
  `racewinssolo1` INTEGER NOT NULL DEFAULT '0',
  `racewinssolo2` INTEGER NOT NULL DEFAULT '0',
  `racewinssolo3` INTEGER NOT NULL DEFAULT '0',
  `racewinsteam0` INTEGER NOT NULL DEFAULT '0',
  `racewinsteam1` INTEGER NOT NULL DEFAULT '0',
  `racewinsteam2` INTEGER NOT NULL DEFAULT '0',
  `racewinsteam3` INTEGER NOT NULL DEFAULT '0',
  `reinforcementused0` INTEGER NOT NULL DEFAULT '0',
  `reinforcementused1` INTEGER NOT NULL DEFAULT '0',
  `reinforcementused10` INTEGER NOT NULL DEFAULT '0',
  `reinforcementused11` INTEGER NOT NULL DEFAULT '0',
  `reinforcementused12` INTEGER NOT NULL DEFAULT '0',
  `reinforcementused13` INTEGER NOT NULL DEFAULT '0',
  `reinforcementused14` INTEGER NOT NULL DEFAULT '0',
  `reinforcementused15` INTEGER NOT NULL DEFAULT '0',
  `reinforcementused16` INTEGER NOT NULL DEFAULT '0',
  `reinforcementused17` INTEGER NOT NULL DEFAULT '0',
  `reinforcementused18` INTEGER NOT NULL DEFAULT '0',
  `reinforcementused19` INTEGER NOT NULL DEFAULT '0',
  `reinforcementused2` INTEGER NOT NULL DEFAULT '0',
  `reinforcementused20` INTEGER NOT NULL DEFAULT '0',
  `reinforcementused21` INTEGER NOT NULL DEFAULT '0',
  `reinforcementused22` INTEGER NOT NULL DEFAULT '0',
  `reinforcementused23` INTEGER NOT NULL DEFAULT '0',
  `reinforcementused24` INTEGER NOT NULL DEFAULT '0',
  `reinforcementused25` INTEGER NOT NULL DEFAULT '0',
  `reinforcementused26` INTEGER NOT NULL DEFAULT '0',
  `reinforcementused27` INTEGER NOT NULL DEFAULT '0',
  `reinforcementused28` INTEGER NOT NULL DEFAULT '0',
  `reinforcementused29` INTEGER NOT NULL DEFAULT '0',
  `reinforcementused3` INTEGER NOT NULL DEFAULT '0',
  `reinforcementused4` INTEGER NOT NULL DEFAULT '0',
  `reinforcementused5` INTEGER NOT NULL DEFAULT '0',
  `reinforcementused6` INTEGER NOT NULL DEFAULT '0',
  `reinforcementused7` INTEGER NOT NULL DEFAULT '0',
  `reinforcementused8` INTEGER NOT NULL DEFAULT '0',
  `reinforcementused9` INTEGER NOT NULL DEFAULT '0',
  `techsplayed0` INTEGER NOT NULL DEFAULT '0',
  `techsplayed1` INTEGER NOT NULL DEFAULT '0',
  `techsplayed2` INTEGER NOT NULL DEFAULT '0',
  `techsplayed3` INTEGER NOT NULL DEFAULT '0',
  `totalplaytime` INTEGER NOT NULL DEFAULT '0',
  `unitseff` INTEGER NOT NULL DEFAULT '0',
  `unitskilled` INTEGER NOT NULL DEFAULT '0',
  `unitslost` INTEGER NOT NULL DEFAULT '0',
  `winsagainst0` INTEGER NOT NULL DEFAULT '0',
  `winsagainst1` INTEGER NOT NULL DEFAULT '0',
  `winsagainst2` INTEGER NOT NULL DEFAULT '0',
  `winsagainst3` INTEGER NOT NULL DEFAULT '0',
  `xp` INTEGER NOT NULL DEFAULT '0',
  `mapsplayed8` INTEGER NOT NULL DEFAULT '0',
  `mapsplayed9` INTEGER NOT NULL DEFAULT '0',
  `totalgamesplayed` INTEGER NOT NULL DEFAULT '0',
  `medals0` INTEGER NOT NULL DEFAULT '0',
  `medals1` INTEGER NOT NULL DEFAULT '0',
  `medals2` INTEGER NOT NULL DEFAULT '0',
  `medals3` INTEGER NOT NULL DEFAULT '0',
  `winsinseries` INTEGER NOT NULL DEFAULT '0'
);

CREATE TABLE IF NOT EXISTS `ignorelist` (
  `uid` INTEGER PRIMARY KEY AUTOINCREMENT,
  `recipient` INTEGER NOT NULL DEFAULT '0',
  `sender` INTEGER NOT NULL DEFAULT '0'
);

CREATE TABLE IF NOT EXISTS `names` (
  `nameID` INTEGER PRIMARY KEY AUTOINCREMENT,
  `Name` TEXT NOT NULL DEFAULT '',
  `banned` INTEGER NOT NULL DEFAULT '0'
);

CREATE TABLE IF NOT EXISTS `resultslog` (
  `LogID` INTEGER PRIMARY KEY AUTOINCREMENT,
  `gamelength` INTEGER NOT NULL DEFAULT '0',
  `mapplayed` INTEGER NOT NULL DEFAULT '0',
  `playerid` INTEGER NOT NULL DEFAULT '0',
  `techplayed` INTEGER NOT NULL DEFAULT '0',
  `win` INTEGER NOT NULL DEFAULT '0',
  `logdate` TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS `serverlog` (
  `recordid` INTEGER PRIMARY KEY AUTOINCREMENT,
  `logtime` TEXT NOT NULL DEFAULT '0000-00-00 00:00:00',
  `GLOBAL_TotalGamesCreated` REAL DEFAULT '-1',
  `GLOBAL_ThroughServerConnections` REAL DEFAULT '-1',
  `GLOBAL_TotalLogins` REAL DEFAULT '-1',
  `MySQL_QueriesPerSecond` REAL DEFAULT '-1',
  `CUSTOM_GamesAverage` REAL DEFAULT '-1',
  `CUSTOM_GamesPerSecond` REAL DEFAULT '-1',
  `CUSTOM_PlayersAverage` REAL DEFAULT '-1',
  `CUSTOM_TotalPlayersEntered` REAL DEFAULT '-1',
  `CUSTOM_GamesTotal` REAL DEFAULT '-1',
  `LADDER_GamesAverage` REAL DEFAULT '-1',
  `LADDER_GamesPerSecond` REAL DEFAULT '-1',
  `LADDER_RequestsPerSecond` REAL DEFAULT '-1',
  `LADDER_PlayersAverage` REAL DEFAULT '-1',
  `LADDER_TotalPlayersEntered` REAL DEFAULT '-1',
  `LADDER_GamesTotal` REAL DEFAULT '-1',
  `LADDER_AverageWaitTime` REAL DEFAULT '-1'
);

CREATE TABLE IF NOT EXISTS `users` (
  `userID` INTEGER PRIMARY KEY AUTOINCREMENT,
  `name` INTEGER NOT NULL DEFAULT '0',
  `cdkey` INTEGER NOT NULL DEFAULT '0',
  `email` TEXT NOT NULL DEFAULT '',
  `password` TEXT NOT NULL DEFAULT '',
  `gamestats` INTEGER NOT NULL DEFAULT '0'
);

CREATE TABLE IF NOT EXISTS `validcdkeys` (
  `cdkeyID` INTEGER PRIMARY KEY AUTOINCREMENT,
  `cdkey` TEXT NOT NULL DEFAULT ''
);

CREATE UNIQUE INDEX IF NOT EXISTS `names_name` ON `names` (`Name`);

CREATE UNIQUE INDEX IF NOT EXISTS `validcdkeys_cdkey` ON `validcdkeys` (`cdkey`);

CREATE INDEX IF NOT EXISTS `cdkeys_cdkey` ON `cdkeys` (`CDKey`);
)SQL"
