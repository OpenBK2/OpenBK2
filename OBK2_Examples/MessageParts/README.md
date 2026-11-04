# Localized message parts

`messages.pak` is a ZIP archive containing `MessageParts.xdb` and all 29 English
text files under `Other/Text/Game/Messages/`. Copy it into the game's or mod's
`Data` directory. Link the record from the active `UIConstsB2`:

```xml
<MessageParts href="/Other/Text/Game/Messages/MessageParts.xdb" />
```

The repository's `Consts/Game/Game_UIConstsB2.xdb` already has this link. Mods
with their own UI constants need to add it to their own record. The archive does
not replace the rest of a mod's UI constants.

The record contains a `Parts` list with `ValName` and `ValueTextFileRef` pairs:

```xml
<MessageParts>
	<Parts>
		<Item>
			<ValName>MP_CHAT_TEAM_SUFFIX</ValName>
			<ValueTextFileRef href="MP_CHAT_TEAM_SUFFIX.txt" />
		</Item>
	</Parts>
</MessageParts>
```

Use `<val MP_CHAT_TEAM_SUFFIX>` in UI text. Names are case-sensitive. Resolution
tries `MessageParts`, then the existing `UIConstsB2/Tags` values, then the built-in
English text. Missing records, entries, empty text and unreadable files fall
through. Other existing `<val>` values and dynamic `<val %name>` parameters keep
working. Each name should occur only once; the first nonempty entry wins.

The co-op minimum-player warning and replay result labels retain their existing
screen `RelatedTexts` translations when neither `MessageParts` nor general
`Tags` supplies a value. CLASSIC also retains its original same-team warning in
that case. Those translations take precedence over the built-in English.

The catalogue covers chat team marking, CLASSIC/COOP names, reinforcement timer
text, unnamed AI/player prefixes, replay results, map/lobby validation errors,
the invalid join-slot error and lobby time display. All keys and their English
values are listed in `messages.json`.

Translate the text files, keeping their names and XDB references. Game text files
must be UTF-16LE with a BOM. Preserve leading/trailing spaces in prefixes and
suffixes; do not add a trailing newline to displayed text. `<br>` is a UI line
break. Player numbers are appended by the game, and map-authored player names
continue to use their existing localized text references. New replay snapshots
retain `<val>` tags for unnamed bots; previously recorded literal names remain
as recorded.

To rebuild the English archive and matching loose files in the repository's
`Versions/Current/Data`, edit the UTF-8 `messages.json`, then run:

```sh
python OBK2_Examples/MessageParts/build.py
```

The builder uses fixed ZIP metadata so identical input produces an identical
archive. The `MessageParts_test` checks the archive through the game's VFS and
database, including every text reference and the built-in English backups.
