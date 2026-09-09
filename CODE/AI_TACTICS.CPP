// Computer base management. Orders use existing missions/timers so saves retain their state.
#include "function.h"

namespace {
bool Present(TechnoClass const * object)
{
	return object && !object->IsInLimbo && object->Strength > 0;
}

bool Known_Enemy(HouseClass const * house, TechnoClass const * object)
{
	return Present(object) && !house->Is_Ally(object) && object->IsDiscoveredByComputer && object->Cloak != CLOAKED;
}

int Effectiveness(TechnoTypeClass const * type, ArmorType armor, bool air)
{
	int best = 0;
	WeaponTypeClass const * weapons[] = {type->PrimaryWeapon, type->SecondaryWeapon};
	for (int i = 0; i < 2; ++i) {
		WeaponTypeClass const * weapon = weapons[i];
		if (!weapon || !weapon->WarheadPtr || weapon->Attack <= 0) continue;
		if (air && (!weapon->Bullet || !weapon->Bullet->IsAntiAircraft)) continue;
		if (!air && weapon->Bullet && !weapon->Bullet->IsAntiGround) continue;
		int damage = weapon->Attack * weapon->WarheadPtr->Modifier[armor];
		best = max(best, damage * max(1, weapon->Burst) * 100 / max(1, weapon->ROF));
	}
	return best;
}

CELL Offset_Cell(COORDINATE origin, int dx, int dy)
{
	int x = max(Map.MapCellX, min(Map.MapCellX + Map.MapCellWidth - 1, Cell_X(Coord_Cell(origin)) + dx));
	int y = max(Map.MapCellY, min(Map.MapCellY + Map.MapCellHeight - 1, Cell_Y(Coord_Cell(origin)) + dy));
	return XY_Cell(x, y);
}

CELL Reachable_Post(FootClass const * unit, CELL desired)
{
	TechnoTypeClass const * type = unit->Techno_Type_Class();
	int zone = Map[unit->Center_Coord()].Zones[type->MZone];
	if (unit->Mission == MISSION_GUARD_AREA && Is_Target_Cell(unit->ArchiveTarget) &&
		Distance(As_Coord(unit->ArchiveTarget), Cell_Coord(desired)) <= CELL_LEPTON_W) {
		CELL previous = Coord_Cell(As_Coord(unit->ArchiveTarget));
		if (unit->Distance(Cell_Coord(previous)) <= CELL_LEPTON_W ||
			Map[previous].Is_Clear_To_Move(type->Speed, true, false, zone, type->MZone)) return previous;
	}
	return Map.Nearby_Location(desired, type->Speed, zone, type->MZone);
}

void Hold_Post(FootClass * unit, CELL cell)
{
	if (!cell) return;
	TARGET post = ::As_Target(cell);
	// Keep an existing route. Reissuing it every think would repeatedly restart pathfinding.
	if (unit->Mission == MISSION_GUARD_AREA && unit->ArchiveTarget == post) return;
	unit->Assign_Mission(MISSION_GUARD_AREA);
	unit->ArchiveTarget = post;
	unit->Assign_Target(TARGET_NONE);
	unit->Assign_Destination(post);
	unit->Commence();
}
}

ZoneType HouseClass::AI_Front_Zone(void) const
{
	HouseClass const * enemy = Enemy == HOUSE_NONE ? NULL : As_Pointer(Enemy);
	if (!Center || !enemy || !enemy->Center) return ZONE_NONE;
	int dx = Coord_X(enemy->Center) - Coord_X(Center);
	int dy = Coord_Y(enemy->Center) - Coord_Y(Center);
	if (abs(dx) > abs(dy)) return dx > 0 ? ZONE_EAST : ZONE_WEST;
	return dy > 0 ? ZONE_SOUTH : ZONE_NORTH;
}

