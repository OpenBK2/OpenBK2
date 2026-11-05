-- Requires an objective at index 0 and the normal common scripts.
function ObjectiveCountdown()
	-- Postfixes can be prepared before the notification is shown.
	UpdateObjectiveNotification( 0, " - 30 s" );
	KeepObjectiveNotified( 0, 1 );
	for seconds = 29, 0, -1 do
		Wait( 1 );
		UpdateObjectiveNotification( 0, " - " .. seconds .. " s" );
	end;
	-- A completed or failed objective stays pinned until the script removes it.
	KeepObjectiveNotified( 0, 0 );
	UpdateObjectiveNotification( 0, "" );
end;

StartThread( ObjectiveCountdown );
