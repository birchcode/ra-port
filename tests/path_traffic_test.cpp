// Real path search and route selection on a synthetic narrow crossing.
#include "function.h"
#include <stdio.h>
#include <stdlib.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x); exit(1); } } while (0)
class CrossingUnit : public UnitClass {
public:
 static MoveType traffic;
 static int gap;
 static bool enclosed;
 virtual bool Mark(MarkType) { return true; }
 CrossingUnit(UnitType type=UNIT_HTANK) : UnitClass(type,HOUSE_GREECE) {}
 virtual MoveType Can_Enter_Cell(CELL cell, FacingType = FACING_NONE) const {
  if(enclosed) return MOVE_NO;
  int x=Cell_X(cell), y=Cell_Y(cell);
  if(x<40 || x>90 || y<40 || y>90) return MOVE_NO;
  if(x==64 && y>=gap && y<=80) return y==64 ? traffic : MOVE_NO;
  return MOVE_OK;
 }
};
class WaitingInfantry : public InfantryClass {
public:
 WaitingInfantry() : InfantryClass(INFANTRY_E1,HOUSE_GREECE) {}
 virtual bool Mark(MarkType) { return true; }
 virtual MoveType Can_Enter_Cell(CELL cell, FacingType = FACING_NONE) const {
  return cell==XY_Cell(64,64) ? MOVE_MOVING_BLOCK : MOVE_OK;
 }
};
MoveType CrossingUnit::traffic=MOVE_MOVING_BLOCK;
int CrossingUnit::gap=49;
bool CrossingUnit::enclosed=false;
int main() {
 setbuf(stdout,NULL);
 HouseTypes.Set_Heap(HOUSE_COUNT); HouseTypeClass::Init_Heap();
 UnitTypes.Set_Heap(UNIT_COUNT); UnitTypeClass::Init_Heap();
 AircraftTypes.Set_Heap(AIRCRAFT_COUNT); AircraftTypeClass::Init_Heap();
 InfantryTypes.Set_Heap(INFANTRY_COUNT); InfantryTypeClass::Init_Heap();
 Houses.Set_Heap(HOUSE_MAX); Units.Set_Heap(30); Aircraft.Set_Heap(10); Infantry.Set_Heap(20);
 HouseClass::One_Time(); Map.Size=MAP_CELL_TOTAL; Map.Alloc_Cells(); Map.Init_Cells();
 Map.CellRedraw.Resize(MAP_CELL_TOTAL);
 Map.MapCellX=Map.MapCellY=1; Map.MapCellWidth=Map.MapCellHeight=126;
 static char strings[4000]={0}; SystemStrings=DebugStrings=strings;
 Session.Type=GAME_NORMAL; ScenarioInit=1;
 HouseClass *house=new HouseClass(HOUSE_GREECE); house->IsHuman=true; PlayerPtr=house;
 CrossingUnit *unit=new CrossingUnit();
 unit->Coord=Cell_Coord(XY_Cell(60,64)); unit->IsInLimbo=false;
 unit->Strength=100;
 unit->Mission=MISSION_MOVE; unit->NavCom=As_Target(XY_Cell(68,64));
 unit->PrimaryFacing.Set(DIR_E);
 for(int mode=0;mode<10;++mode) {
  unit->PathThreshhold=MOVE_CLOAK;
  unit->traffic=mode==1 ? MOVE_TEMP : mode==2 ? MOVE_NO : mode==5 ? MOVE_DESTROYABLE : mode==6 ? MOVE_OK : MOVE_MOVING_BLOCK;
  house->IsHuman=mode!=3;
  unit->Mission=mode==7 ? MISSION_ATTACK : mode==8 ? MISSION_GUARD : MISSION_MOVE;
  unit->MissionQueue=mode==8 ? MISSION_MOVE : mode==9 ? MISSION_ATTACK : MISSION_NONE;
  unit->gap=mode==4 ? 64 : 49;
  CHECK(unit->Basic_Path());
  CELL end=XY_Cell(60,64); bool crossed=false;
  for(int i=0;i<(int)ARRAY_SIZE(unit->Path) && unit->Path[i]!=FACING_NONE;++i) {
   end=Adjacent_Cell(end,unit->Path[i]);
   if(end==XY_Cell(64,64)) crossed=true;
  }
  CHECK(crossed==(mode==0 || mode==6 || mode==8));
  printf("crossing case %d passed (bridge=%d)\n",mode,crossed);
 }
 unit->MissionQueue=MISSION_NONE;
 // Persistent traffic must eventually permit the clear detour.
 house->IsHuman=true; unit->Mission=MISSION_MOVE; unit->traffic=MOVE_MOVING_BLOCK; unit->gap=49;
 unit->Coord=Cell_Coord(XY_Cell(63,64)); unit->Path[0]=FACING_E; unit->Path[1]=FACING_NONE;
 unit->PrimaryFacing.Set(DIR_E); unit->PathDelay=10;
 unit->Start_Of_Move(); CHECK(unit->Path[0]==FACING_E);
 unit->PathDelay=0; unit->Start_Of_Move();
 CHECK(unit->Path[0]==FACING_NONE && unit->TryTryAgain==0);
 CHECK(Target_Legal(unit->NavCom));
 CHECK(unit->Basic_Path()); CHECK(unit->Path[0]!=FACING_E);
 puts("traffic timeout preserves destination and takes detour");
 // A moving queue must earn a fresh wait window after each advance.
 unit->Path[0]=FACING_E; unit->Path[1]=FACING_NONE; unit->TryTryAgain=unit->PATH_RETRY;
 unit->PathDelay=0; unit->Start_Of_Move();
 CHECK(unit->TryTryAgain==unit->PATH_WAITING && unit->PathDelay>=3*TICKS_PER_SECOND);
 unit->traffic=MOVE_OK;
 Ground[LAND_CLEAR].Cost[SPEED_TRACK]=1;
 unit->Start_Of_Move(); CHECK(unit->IsDriving && unit->TryTryAgain==unit->PATH_RETRY && unit->PathDelay==0);
 unit->Stop_Driver(); unit->traffic=MOVE_MOVING_BLOCK;
 puts("traffic progress resets wait window");

 // Formation traffic: a faster member asks a slower stationary member for a
 // temporary side step, while a slower member leaves the blocker in place.
 CrossingUnit *fast=new CrossingUnit(UNIT_JEEP); CrossingUnit *slow=new CrossingUnit(UNIT_HTANK);
 fast->Strength=slow->Strength=100; fast->IsInLimbo=slow->IsInLimbo=false;
 fast->Coord=Cell_Coord(XY_Cell(62,64)); slow->Coord=Cell_Coord(XY_Cell(63,64));
 fast->IsFormationMove=slow->IsFormationMove=true; fast->Group=slow->Group=7;
 fast->Mission=MISSION_MOVE; slow->Mission=MISSION_GUARD;
 const_cast<UnitTypeClass &>(*fast->Class).MaxSpeed=(MPHType)10;
 const_cast<UnitTypeClass &>(*slow->Class).MaxSpeed=(MPHType)5;
 Map[slow->Coord].OccupierPtr=slow;
 CHECK(fast->Try_Formation_Yield(Coord_Cell(slow->Coord)));
 CHECK(Target_Legal(slow->NavCom));
 Map[slow->Coord].OccupierPtr=NULL;
 puts("formation traffic lets the faster member request a temporary yield");

 // Local defense: an idle nearby unit responds to an attack on its ally,
 // while the victim's existing movement/target order is left alone.
 HouseClass *enemy_house=new HouseClass(HOUSE_USSR);
 UnitClass *victim=new UnitClass(UNIT_HTANK,HOUSE_GREECE);
 UnitClass *defender=new UnitClass(UNIT_HTANK,HOUSE_GREECE);
 UnitClass *attacker=new UnitClass(UNIT_HTANK,HOUSE_USSR);
 victim->Strength=defender->Strength=attacker->Strength=100;
 victim->IsInLimbo=defender->IsInLimbo=attacker->IsInLimbo=false;
 victim->Coord=Cell_Coord(XY_Cell(70,70)); defender->Coord=Cell_Coord(XY_Cell(72,70));
 attacker->Coord=Cell_Coord(XY_Cell(74,70));
 victim->Mission=MISSION_MOVE; victim->NavCom=As_Target(XY_Cell(80,70));
 defender->Mission=MISSION_GUARD;
 WeaponTypeClass defense_weapon("LocalDefense");
 defense_weapon.Attack=10;
 const_cast<UnitTypeClass &>(*defender->Class).PrimaryWeapon=&defense_weapon;
 victim->Alert_Nearby_Allies(attacker);
 CHECK(defender->MissionQueue==MISSION_ATTACK);
 CHECK(victim->Mission==MISSION_MOVE && victim->NavCom==As_Target(XY_Cell(80,70)));
 puts("nearby idle ally responds without interrupting the victim's order");

 // Harvesters receive the same short-route comparison during economic travel.
 unit->Coord=Cell_Coord(XY_Cell(60,64)); unit->TryTryAgain=unit->PATH_RETRY;
 unit->PathThreshhold=MOVE_CLOAK; unit->Mission=MISSION_HARVEST;
 UnitTypeClass &type=const_cast<UnitTypeClass &>(*unit->Class);
 type.IsToHarvest=true;
 CHECK(unit->Basic_Path()); CHECK(unit->Path[0]==FACING_E);

 CrossingUnit *low=new CrossingUnit(); CrossingUnit *high=new CrossingUnit();
 low->Strength=high->Strength=100; low->IsInLimbo=high->IsInLimbo=false;
 low->Coord=Cell_Coord(XY_Cell(63,64)); high->Coord=Cell_Coord(XY_Cell(64,64));
 low->Mission=high->Mission=MISSION_HARVEST;
 low->NavCom=As_Target(XY_Cell(70,64)); high->NavCom=As_Target(XY_Cell(58,64));
 Map[low->Coord].OccupierPtr=low; Map[high->Coord].OccupierPtr=high;
 CHECK(low->ID<high->ID);
 CHECK(low->Start_Of_Move()); CHECK(low->PathDelay>0 && low->Path[0]==FACING_NONE);
 CHECK(high->Start_Of_Move()); CHECK(high->Path[0]==FACING_E);
 high->PrimaryFacing.Set(DIR_E); high->Start_Of_Move(); CHECK(high->IsDriving);
 high->Stop_Driver();
 CHECK(low->NavCom==As_Target(XY_Cell(70,64)) && high->NavCom==As_Target(XY_Cell(58,64)));
 high->Path[0]=FACING_NONE; low->PathDelay=0;
 CrossingUnit::enclosed=true;
 CHECK(!low->Resolve_Harvester_Traffic() && !high->Resolve_Harvester_Traffic());
 CrossingUnit::enclosed=false;
 Map[low->Coord].OccupierPtr=NULL; Map[high->Coord].OccupierPtr=NULL;
 puts("head-on harvesters choose one yielding truck, retain goals, and require a free escape cell");
 type.IsToHarvest=false;
 puts("harvester prefers crossing behind moving traffic");

 // Movement may route around a destroyable object, but may not plan through it.
 unit->Mission=MISSION_MOVE; unit->traffic=MOVE_DESTROYABLE;
 CHECK(unit->Passable_Cell(XY_Cell(64,64),FACING_E,-1,MOVE_TEMP)==0);
 unit->Mission=MISSION_ATTACK;
 CHECK(unit->Passable_Cell(XY_Cell(64,64),FACING_E,-1,MOVE_TEMP)>0);
 unit->MissionQueue=MISSION_MOVE;
 CHECK(unit->Passable_Cell(XY_Cell(64,64),FACING_E,-1,MOVE_TEMP)==0);
 unit->Mission=MISSION_MOVE; unit->MissionQueue=MISSION_ATTACK;
 CHECK(unit->Passable_Cell(XY_Cell(64,64),FACING_E,-1,MOVE_TEMP)>0);
 unit->MissionQueue=MISSION_NONE;
 puts("move does not authorize obstacle attacks; explicit attack still does");

 WaitingInfantry *walker=new WaitingInfantry();
 walker->Coord=Cell_Coord(XY_Cell(63,64)); walker->IsInLimbo=false; walker->Strength=50;
 walker->IsFiring=false; walker->IsFalling=false; walker->IsDriving=false; walker->IsLocked=false;
 walker->Mission=MISSION_MOVE; walker->NavCom=As_Target(XY_Cell(68,64));
 walker->Path[0]=FACING_E; walker->Path[1]=FACING_NONE; walker->Doing=DO_STAND_READY;
 walker->Movement_AI();
 CHECK(walker->Path[0]==FACING_E && walker->TryTryAgain==walker->PATH_WAITING);
 walker->PathDelay=0; walker->Movement_AI();
 CHECK(walker->Path[0]==FACING_NONE && walker->TryTryAgain==0 && Target_Legal(walker->NavCom));
 puts("infantry waits behind traffic and preserves order when rerouting");

 // Exercise the actual player event, including a repeated destination.
 unit->Mission=MISSION_MOVE; unit->MissionQueue=MISSION_ATTACK; unit->PathDelay=100; unit->TryTryAgain=0;
 unit->PathThreshhold=MOVE_TEMP; unit->Path[0]=FACING_N;
 EventClass order(TargetClass(unit),MISSION_MOVE,TargetClass(TARGET_NONE),TargetClass(unit->NavCom));
 order.Execute();
 CHECK(unit->MissionQueue==MISSION_NONE);
 CHECK(unit->PathDelay==0 && unit->TryTryAgain==unit->PATH_RETRY);
 CHECK(unit->PathThreshhold==MOVE_CLOAK && unit->Path[0]==FACING_NONE);
 unit->PathDelay=100; unit->TryTryAgain=3; unit->Path[0]=FACING_E;
 EventClass queued(TargetClass(unit),MISSION_QMOVE,TargetClass(TARGET_NONE),TargetClass(As_Target(XY_Cell(70,64))));
 queued.Execute(); CHECK(unit->PathDelay==100 && unit->TryTryAgain==3 && unit->Path[0]==FACING_E);
 puts("fresh orders reset stale retries; queued orders preserve current travel");

 AircraftClass *hind=new AircraftClass(AIRCRAFT_HIND,HOUSE_GREECE);
 hind->Strength=100; hind->Coord=Cell_Coord(XY_Cell(60,60)); hind->IsInLimbo=false;
 hind->Mission=MISSION_ATTACK; hind->Ammo=5;
 WeaponTypeClass gun("TrackingTest"); gun.Range=3*CELL_LEPTON_W;
 const_cast<AircraftTypeClass &>(*hind->Class).PrimaryWeapon=&gun;
 hind->TarCom=As_Target(XY_Cell(75,60)); hind->NavCom=As_Target(XY_Cell(66,60)); hind->Status=3;
 hind->Mission_Attack(); CHECK(hind->Status==1); // Repick before reaching the obsolete position.
 hind->Status=1; hind->Mission_Attack(); CHECK(hind->Status==1); // No visible firing cells yet: retry.
 for(int c=0;c<MAP_CELL_TOTAL;++c) Map[(CELL)c].IsVisible=true;
 hind->Mission_Attack(); CHECK(hind->Status==2 && Target_Legal(hind->NavCom));
 CHECK(::Distance(As_Coord(hind->NavCom),As_Coord(hind->TarCom))<=hind->Weapon_Range(0));
 hind->Height=0; hind->Mission_Attack(); CHECK(hind->IsTakingOff);
 hind->Height=ObjectClass::FLIGHT_LEVEL; hind->Mission_Attack(); CHECK(hind->Status==3);
 puts("helicopter repicks stale firing position and retries unavailable positions before takeoff");

 InfantryClass *passenger=new InfantryClass(INFANTRY_E1,HOUSE_GREECE);
 passenger->Strength=50; passenger->Mission=MISSION_ENTER;
 passenger->Coord=Cell_Coord(XY_Cell(60,70)); passenger->IsInLimbo=false;
 UnitClass *apc=new UnitClass(UNIT_APC,HOUSE_GREECE);
 apc->Strength=100; apc->Coord=Cell_Coord(XY_Cell(64,70)); apc->IsInLimbo=false; apc->IsDriving=true;
 const_cast<UnitTypeClass &>(*apc->Class).MaxPassengers=5;
 passenger->Assign_Destination(apc->As_Target());
 CHECK(passenger->ArchiveTarget==apc->As_Target());
 passenger->Mission_Enter(); CHECK(passenger->Mission==MISSION_ENTER && passenger->MissionQueue==MISSION_NONE);
 CHECK(passenger->ArchiveTarget==apc->As_Target());
 puts("passenger retains moving transport after rejected docking");

 AircraftClass *transport=new AircraftClass(AIRCRAFT_TRANSPORT,HOUSE_GREECE);
 transport->Strength=100; transport->Height=ObjectClass::FLIGHT_LEVEL;
 const_cast<AircraftTypeClass &>(*transport->Class).MaxPassengers=5;
 InfantryClass *second=new InfantryClass(INFANTRY_E1,HOUSE_GREECE); second->Strength=50;
 passenger->Transmit_Message(RADIO_OVER_OUT);
 CHECK(passenger->Transmit_Message(RADIO_DOCKING,transport)==RADIO_ROGER);
 CHECK(transport->Contact_With_Whom()==passenger && passenger->Contact_With_Whom()==transport);
 CHECK(second->Transmit_Message(RADIO_DOCKING,transport)==RADIO_ROGER);
 CHECK(transport->Contact_With_Whom()==passenger && !second->In_Radio_Contact());
 passenger->Transmit_Message(RADIO_OVER_OUT);
 CHECK(second->Transmit_Message(RADIO_DOCKING,transport)==RADIO_ROGER);
 CHECK(transport->Contact_With_Whom()==second && second->Contact_With_Whom()==transport);
 puts("transport helicopter serializes standby passengers and advances queue");
 return 0;
}
