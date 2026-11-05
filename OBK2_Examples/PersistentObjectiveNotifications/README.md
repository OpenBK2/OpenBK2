# Persistent objective notifications

Mission Lua scripts can pin an objective in the notification area and update its
postfix without recreating the notification or replaying its sound:

```lua
UpdateObjectiveNotification( 0, " - 00:30" );
KeepObjectiveNotified( 0, 1 );
-- Displays the regular objective icon and "Objective: <objective header> - 00:30".

UpdateObjectiveNotification( 0, " - 00:29" );
KeepObjectiveNotified( 0, 0 );
```

Indices are zero-based, matching `ObjectiveChanged`: objective 0 is the first
entry in the map's `Objectives` list. The index must name an existing objective
and be in the range 0-254. The keep flag must be the number `0` or `1`.

`KeepObjectiveNotified(index, 1)` creates at most one persistent entry per
objective. Repeated calls leave that entry in place. Only
`KeepObjectiveNotified(index, 0)` removes it; time passing, clicking the entry,
and completing or failing the objective do not dismiss it. Left-click retains
the regular objective notification action. Normal temporary objective
notifications continue to work separately.

`UpdateObjectiveNotification(index, postfix)` replaces the previous postfix.
Include any desired spaces or punctuation in the string, and use `""` to clear
it. Lua strings are UTF-8. Updating a hidden objective remembers its postfix
without showing a notification, and removing an entry retains that postfix for
its next pin. These calls only change notifications, not objective state or the
objective's header in the objectives screen.

Pins and postfixes are saved with the mission. Replays recreate them through
the normal script feedback flow. Existing maps and saves need no new data.

`mission.lua` demonstrates a 30-second countdown for objective 0. Include the
normal common scripts that provide `Wait` and `StartThread`, and copy the example
into a mission with at least one objective.

The prefix defaults to `Objective: `. To translate it, add the message key
`OBJECTIVE_NOTIFICATION_PREFIX` to the active `UIConstsB2.MessageParts` catalogue
or its existing `Tags` list. Text files use UTF-16LE with a BOM; preserve the
trailing space and omit a trailing newline. The objective name uses its existing
localized `HeaderFileRef`. See [message parts](../MessageParts/README.md).
