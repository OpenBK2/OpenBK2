-- Requires the normal common Definitions/Miscelaneous scripts.
-- Place enemy units in script group 100 and the defended object(s) in group 200.
-- Create script areas named CoopApproach and CoopObjective on the map.
-- Example map player IDs: human 0, AI_BOT 1, human 2, neutral 3.

function CoopMission()
	if GetNUnitsInScriptGroup( 100 ) == 0 or GetNUnitsInScriptGroup( 200 ) == 0 then
		Trace( "COOP example: create script groups 100 and 200 before running this mission" );
		return;
	end;

	local delay = 30 - GetDifficultyLevel() * 5;
	if delay < 5 then
		delay = 5;
	end;
	Wait( delay );
	Cmd( ACT_SWARM, 100, 0, "CoopApproach" );
	QCmd( ACT_SWARM, 100, 0, "CoopObjective" );

	while 1 do
		Wait( 1 );
		if GetNUnitsInScriptGroup( 200 ) == 0 then
			Win( 1 );
			return;
		end;
		if GetNUnitsInScriptGroup( 100 ) == 0 then
			Win( 0 );
			return;
		end;
	end;
end;

StartThread( CoopMission );