int HouseClass::AI_Production_Score(TechnoTypeClass const * type, int quantity) const
{
	if (!Can_Build(type, ActLike, true) || type->Cost_Of() > Available_Money()) return 0;
	// Difficulty is the computer's handicap: DIFF_HARD is the player's Easy opponent.
	bool easy = Difficulty == DIFF_HARD;
	int army = CurInfantry + CurUnits;
	if (easy && army >= max(8, CurBuildings)) return 0;
	// Reserve cash for a selected economic recovery project instead of starving it with troops.
	if (State != STATE_ATTACKED && BQuantity[STRUCT_CONST] > 0 && BuildStructure != STRUCT_NONE &&
		Available_Money() < BuildingTypeClass::As_Reference(BuildStructure).Cost_Of() + 300) return 0;
	if (State != STATE_ATTACKED && army >= max(12, CurBuildings * (Difficulty == DIFF_EASY ? 4 : 3)) &&
		Available_Money() < Rule.InfantryReserve) return 0;
	HouseClass const * enemy = Enemy == HOUSE_NONE ? NULL : As_Pointer(Enemy);
	int infantry = enemy ? max(1, enemy->CurInfantry) : 3;
	int armor = enemy ? max(1, enemy->CurUnits) : 2;
	int air = enemy ? enemy->CurAircraft : 0;
	int score = infantry * Effectiveness(type, ARMOR_NONE, false) +
		armor * Effectiveness(type, ARMOR_STEEL, false) + air * Effectiveness(type, ARMOR_ALUMINUM, true);
	if (score <= 0) return 0;
	// Diminishing returns maintain a mixed force; costs favour sustainable replacements.
	return max(1, min(1000, score * 100 / max(100, type->Cost_Of()) / (max(0, quantity) + 1)));
}

