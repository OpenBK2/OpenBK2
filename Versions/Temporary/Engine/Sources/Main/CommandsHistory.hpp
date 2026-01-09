#pragma once

#include "System/CheckSumLog.h"
#include "System/Basic.h"
#include "GameX/AILogicCommandInternal.h"
#include "Stats_B2_M1/ActionCommand.h"
#include <unordered_map>

namespace NDb
{
	struct SMapInfo;
}

using CHistory = std::unordered_map<int, std::list<CPtr<IAILogicCommandB2>>>;

struct ICommandsHistory : public ICheckSumLog
{
	virtual void StartNewGame( const NDb::SMapInfo *pMap ) = 0;
	virtual const NDb::SMapInfo *GetMap() const = 0;
	
	virtual void AddCommand( const int nSegment, struct IAILogicCommandB2 *pCmd ) = 0;
	virtual void ExecuteSegmentCommands( const int nSegment, struct ITransceiver *pTranceiver ) = 0;

	//virtual bool SaveReplay( const string &szFileName );

	// only remember last checksum
	virtual bool AddChecksumLog( const int nGameTime, const unsigned long ulChecksum, const int nEntry ) = 0;
	virtual const unsigned long GetLastChecksum() const = 0;
	virtual CHistory GetCommandsHistory() const = 0;

	virtual void WriteCommandsHistoryToFile(const std::string& path)
	{
		auto cmds = GetCommandsHistory();

		if (cmds.size() > 0)
		{
			FILE* f = fopen(path.c_str(), "w");
			fprintf(f, "Commands:\n");
			using cmd_pair = std::pair<int, std::list<CPtr<IAILogicCommandB2>>>;
			std::vector<cmd_pair> all;
			for (auto pair : cmds)
				all.push_back({ pair.first, pair.second });
			std::sort(all.begin(), all.end(), [](const cmd_pair& p1, const cmd_pair& p2) { return p1.first < p2.first; });
			for (int i = 0; i < all.size(); i++)
			{
				auto pair = all[i];
				fprintf(f, "frame: %d [%d]\n", i, pair.first);
				for (auto fcmd : pair.second)
				{
					auto ptr = fcmd.GetPtr();
					fprintf(f, "\t%s {%s}", ptr->GetFullTypeName(), ptr->GetDebugInfo().c_str());
					CB2GroupCommand* gc = dynamic_cast<CB2GroupCommand*>(ptr);
					if (gc)
					{
						auto cmde = gc->GetCommand();
						auto cmdname = ActionCommandToString.at(cmde->nCmdType);
						fprintf(f, " [%s]", cmdname.c_str());
					}
					CUnitCommand* ucmd = dynamic_cast<CUnitCommand*>(ptr);
					if (ucmd)
					{
						auto cmde = ucmd->GetCommand();
						auto cmdnameiter = ActionCommandToString.find(cmde->nCmdType);
						if (cmdnameiter != ActionCommandToString.end())
							fprintf(f, " [%s]", cmdnameiter->second.c_str());
						else
							fprintf(f, " UNKNOWN_CMD[%d]", (int)cmde->nCmdType);
					}
					fprintf(f, "\n");
				}
			}
			fclose(f);
		}
	}
};
struct SReplayInfo;
ICommandsHistory *CreateCommandsHistory( const SReplayInfo &replay );

