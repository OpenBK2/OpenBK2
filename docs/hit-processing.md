# Hit processing, armor, cover, and damage

Source investigation dated 2026-11-08, at commit `0c6712bcf`. This describes the
current implementation, including its exceptions, rather than a proposed combat
model. No gameplay code was changed. Values from a running mod/profile can differ
from both the C++ defaults and the profiles checked into this repository.

## Main result

For an ordinary direct hit on a unit, the rule is:

```text
damage is allowed when:

	impact passes the full unit rectangle and height checks
	AND impact is inside the smaller combat rectangle
	AND modified random piercing >= random armor of the selected side
	AND the cover roll allows the hit
```

Armor is a pass/fail gate, not a subtraction from damage. Excess piercing does not
increase damage. A miss of the smaller rectangle is `EHT_MISS`; failed penetration
or a cover save is `EHT_REFLECT`. The latter is a result/effect classification, not
a simulated ricochet trajectory.

Explosive splash is a separate path. It does **not** compare shell piercing with
armor and does **not** apply a continuous distance falloff. It tests a radius,
uses a fixed armor threshold, and multiplies a fresh damage roll by a constant.
Infantry and static objects have additional rules described below.

## Where the work happens

Paths below are relative to `Versions/Temporary/Engine/Sources`.

| Stage | Main implementation |
|---|---|
| Select direct-only or explosive gun behavior | [AILogic/UnitGuns.cpp](../Versions/Temporary/Engine/Sources/AILogic/UnitGuns.cpp#L61), `CUnitGuns::AddGun` |
| Fire, select projectile, and schedule impact | [AILogic/Guns.cpp](../Versions/Temporary/Engine/Sources/AILogic/Guns.cpp#L1731), `CBasicGun::Fire` |
| Snapshot modifiers and scatter the impact point | [AILogic/Shell.cpp](../Versions/Temporary/Engine/Sources/AILogic/Shell.cpp#L380), `CExplosion::Init` |
| Enumerate targets | [AILogic/Shell.cpp](../Versions/Temporary/Engine/Sources/AILogic/Shell.cpp#L623), `CCumulativeExpl::Explode` and `CBurstExpl::Explode` |
| Direct unit hit, cover, armor, splash | [AILogic/AIUnit.cpp](../Versions/Temporary/Engine/Sources/AILogic/AIUnit.cpp#L166), `ProcessExactHit`, `ProcessCumulativeExpl`, `ProcessAreaDamage`, `ProcessBurstExpl` |
| Infantry splash exception | [AILogic/Soldier.cpp](../Versions/Temporary/Engine/Sources/AILogic/Soldier.cpp#L1329), `CSoldier::ProcessAreaDamage` |
| Static objects and buildings | [AILogic/StaticObject.cpp](../Versions/Temporary/Engine/Sources/AILogic/StaticObject.cpp#L213), `CExistingObject::ProcessCumulativeExpl` |
| Subtract unit HP | [AILogic/AIUnit.cpp](../Versions/Temporary/Engine/Sources/AILogic/AIUnit.cpp#L1193), `CAIUnit::TakeDamage` |

The similarly named `CExplosion::ProcessExactHit` in `Shell.cpp` duplicates the
basic test, but the unit path calls **`CAIUnit::ProcessExactHit`**. AI targeting and
kill-speed estimates elsewhere are not the authoritative hit resolver.

For ordinary line/ballistic shells and guided missiles, gun creation chooses the
direct-only class when the shell's **base `Area2 == 0`**, otherwise the burst class.
Bombs, rockets, flames, and torpedoes have dedicated branches. The class choice is
made before shot-time area modifiers; giving an already direct-only gun a nonzero
`weaponArea2` bonus does not convert its explosion to `CBurstExpl`. The name
`CCumulativeExpl` should therefore be read as this code path, not as proof that
the ammunition physically represents a shaped charge.

## 1. Dispersion chooses the impact point

For the usual gun-based `CExplosion` constructor:

```text
Modifier(x) = ZeroCount != 0 ? 0 : (x + AddBonus) * MultBonus

dispersion = shooter.weaponDispersion(weapon.Dispersion)
distance = length(intendedImpact3D - firingPosition3D)
radius = dispersion * distance / weapon.RangeMax

ratioAtMaxRange = dispersionRatio[trajectory][0]
ratioAtMinRange = dispersionRatio[trajectory][1]
ratio = ratioAtMinRange
	+ (distance - RangeMin) / (RangeMax - RangeMin)
	* (ratioAtMaxRange - ratioAtMinRange)

angle = RandomInteger(0, 65535) * 2*pi / 65536
r = radius * RandomFloat(0, 1)
forward = normalize(horizontal(intendedImpact - firingPosition))
sideways = perpendicular(forward)
offset = forward * (r * cos(angle) * ratio) + sideways * (r * sin(angle))
impact = intendedImpact + (offset.x, offset.y, 0)
```

This is an ellipse when `ratio != 1`. `RandQuadrInCircle` special-cases a zero
ratio as a circle, not a zero-length ellipse. The interpolation is not clamped.
Angle is sampled using the engine's 16-bit direction representation.

The radius itself is sampled uniformly. Points therefore concentrate toward the
center; this is not a uniform distribution over the disk's area. Consequently,
halving the combat rectangle's dimensions does not generally quarter the actual
hit probability, even though it quarters the rectangle's area.

The configuration names are counterintuitive: `AIConsts.cpp` binds the variables
ending in `Min` to index `[0]`, which `GetDispRatio` uses **at maximum range**, and
those ending in `Max` to `[1]`, used **at minimum range**. Read the formula rather
than inferring the mapping from the names.

Important variants:

- Constructors with `bRandomize == false` do not apply this scatter.
- The raw-weapon constructor uses the weapon's base dispersion and ratio `1`.
- Bombs compute dispersion as lateral acceleration during their fall, using
  altitude in the dispersion radius, then create the explosion without scattering
  it again. `ROCKET_GUN` targeting can also randomize the target height.
- Ordinary line shots are delayed invisible shells with a preselected endpoint;
  visible ballistic shells also use a selected endpoint. Do not model all of
  these as per-tick raycasts through a rendered mesh.
- Guided ATGM/SAM trajectories update the impact position during flight and have
  swept collision/proximity logic. On confirmed unit contact, that code moves the
  explosion to the unit's horizontal center. Their final hit is not just the
  original scattered endpoint.

Sources: [Guns.cpp](../Versions/Temporary/Engine/Sources/AILogic/Guns.cpp#L84)
(`GetDispByRadius`, `GetDispRatio`, `Fire`),
[Randomize.h](../Versions/Temporary/Engine/Sources/AILogic/Randomize.h#L22),
[Shell.cpp](../Versions/Temporary/Engine/Sources/AILogic/Shell.cpp#L380),
[AIConsts.cpp](../Versions/Temporary/Engine/Sources/AILogic/AIConsts.cpp#L537),
[modifier implementation](../Versions/Temporary/Engine/Sources/Stats_B2_M1/include_UnitStatsModifier_ParameterModifier.h).

## 2. AABB size and coefficient are two geometric gates

Despite the AABB names, the normal unit hit rectangle is a **rotated 2D footprint**
plus a separate height check. It is not a 3D mesh intersection.

`CAIUnit::GetUnitRect` constructs the full rectangle using:

```text
halfLength = AABBHalfSize.y * BOUND_RECT_FACTOR
halfWidth  = AABBHalfSize.x * BOUND_RECT_FACTOR
center = unitCenter + rotated(AABBCenter)
orientation = unit front direction
```

`BOUND_RECT_FACTOR` is `1.0` in the source. Aviation has an override using its own
direction vector but the same size/center principle.

The direct path first requires the explosion's horizontal point inside this full
rectangle, the unit not in a solid place, and normally:

```text
abs(target.GetVisZ() - impact.z) <= AI_TILES_IN_VIS_TILE * AI_TILE_SIZE
```

That tolerance is `64` AI coordinate units. A configured overhead top-attack ATGM
can instead pass a tolerance around `GetVisZ() + AI_TILE_SIZE + TopTargetingHeight`.
These are explicit tolerances, not a height extracted from the model's box.

Only after this first gate does the direct path compute the combat rectangle:

```text
coefficient = target.smallAABBCoeff(target.SmallAABBCoeff)
combatRect = fullRect

if armorMode != BOTTOM and shooter is valid and not shooter.IgnoreAABBCoeff:
	combatRect.halfLength *= coefficient
	combatRect.halfWidth  *= coefficient
```

The center and orientation are retained. Thus `0.5` halves both dimensions,
`1.0` leaves them unchanged, and a value above `1.0` expands them. The initial full
rectangle gate still limits direct hits even if the combat rectangle is larger.
There is no clamp in `GetRemissiveCoeff` or `SRect::Compress`.

Bottom-armor hits skip compression. The direct path also skips compression when
the shooter reference is invalid, or its ignore-AABB ability is active. Generic
splash skips compression only for bottom mode; it does not check that ability.

For a nondegenerate rectangle, `SRect::IsPointInside` excludes an exactly-on-edge
point. A fully collapsed rectangle instead uses a small squared-distance tolerance
of `< 0.001` around its center. A zero coefficient therefore is not literally an
unconditional immunity flag.

Sources: [AIUnit.cpp](../Versions/Temporary/Engine/Sources/AILogic/AIUnit.cpp#L207),
[GetRemissiveCoeff](../Versions/Temporary/Engine/Sources/AILogic/AIUnit.cpp#L1469),
[GetUnitRect](../Versions/Temporary/Engine/Sources/AILogic/AIUnit.cpp#L3642),
[Aviation.cpp](../Versions/Temporary/Engine/Sources/AILogic/Aviation.cpp#L61),
[Geom.cpp](../Versions/Temporary/Engine/Sources/Misc/Geom.cpp#L285).

## 3. Armor side comes from direction or a preset mode

The explosion passes an armor **mode**, not a face returned by a mesh raycast:

| Mode | Armor selected by `GetRandArmorByDir` |
|---|---|
| `0` | Front/left/back/right using the rectangle's orientation and attack direction |
| `1` | `RPG_BOTTOM` |
| `2` | `RPG_TOP` |

For mode `0`, `SRect::GetSide` divides direction into four 90-degree sectors,
centered on front, left, back, and right. It does not use rectangle aspect ratio,
the precise impact point, turret orientation, or a surface normal. Boundaries in
16-bit angle units are `8192`, `24576`, `40960`, and `57344`.

For ordinary explosions the attack direction points back toward the shooter,
using the **unscattered** intended endpoint. For guided missiles it is updated
from the negative horizontal velocity. Target orientation is read when the hit is
resolved, so turning before impact can change the selected side.

Constructor rules differ:

| Explosion class | Armor mode selection |
|---|---|
| `CCumulativeExpl` | SAM: bottom. Otherwise top-attack ATGM or shooter `GetZ()` above explosion Z: top. Otherwise directional. |
| `CBurstExpl` | SAM: bottom. Otherwise non-line trajectory or shooter `GetZ()` above explosion Z: top. Otherwise retain the caller's mode, normally directional. |

`IsLineTrajectory()` includes ordinary line and side-attack ATGM. Therefore, do
not assume every non-line direct-only shell automatically selects top armor; that
rule belongs to the burst constructor.

There is no impact-angle cosine correction or distance attenuation of piercing
in these hit-resolution functions.

Sources: [AIUnit.cpp](../Versions/Temporary/Engine/Sources/AILogic/AIUnit.cpp#L190),
[Geom.cpp](../Versions/Temporary/Engine/Sources/Misc/Geom.cpp#L365),
[Shell.cpp](../Versions/Temporary/Engine/Sources/AILogic/Shell.cpp#L579),
[Shell.h](../Versions/Temporary/Engine/Sources/AILogic/Shell.h#L663).

## 4. Random armor, piercing, damage, and cover

The rolled values are:

```text
rawPiercing = max(0, RandomInteger(Piercing - PiercingRandom,
	                              Piercing + PiercingRandom))
piercing = int(shot.weaponPiercing(rawPiercing))

rawDamage = max(0, RandomFloat(DamagePower - DamageRandom,
	                            DamagePower + DamageRandom))
damage = shot.weaponDamage(rawDamage)

cover = target.cover(1.0)
savedByCover = RandomFloat(0, 1) >= cover
```

Clipping happens **after** the raw roll. It is not equivalent to rolling uniformly
between `max(0, minimum)` and `maximum`: negative outcomes accumulate at zero.
The stat modifier runs after this clipping; it does not receive a second
nonnegative clamp here. Modified piercing is converted back to `int`.

`CExplosion::Init` snapshots the shooter's damage, piercing, `Area`, and `Area2`
modifiers at firing time. Later modifier changes do not change those saved
bonuses. Rolls themselves happen while resolving each target; there is no one
shared damage or penetration roll for all victims of an explosion. Target cover,
combat-box coefficient, and durability are read at resolution time.

Armor sources are type-specific:

- Mechanical units roll integers from `armors[side].nMin` to `.nMax`. Those runtime
  bounds are made by truncating the schema's floating `Min` and `Max` values.
- Ordinary infantry return `0` for random armor, without an armor RNG draw.
- The base HP-object stats use `defences[side].ArmorMin/ArmorMax`, notably for
  static objects. Do not read these as the mechanical unit's active armor array.

Integer random bounds are inclusive. `RandomCheck(min, max)` returns `min` for
reversed integer bounds. The float generator can include both endpoints as well;
probability descriptions here are approximate and ignore endpoint/rounding effects.

`Cover` is the chance of allowing damage through this gate, not the chance of
blocking it. Approximately:

| Effective `GetCover()` | Cover-save chance |
|---|---|
| `1.0` | 0% |
| `0.75` | 25% |
| `0.25` | 75% |
| `0.0` | 100% |

The exact comparison is `roll >= cover`, so even `1.0` can save on the float
generator's upper endpoint. No `[0, 1]` clamp is applied here. This is an abstract
random gate, separate from obstacle/visibility checks used before firing.

Sources: [prefix_RPGStats.h](../Versions/Temporary/Engine/Sources/Stats_B2_M1/prefix_RPGStats.h#L90),
[shell stat helpers](../Versions/Temporary/Engine/Sources/Stats_B2_M1/include_WeaponRPGStats_Shell.h),
[mechanical armor](../Versions/Temporary/Engine/Sources/Stats_B2_M1/include_MechUnitRPGStats.h#L12),
[armor conversion](../Versions/Temporary/Engine/Sources/Stats_B2_M1/include_MechUnitRPGStats_Armor.h),
[infantry armor](../Versions/Temporary/Engine/Sources/Stats_B2_M1/include_InfantryRPGStats.h),
[HP-object armor](../Versions/Temporary/Engine/Sources/Stats_B2_M1/include_HPObjectRPGStats.h),
[RandomGen.h](../Versions/Temporary/Engine/Sources/System/RandomGen.h#L63),
[cover](../Versions/Temporary/Engine/Sources/AILogic/AIUnit.cpp#L2908).

## 5. More accurate direct-hit pseudocode

This preserves the significant evaluation order and return meaning. `RollDamage`
and `RollPiercing` include the clipping and saved modifiers described above.

```text
TryDirectUnitHit(target, explosion, armorMode):
	fullRect = target.GetUnitRect()
	if target.IsInSolidPlace() or not DirectHeightPasses(target, explosion):
		return false
	if not fullRect.Contains(explosion.xy):
		return false

	armor = target.RollArmor(SelectArmorSide(armorMode, attackDirection, fullRect))
	combatRect = CompressForDirectHit(fullRect, target, shooter, armorMode)
	piercing = explosion.RollPiercing()  // Rolled even if combatRect will miss.

	if not combatRect.Contains(explosion.xy):
		result = MISS
	else if piercing < armor:
		result = REFLECT
	else if target.IsSavedByCover():    // No cover draw if armor already failed.
		result = REFLECT
	else:
		result = HIT

	if result == HIT or FirstShootCheat or ValidShooterIsTargetingTracks:
		target.TakeDamage(explosion.RollDamage())

	RecordHitEffect(result)
	return true                       // Processed full-rectangle hit, not HP loss!

ProcessBurstUnitHit(target, explosion):
	if not TryDirectUnitHit(target, explosion, armorMode):
		target.ProcessAreaDamage(explosion, armorMode, Area2, Area)
```

The last distinction is essential: once the full rectangle and height gates pass,
`ProcessCumulativeExpl` returns `true` even for a combat-box miss, an armor failure,
or a cover save. The explosive path then does **not** try splash on that target.
Likewise, a successful direct hit receives full rolled damage without an extra
splash contribution. A track-targeting override or cheat bypasses the smaller-box,
armor, and cover outcome but still requires the initial full-box/height gates.

Source: [AIUnit.cpp](../Versions/Temporary/Engine/Sources/AILogic/AIUnit.cpp#L166).

## 6. Splash: `Area` and `Area2` are not a falloff curve

`CBurstExpl::Explode` queries unit candidates using effective `Area2`, then calls
each target's direct-first burst handler. The ordinary unit splash fallback is:

```text
GenericUnitSplash(target, explosion):
	if target.IsInSolidPlace() or abs(target.GetVisZ() - explosion.z) >= 64:
		return false
	if target.IsSavedByCover():
		return false

	combatRect = target.GetUnitRect()
	if armorMode != BOTTOM:
		combatRect.Compress(target.GetRemissiveCoeff())
	if not combatRect.IntersectsCircle(explosion.xy, effectiveArea):
		return false

	armor = target.RollArmor(SelectArmorSide(armorMode, attackDirection, combatRect))
	if armor > ARMOR_FOR_AREA_DAMAGE:
		return false

	target.TakeDamage(explosion.RollDamage() * AREA_DAMAGE_COEFF)
	return true
```

Here the code uses `fZDiff < 2 * SConsts::TILE_SIZE`, hence strict `< 64`, unlike
the ordinary direct hit's `<= 64`. Cover is rolled before the circle/armor checks.
Intersection uses distance to the rectangle, not simply distance to unit center.
`Area2` selects candidates but is not the radius used by this generic unit test.

The constants are configurable:

| Setting | C++ default | Checked-in Current and Multiplayer profiles |
|---|---:|---:|
| `AI.Common.AreaDamageCoeff` | `0.2` | `0.3` |
| `AI.Common.ArmorForAreaDamage` | `10` | `300` |

For those profiles, eligible splash deals `30%` of a fresh modified damage roll
if the selected armor roll is `<= 300`. Shell piercing is irrelevant to this
particular check. Use the loaded profile/mod values when reasoning about a match.

`Area`, `Area2`, dispersion, and weapon range are converted from authored meters
to AI coordinates with a factor of `32` by the stats helpers.

Sources: [Shell.cpp](../Versions/Temporary/Engine/Sources/AILogic/Shell.cpp#L809),
[AIUnit.cpp](../Versions/Temporary/Engine/Sources/AILogic/AIUnit.cpp#L279),
[AIConsts.cpp](../Versions/Temporary/Engine/Sources/AILogic/AIConsts.cpp#L492),
[Current consts.cfg](../Versions/Current/Profiles/consts.cfg#L128),
[Multiplayer consts.cfg](../Versions/Multiplayer/profiles/consts.cfg#L114),
[weapon conversion](../Versions/Temporary/Engine/Sources/Stats_B2_M1/include_WeaponRPGStats.h),
[shell conversion](../Versions/Temporary/Engine/Sources/Stats_B2_M1/include_WeaponRPGStats_Shell.h).

### Infantry's outer-radius fallback

`CSoldier::ProcessAreaDamage` first requires the soldier outside a building and a
solid place, and `abs(GetVisZ() - explosion.z) <= stats.AABBHalfSize.y`. The use of
the footprint's **Y half-size** for this height test is what the source says.

It then tries the generic unit splash logic. If that returns false, a soldier
that is **free and not lying** also takes reduced damage when:

```text
squaredLength(soldier.GetCenter() - explosion.position) <= effectiveArea2^2
```

This is a 3D center-distance test. The fallback performs no new cover, armor, or
small-rectangle check. It can therefore damage a standing soldier even if the
generic splash attempt just saved that soldier through cover. Lying removes this
fallback; it does not itself remove the generic `Area` test or direct-hit test.

There is also a return-value oddity: after a successful generic splash, the soldier
override falls through and returns `false`. It does not apply damage twice. The
normal burst caller ignores that return value anyway; it only uses the return
from the preceding direct-hit attempt.

Source: [Soldier.cpp](../Versions/Temporary/Engine/Sources/AILogic/Soldier.cpp#L1329).

### Static objects and buildings

These do not use the unit cover/combat-rectangle algorithm:

- `CExistingObject::ProcessCumulativeExpl` accepts a footprint hit **or** an attack
  center within effective `Area`. It has its own height gate, rolls armor then
  piercing, and calls `TakeDamage` with full damage on penetration.
- Common static objects' fallback uses a bound-rectangle intersection with
  effective **`Area2`**, the fixed armor threshold, and reduced damage. It has no
  unit cover roll or small-AABB compression, and rejects fallen objects.
- The base existing-object area handler is empty; the common-static override
  should not be generalized to every static subclass.
- `CCommonStaticObject`, `CBuilding`, and bridge damage handlers require
  `bFromExplosion` to remove HP. The direct-only explosion path passes `false`;
  the burst path passes `true`, including its full-damage direct attempt. Thus a
  reported penetration on these objects need not imply an HP change.
- Buildings add destruction/occupant logic and special handling for key buildings.

The normal explosion loops only enumerate static objects when the explosion is
within one AI tile of terrain height. Inside the static direct handler, the height
test is `impact.z - terrainHeightAtObject <= 2 * AI_TILE_SIZE`, without `abs`.

Sources: [StaticObject.cpp](../Versions/Temporary/Engine/Sources/AILogic/StaticObject.cpp#L213),
[static splash and HP](../Versions/Temporary/Engine/Sources/AILogic/StaticObject.cpp#L727),
[StaticObject.h](../Versions/Temporary/Engine/Sources/AILogic/StaticObject.h#L297),
[Building.cpp](../Versions/Temporary/Engine/Sources/AILogic/Building.cpp#L1680),
[Bridge.cpp](../Versions/Temporary/Engine/Sources/AILogic/Bridge.cpp#L510).

## 7. Final damage can still be modified

Ignoring cheats and subclass-specific effects, normal unit HP loss is:

```text
rolledDamage = shot.weaponDamage(max(0, RandomFloat(DamagePower - DamageRandom,
	                                                DamagePower + DamageRandom)))
incomingDamage = rolledDamage * (isSplash ? AREA_DAMAGE_COEFF : 1)
hpLoss = shooterPartyIsNeutral ? incomingDamage : target.durability(incomingDamage)
HP -= hpLoss
```

Durability uses the same additive/multiplicative modifier function, so it is not
necessarily just a percentage. No armor subtraction follows penetration.

`TakeDamage` also handles death, statistics, repair requests, and notifications.
Tank track-targeting can scale damage before the base handler and break tracks;
ordinary tank damage can roll `BrokeTrackProbability`. Soldiers in the parachuting
state ignore `TakeDamage`. Immortality and first-shot cheats further alter HP or
death behavior. These exceptions are why an `EHT_HIT` notification alone is not a
guarantee of a particular HP loss.

Sources: [AIUnit.cpp](../Versions/Temporary/Engine/Sources/AILogic/AIUnit.cpp#L1193),
[Technics.cpp](../Versions/Temporary/Engine/Sources/AILogic/Technics.cpp#L619),
[Soldier.cpp](../Versions/Temporary/Engine/Sources/AILogic/Soldier.cpp#L1230).

## Property reference

| Property | Actual role |
|---|---|
| `AABBHalfSize`, `AABBCenter` | Full oriented footprint, then the basis for the combat footprint. Also used in some special height/collision tests. |
| `SmallAABBCoeff` and its modifier | Scales both combat-footprint dimensions. Not a separate probability roll. |
| `Cover` modifier | Random rejection gate; lower effective values protect more. Standing-infantry splash fallback can bypass a save. |
| Mechanical `armors[]` / HP-object `defences[]` | Random integer armor for the selected side. Infantry override armor to zero. |
| `Piercing`, `PiercingRandom`, weapon-piercing modifier | Direct-hit threshold against armor, with equality passing. No damage scaling from surplus piercing. |
| `Dispersion` and weapon-dispersion modifier | Distance-scaled scatter before ordinary impact resolution, with trajectory-dependent ellipse ratio. |
| `DamagePower`, `DamageRandom`, weapon-damage modifier | Nonnegative raw damage roll, then the shot's saved modifier. |
| `Area` and its modifier | Generic unit splash circle; also expands the static-object direct/full-damage test. |
| `Area2` and its modifier | Burst candidate radius, standing-infantry fallback radius, and common-static splash radius. |
| `Durability` modifier | Changes incoming unit damage after the hit/splash calculation, except neutral-party damage. |

## Determinism and scope notes

The `CCumulativeExpl` and `CBurstExpl` target lists are sorted by descending unique
ID and deduplicated before processing. Random draws use simulation `NRandom`.
Preserving short-circuit behavior matters: direct armor and piercing are rolled
before the small-box outcome, cover only after penetration, and damage only when
the damage branch is entered. Generic splash instead rolls cover before armor.

`CCumulativeExpl` excludes its shooter and stops processing further infantry once
one infantry full-rectangle hit has been processed, even if it reflected or missed
the combat rectangle. Burst enumeration uses `ANY_PARTY` and does not exclude the
shooter from damage processing. There is no friendly-only filter in these impact
loops.

Smoke damage type exits into smoke-screen creation. Flamethrowers create a series
of burst explosions; bombs, guided missiles, mines, scripted damage, and fighting
inside stormable objects have their own entry paths. This document explains the
shared ordinary projectile/explosion resolver and its important overrides; it
does not claim every call to `TakeDamage` passes through penetration and cover.

Validation was static: call sites, overrides, helper implementations, and shipped
configuration were checked. No game build, runtime balance test, or replay was
run for this documentation-only investigation.