int HouseClass::Tactical_AI(void)
{
	// These are opponent handicaps, reversed from the difficulty shown to the player.
	bool easy = Difficulty == DIFF_HARD;
	bool hard = Difficulty == DIFF_EASY;
	int think = TICKS_PER_SECOND * (easy ? 5 : hard ? 1 : 2);
	if (IsHuman || IsDefeated || !IsBaseBuilding) return think;
	if (!Center) Recalc_Center();
	if (!Center) {
		if (State != STATE_ENDGAME) { State = STATE_ENDGAME; Do_All_To_Hunt(); }
		return think;
	}

	DynamicVectorClass<FootClass *> troops;
	DynamicVectorClass<TechnoClass *> enemies;
	DynamicVectorClass<TechnoClass *> assets;
	for (int i = 0; i < Buildings.Count(); ++i) {
		BuildingClass * building = Buildings.Ptr(i);
		if (Present(building) && building->House == this && !building->Class->IsInsignificant) assets.Add(building);
		else if (Known_Enemy(this, building) && !building->Class->IsInsignificant) enemies.Add(building);
	}
	for (int i = 0; i < Infantry.Count() + Units.Count(); ++i) {
		FootClass * unit = i < Infantry.Count() ? (FootClass *)Infantry.Ptr(i) : (FootClass *)Units.Ptr(i - Infantry.Count());
		if (!Present(unit)) continue;
		if (unit->House == this && unit->What_Am_I() == RTTI_UNIT && *((UnitClass *)unit) == UNIT_HARVESTER) assets.Add(unit);
		if (unit->House == this && unit->Is_Weapon_Equipped() && !unit->Team.Is_Valid()) troops.Add(unit);
		else if (Known_Enemy(this, unit)) enemies.Add(unit);
	}
	for (int i = 0; i < Aircraft.Count(); ++i) {
		if (Known_Enemy(this, Aircraft.Ptr(i))) enemies.Add(Aircraft.Ptr(i));
	}

	TechnoClass * objective = NULL;
	TechnoClass * incursion = NULL;
	int best = -1;

	for (unsigned i = 0; i < enemies.Count(); ++i) {
		TechnoClass * enemy = enemies[i];
		bool nearby = false;
		for (unsigned j = 0; j < assets.Count(); ++j) {
			if (enemy->Distance(assets[j]) < 8 * CELL_LEPTON_W) { nearby = true; break; }
		}
		if (nearby && enemy->Is_Weapon_Equipped()) {
			if (!incursion || enemy->Distance(Center) < incursion->Distance(Center)) incursion = enemy;
		}
		if (enemy->What_Am_I() == RTTI_AIRCRAFT) continue;
		int value = 100;
		if (enemy->What_Am_I() == RTTI_BUILDING) {
			StructType type = ((BuildingClass *)enemy)->Class->Type;
			if (type == STRUCT_REFINERY || type == STRUCT_POWER || type == STRUCT_ADVANCED_POWER) value = 400;
			else if (type == STRUCT_WEAP || type == STRUCT_BARRACKS || type == STRUCT_TENT) value = 300;
		} else if (enemy->What_Am_I() == RTTI_UNIT && *((UnitClass *)enemy) == UNIT_HARVESTER) value = 450;
		int protection = 0;
		for (unsigned j = 0; j < enemies.Count(); ++j) {
			if (enemies[j]->Is_Weapon_Equipped() && enemies[j]->Distance(enemy) < 6 * CELL_LEPTON_W) protection += max(1, enemies[j]->Risk());
		}
		value = value * 100 / (100 + enemy->Distance(Center) / CELL_LEPTON_W * 5 + protection);
		if (value > best) { best = value; objective = enemy; }
	}
	if (objective) Enemy = objective->Owner();
	State = incursion ? STATE_ATTACKED : Available_Money() < 100 ? STATE_BROKE : STATE_BUILDUP;

	int dx = 0, dy = -1;
	TechnoClass * facing = incursion ? incursion : objective;
	if (facing) {
		dx = Coord_X(facing->Center_Coord()) - Coord_X(Center);
		dy = Coord_Y(facing->Center_Coord()) - Coord_Y(Center);
		int scale = max(1, max(abs(dx), abs(dy)));
		dx = dx * 8 / scale; dy = dy * 8 / scale;
	} else { dy = -8; }
	int radius = min(8, max(3, Radius / CELL_LEPTON_W + 2));
	CELL rally = Offset_Cell(Center, dx * radius / 8, dy * radius / 8);
	CELL rear = Offset_Cell(Center, -dx * 3 / 8, -dy * 3 / 8);

	DynamicVectorClass<FootClass *> ready;
	int fighting = 0;
	bool scouting = false;
	for (unsigned i = 0; i < troops.Count(); ++i) {
		FootClass * unit = troops[i];
		TechnoTypeClass const * type = unit->Techno_Type_Class();
		if (unit->Mission == MISSION_MOVE && unit->ArchiveTarget == unit->NavCom) scouting = true;
		bool idle = unit->Mission == MISSION_GUARD || unit->Mission == MISSION_GUARD_AREA || unit->Mission == MISSION_STICKY;
		bool attacking = unit->Mission == MISSION_ATTACK || unit->Mission == MISSION_HUNT || unit->Mission == MISSION_RESCUE;
		if (!idle && !attacking) continue; // Transports, scripted movement, harvesting and team orders remain intact.
		int support = 0, danger = 0;
		if (attacking) {
			for (int j = 0; j < troops.Count(); ++j) {
				if (troops[j]->Distance(unit) < 6 * CELL_LEPTON_W) support += max(1, troops[j]->Risk());
			}
			for (int j = 0; j < enemies.Count(); ++j) {
				if (enemies[j]->Is_Weapon_Equipped() && enemies[j]->Distance(unit) < 6 * CELL_LEPTON_W) danger += max(1, enemies[j]->Risk());
			}
		}
		if (unit->Strength * 3 < type->MaxStrength || (!incursion && danger > support * 2)) {
			if (unit->What_Am_I() == RTTI_UNIT && unit->Strength * 3 < type->MaxStrength &&
				BQuantity[STRUCT_REPAIR] > 0 && Available_Money() > 100) {
				unit->Assign_Target(TARGET_NONE); unit->Assign_Mission(MISSION_REPAIR); unit->Commence();
			} else Hold_Post(unit, Reachable_Post(unit, rear));
			continue;
		}
		if (incursion) {
			// Easy keeps most of its garrison at their posts instead of mobilizing everyone.
			if (easy && i >= 4 && !attacking && !Target_Legal(unit->TarCom)) {
				COORDINATE anchor = assets.Count() == 0 ? Center : assets[i % assets.Count()]->Center_Coord();
				Hold_Post(unit, Reachable_Post(unit, Offset_Cell(anchor, (int)i % 3 - 1, 2)));
				continue;
			}
			// A raid on remote harvesting should not empty the main base.
			if (incursion->Distance(Center) > Radius + 8 * CELL_LEPTON_W && i % 3 == 0) {
				Hold_Post(unit, Reachable_Post(unit, Offset_Cell(Center, (int)i % 3 - 1, 2)));
				continue;
			}
			TechnoClass * target = NULL;
			int score = 0;
			for (unsigned j = 0; j < enemies.Count(); ++j) {
				TechnoClass * enemy = enemies[j];
				if (enemy->Distance(incursion) > 10 * CELL_LEPTON_W) continue;
				bool air = enemy->What_Am_I() == RTTI_AIRCRAFT;
				if (!air && Map[unit->Center_Coord()].Zones[type->MZone] != Map[enemy->Center_Coord()].Zones[type->MZone]) continue;
				int effect = Effectiveness(type, enemy->Techno_Type_Class()->Armor, air);
				int value = effect > 0 ? max(1, effect * 100 / (100 + unit->Distance(enemy))) : 0;
				if (value > score) { score = value; target = enemy; }
			}
			if (target && unit->TarCom != target->As_Target()) {
				unit->Assign_Mission(MISSION_ATTACK); unit->Assign_Target(target->As_Target());
				unit->Assign_Destination(TARGET_NONE); unit->Commence();
			}
			continue;
		}
		if (attacking && Target_Legal(unit->TarCom)) { ++fighting; continue; }
		if (idle && Target_Legal(unit->TarCom)) continue;
		ready.Add(unit);
	}

	// Keep a home reserve. Assemble the remainder before releasing a coordinated attack.
	unsigned reserve = max(3, (int)ready.Count() / (easy ? 2 : hard ? 4 : 3));
	unsigned waveend = easy ? min((unsigned)ready.Count(), reserve + 4) : ready.Count();
	int minimum = easy ? 3 : 4;
	bool assemble = !incursion && objective && Attack == 0 && ready.Count() >= reserve + minimum && fighting == 0 &&
		(!easy || Frame >= TICKS_PER_MINUTE * 3);
	int gathered = 0, strength = 0, assaultsize = 0;
	for (unsigned i = 0; i < ready.Count(); ++i) {
		FootClass * unit = ready[i];
		if (assemble && i >= reserve && i < waveend && Effectiveness(unit->Techno_Type_Class(), objective->Techno_Type_Class()->Armor, false) > 0) {
			CELL post = Reachable_Post(unit, Offset_Cell(Cell_Coord(rally), (int)(i % 3) - 1, (int)(i / 3 % 3) - 1));
			if (post) {
				++assaultsize;
				Hold_Post(unit, post);
				if (unit->Distance(Cell_Coord(rally)) <= 3 * CELL_LEPTON_W) ++gathered;
				strength += max(1, unit->Risk());
			}
		} else {
			COORDINATE anchor = assets.Count() == 0 ? Center : assets[i % assets.Count()]->Center_Coord();
			// Cover separate approaches to separate assets, with space between firing positions.
			CELL post = Offset_Cell(anchor, dx * 2 / 8 + ((int)i % 3 - 1) * 2, dy * 2 / 8 + ((int)i / 3 % 3 - 1) * 2);
			Hold_Post(unit, Reachable_Post(unit, post));
		}
	}
	if (assemble && assaultsize >= minimum && gathered * 3 >= assaultsize * 2) {
		int opposition = 0;
		for (unsigned i = 0; i < enemies.Count(); ++i) {
			if (enemies[i]->Is_Weapon_Equipped() && enemies[i]->Distance(objective) < 7 * CELL_LEPTON_W) opposition += max(1, enemies[i]->Risk());
		}
		if (strength >= opposition * 5 / 4) {
			for (unsigned i = reserve; i < waveend; ++i) {
				FootClass * unit = ready[i];
				if (unit->Distance(Cell_Coord(rally)) > 4 * CELL_LEPTON_W ||
					Effectiveness(unit->Techno_Type_Class(), objective->Techno_Type_Class()->Armor, false) == 0) continue;
				unit->Assign_Mission(MISSION_ATTACK); unit->Assign_Target(objective->As_Target());
				unit->Assign_Destination(TARGET_NONE); unit->Commence();
			}
			for (int i = 0; !easy && i < Aircraft.Count(); ++i) {
				AircraftClass * aircraft = Aircraft.Ptr(i);
				if (Present(aircraft) && aircraft->House == this && !aircraft->Team.Is_Valid() && aircraft->Ammo > 0 &&
					(aircraft->Mission == MISSION_GUARD || aircraft->Mission == MISSION_GUARD_AREA) &&
					Effectiveness(aircraft->Techno_Type_Class(), objective->Techno_Type_Class()->Armor, false) > 0) {
					aircraft->Assign_Mission(MISSION_ATTACK); aircraft->Assign_Target(objective->As_Target()); aircraft->Commence();
				}
			}
			Attack = TICKS_PER_SECOND * (easy ? 120 : hard ? 15 : 30);
		}
	}
	// A single scout searches legal map positions when no opponent has been discovered.
	if (!scouting && !incursion && !objective && ready.Count() > 3) {
		FootClass * scout = ready[ready.Count()-1];
		int sector = (Frame / (TICKS_PER_SECOND * 30) + (int)Class->House) % 4;
		int sx = sector & 1 ? Map.MapCellX + Map.MapCellWidth - 4 : Map.MapCellX + 3;
		int sy = sector & 2 ? Map.MapCellY + Map.MapCellHeight - 4 : Map.MapCellY + 3;
		CELL post = Reachable_Post(scout, XY_Cell(sx, sy));
		if (post) { scout->ArchiveTarget = ::As_Target(post); scout->Assign_Mission(MISSION_MOVE); scout->Assign_Destination(::As_Target(post)); scout->Commence(); }
	}
	return think;
}
