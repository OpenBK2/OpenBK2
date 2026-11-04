#include "stdafx.h"

#include "MessageParts.h"
#include "Misc/StrProc.h"
#include "System/Text.h"
#include "UISpecificB2/DBUISpecificB2.h"

#include <utility>

namespace NGameX
{
const std::wstring &FindMessagePart( const NDb::SUIConstsB2 *pConsts, const std::wstring &wszName )
{
	static const std::wstring empty;
	if ( !pConsts )
	{
		return empty;
	}
	const std::string szName = NStr::ToMBCS( wszName );
	if ( pConsts->pMessageParts )
	{
		for ( const auto &part : pConsts->pMessageParts->parts )
		{
			if ( part.szValName == szName )
			{
				const std::wstring &text = NText::GetText( part.szValueTextFileRef );
				if ( !text.empty() )
				{
					return text;
				}
			}
		}
	}
	for ( const auto &tag : pConsts->tags )
	{
		if ( tag.szName == szName )
		{
			const std::wstring &text = NText::GetText( tag.szTextFileRef );
			if ( !text.empty() )
			{
				return text;
			}
		}
	}
	return empty;
}

const std::wstring &GetMessagePart( const NDb::SUIConstsB2 *pConsts, const std::wstring &wszName )
{
	const std::wstring &text = FindMessagePart( pConsts, wszName );
	if ( !text.empty() )
	{
		return text;
	}
	// These are backups for old game data, never the primary localization source.
	static const std::pair<const wchar_t *, std::wstring> defaults[] = {
		{ L"MP_CHAT_TEAM_SUFFIX", L" (Team)" },
		{ L"MP_GAME_TYPE_CLASSIC", L"CLASSIC" },
		{ L"MP_GAME_TYPE_COOP", L"COOP" },
		{ L"REINFORCEMENT_TIMER_NEVER", L"never" },
		{ L"AI_PLAYER_NAME_PREFIX", L"AI " },
		{ L"PLAYER_NAME_PREFIX", L"Player " },
		{ L"REPLAY_PLAYER_WINNER", L"Winner" },
		{ L"REPLAY_PLAYER_LOSER", L"Loser" },
		{ L"REPLAY_PLAYER_UNFINISHED", L"Unfinished" },
		{ L"MP_RULE_MISSING_DATA", L"The multiplayer map or multiplayer constants are missing." },
		{ L"MP_RULE_UNSUPPORTED_GAME_TYPE", L"The map has an unsupported game type." },
		{ L"MP_RULE_INVALID_FORCED_TECH_LEVEL", L"The map's forced tech level is outside MultiplayerConsts.TechLevels." },
		{ L"MP_RULE_INVALID_TECH_LEVEL", L"Select a valid multiplayer tech level." },
		{ L"MP_RULE_INVALID_ALLOWED_NATION",
	    L"The map's allowed nations contain an invalid MultiplayerConsts side index." },
		{ L"MP_RULE_NO_AVAILABLE_NATION", L"No allowed nation is available at this tech level." },
		{ L"MP_RULE_INVALID_LOBBY_CAPACITY",
	    L"The lobby requires at least two human slots and cannot exceed the map's player limit." },
		{ L"MP_RULE_COOP_INVALID_PLAYER_LAYOUT",
	    L"COOP maps require at most 16 map players, with the neutral player last." },
		{ L"MP_RULE_COOP_INVALID_PLAYER_ROLES",
	    L"COOP players must be HUMAN on side 0 or AI_BOT on side 1, followed by one neutral." },
		{ L"MP_RULE_COOP_INSUFFICIENT_HUMAN_SLOTS",
	    L"The COOP map declares more human slots than its HUMAN players provide." },
		{ L"MP_RULE_LOBBY_SETTINGS_MISMATCH", L"The lobby settings do not match the map's rules." },
		{ L"MP_RULE_INVALID_PLAYER_ASSIGNMENT", L"A player occupies an unavailable slot, team or nation." },
		{ L"MP_RULE_PLAYERS_NOT_READY", L"Every player must be ready before starting." },
		{ L"MP_RULE_INVALID_PLAYER_COLOUR", L"A player has selected an unavailable colour." },
		{ L"MP_RULE_DUPLICATE_PLAYER_COLOURS", L"Players must choose different colours." },
		{ L"CANT_START_COOP_PLAYERS", L"At least two human players are required.<br>The game will not start." },
		{ L"CANT_START_SAME_TEAM", L"All players have the same team. The game will not start." },
		{ L"MP_JOIN_INVALID_HUMAN_SLOT", L"The host assigned an unavailable human player slot." },
		{ L"TIME_MINUTES_SUFFIX", L" min" },
		{ L"MP_TIME_LIMIT_NONE", L"--:--" } };
	for ( const auto &entry : defaults )
	{
		if ( wszName == entry.first )
		{
			return entry.second;
		}
	}
	static const std::wstring empty;
	return empty;
}
} // namespace NGameX
