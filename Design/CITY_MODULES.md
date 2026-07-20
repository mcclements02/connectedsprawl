# City Layout Modules

Small, single-purpose passes that shape the city. Each is independently
runnable, idempotent, and reports what it did to `Saved/*.txt` (UE 5.8's
python commandlet swallows `LogPython` output, so a sidecar report is the only
reliable channel).

Every module derives its numbers from one place — the grid — rather than
carrying its own copy of the layout.

## The grid is the single source of truth

`Source/ConnectedSprawl/Public/World/SprawlCityGridSubsystem.h` defines the
city. Nothing else should hard-code layout numbers.

| Constant | Value | Meaning |
|---|---|---|
| `BlockSize` | 2000 | Block footprint |
| `LaneWidth` | 350 | One travel lane |
| `LanesPerDirection` | 1 | Lanes each way (two-way street) |
| `ParkingBayWidth` | 250 | Parallel-parking bay |
| `RoadWidth` | 1200 | Kerb to kerb, **derived** from the lane layout |
| `Step` | 3200 | Block pitch (`BlockSize + RoadWidth`) |
| `Span` | 22400 | Full city extent |
| `LaneOffset` | 175 | Centre of lane 0 |
| `ParkingOffset` | 475 | Centre of the parking bay |

A street cross-section, measured out from the centreline:

```
 0 ....... 350 ..... 600 ......... 1000
 |  lane    |  bay    | pavement   | building line
                 kerb ^
```

Changing `LanesPerDirection` re-derives `RoadWidth`, `Step`, `Span`, the
boundary, the stop-line distances and every AI approach distance. The markings
follow too: lane dividers are painted only between travel lanes, so a two-way
street gets a centreline and parking edge lines but no divider.

`LaneCenter(road, heading, lane)` and `ParkingLaneCenter(road, heading)` are
the only correct ways to ask where a vehicle belongs.

## Modules

Run each with:

```sh
UnrealEditor-Cmd ConnectedSprawl.uproject -run=pythonscript \
  -script="Content/Python/<module>.py" -unattended -nosplash -nullrhi \
  -stdout -DisablePlugins=Fab
```

**`-nullrhi` must be dropped for any module that spawns actors** — actor
spawning asserts under the null RHI.

| Module | Spawns? | Purpose |
|---|---|---|
| `audit_world_placement.py` | no | Reports tilted, sunken, floating and character-height props |
| `repair_world_placement.py` | no | Stands props upright and lays road paint flat |
| `audit_street_geometry.py` | no | Reports what forms the road, kerbs and block surfaces |
| `audit_vehicle_grounding.py` | no | Reports how far each vehicle sits into the ground |
| `expand_streets.py` | no | Re-spaces the city onto a wider grid |
| `fix_kerbside_anchoring.py` | no | Returns kerbside objects to their block |
| `park_cars_in_bays.py` | no | Parks every car in a clear, marked bay |
| `city_surfaces.py` | **yes** | Lays pavement and grass block surfaces |
| `fix_building_placement.py` | no | Holds buildings to the building line |

### `expand_streets.py`

Re-spaces the city when the grid changes, instead of regenerating it. Set
`OLD_ROAD` / `NEW_ROAD`; the block size never changes, so only the gaps open
or close. **Block interiors keep their exact internal layout.**

Anchors are chosen by **ownership, not proximity**:

* within `BlockSize/2` of a block centre → block-owned, keeps its exact offset
  from that block centre
* otherwise → road-owned, its offset from the road centre scales by
  `NEW_ROAD / OLD_ROAD`

*Why that distinction matters.* An earlier version used the *nearest* anchor.
Anchors alternate every half step, so anything in the outer ~350cm of a block
is nearer the road centre than its own block centre — exactly where kerbside
trees, hydrants and shopfronts live. They scaled with the road and ended up
standing in the carriageway. Ownership anchoring makes that impossible: the
subsequent 1900 → 1200 pass moved 2379 block-owned actors with **zero**
buildings left in the street.

### `fix_kerbside_anchoring.py` (historical, disabled)

The exact inverse of that mistake, used once to repair the 600 → 1900 pass.
`ENABLED = False` — it is not needed now that anchoring is ownership-based,
and its 280–700cm band would straddle the current 1200cm carriageway and shove
legitimate road furniture onto the pavement. Kept for the record only.

### `park_cars_in_bays.py`

Assigns each car a bay slot along a street, testing every candidate against
tree, prop and building footprints plus the cars already placed. Cars that
cannot be given a clear bay are reported, never dumped somewhere invalid.

*Why it was needed:* `ParkingOffset` used to be 410 while the road half-width
was 300 — "parking" placed cars **past the kerb**, which is why trees grew
through them.

### `city_surfaces.py`

Lays one surface slab per block, flush with the kerb: grass on park blocks,
pavement everywhere else. The gaps are deliberately left bare so the ground
plane reads as asphalt. Collision is off — the slab is dressing, and the
ground plane already carries collision.

### `fix_building_placement.py`

Holds structures to the building line (1350cm from a road centre) and keeps
the carriageway clear. Awnings and signs are permitted to overhang the
pavement — that is their purpose — so only supporting structure is held back.
Set `APPLY = False` to audit without moving anything; **inspect the report
before applying**, since the correction moves real geometry.

## Order of operations

After any grid change:

```
expand_streets.py  ->  fix_kerbside_anchoring.py  ->  park_cars_in_bays.py
                   ->  city_surfaces.py           ->  fix_building_placement.py
```

Then re-validate with the in-game audits (`-SprawlTrafficAudit`,
`-SprawlVisualAudit`, `-SprawlLocomotionAudit`, `-SprawlProgressionAudit`,
`-SprawlCarjackAudit`).

## Known drift

`build_city.py`, `add_city_detail.py`, `realkit_apply.py` and the `dw_*.py`
generation scripts still hard-code `BLOCK/ROAD/STEP = 2000/600/2600`. They are
**no longer consistent with the live grid** — re-running any of them rebuilds
at the old spacing. Update their constants before use, or treat
`expand_streets.py` as the migration step afterwards.
