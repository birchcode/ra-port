// Runs against the desktop engine objects; no game assets or rendering required.
#include "function.h"
#include <stdio.h>
#include <stdlib.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x); exit(1); } } while (0)
int main() {
 HouseTypes.Set_Heap(HOUSE_COUNT); HouseTypeClass::Init_Heap();
 BuildingTypes.Set_Heap(STRUCT_COUNT); BuildingTypeClass::Init_Heap();
 InfantryTypes.Set_Heap(INFANTRY_COUNT); InfantryTypeClass::Init_Heap();
 UnitTypes.Set_Heap(UNIT_COUNT); UnitTypeClass::Init_Heap();
 Houses.Set_Heap(HOUSE_MAX); Buildings.Set_Heap(30); Infantry.Set_Heap(30);
 Factories.Set_Heap(10);
 Units.Set_Heap(10); Teams.Set_Heap(10); TeamTypes.Set_Heap(10);
 HouseClass::One_Time(); Map.Size=MAP_CELL_TOTAL; Map.Alloc_Cells();
 static char strings[4000]={0}; SystemStrings=strings; DebugStrings=strings;
 Session.Type = GAME_NORMAL; ScenarioInit=1;
 HouseClass *soviet = new HouseClass(HOUSE_USSR);
 HouseClass *human = new HouseClass(HOUSE_GREECE); human->IsHuman=true; PlayerPtr=human;
 soviet->Make_Ally(HOUSE_USSR); human->Make_Ally(HOUSE_GREECE);
 soviet->Control.TechLevel=1; soviet->Credits=2500; soviet->Control.MaxInfantry=150;
 soviet->CurBuildings=18; soviet->OldBScan=STRUCTF_BARRACKS|STRUCTF_POWER|STRUCTF_REFINERY;
 WeaponTypeClass weapon("TestRifle"); WarheadTypeClass warhead("TestWH");
 for(int i=0;i<ARMOR_COUNT;++i) warhead.Modifier[i]=1;
 weapon.WarheadPtr=&warhead; weapon.Range=3*CELL_LEPTON_W; weapon.Attack=10; weapon.ROF=20;
 InfantryTypeClass &rifle=const_cast<InfantryTypeClass &>(InfantryTypeClass::As_Reference(INFANTRY_E1));
 rifle.PrimaryWeapon=&weapon; rifle.Ownable=1L << HOUSE_USSR; rifle.Risk=10; rifle.Level=1; rifle.Prerequisite=STRUCTF_BARRACKS;
 InfantryClass *attacker=new InfantryClass(INFANTRY_E1,HOUSE_GREECE);
 attacker->Coord=Cell_Coord(64*MAP_CELL_W+65); attacker->IsInLimbo=false; attacker->Strength=50;
 BuildingClass *base=new BuildingClass(STRUCT_BARRACKS,HOUSE_USSR);
 base->Coord=Cell_Coord(64*MAP_CELL_W+64); base->IsInLimbo=false; base->Strength=500;
 InfantryClass *guard=new InfantryClass(INFANTRY_E1,HOUSE_USSR);
 guard->Coord=Cell_Coord(64*MAP_CELL_W+58); guard->IsInLimbo=false; guard->Strength=50;
 guard->Assign_Mission(MISSION_STICKY); guard->Commence(); MissionControl[MISSION_STICKY].IsRecruitable=false;
 Map[guard->Coord].Zones[rifle.MZone]=1; Map[attacker->Coord].Zones[rifle.MZone]=1;
 ScenarioInit=0;
 // Allied fire and human ownership must not activate computer production.
 base->Base_Is_Attacked(guard); CHECK(!soviet->IsStarted);
 soviet->IsHuman=true; base->Base_Is_Attacked(attacker); CHECK(!soviet->IsStarted);
 soviet->IsHuman=false;
 CHECK(!soviet->IsStarted); CHECK(!soviet->IsBaseBuilding);
 base->Base_Is_Attacked(attacker);
 CHECK(soviet->IsStarted && soviet->IsBaseBuilding);
 CHECK(guard->Mission==MISSION_ATTACK); CHECK(guard->TarCom==attacker->As_Target());
 soviet->AI_Infantry(); CHECK(soviet->BuildInfantry!=INFANTRY_NONE);
 CHECK(soviet->Can_Build(&rifle,HOUSE_USSR,true));
 rifle.Cost=100; soviet->BuildInfantry=INFANTRY_E1;
 base->Factory_AI(); CHECK(base->Factory.Is_Valid());
 CHECK(base->Factory->Get_Object()!=NULL);
 CHECK(base->Factory->Is_Building());
 rifle.Level=10; CHECK(!soviet->Can_Build(&rifle,HOUSE_USSR,true));
 CHECK(soviet->Can_Build(&rifle,HOUSE_USSR));
 Base.House=HOUSE_USSR;
 Base.Nodes.Add(BaseNodeClass(STRUCT_POWER,64*MAP_CELL_W+68));
 soviet->AI_Building(); CHECK(soviet->BuildStructure==STRUCT_POWER);
 BuildingClass *yard=new BuildingClass(STRUCT_CONST,HOUSE_USSR);
 yard->Factory_AI(); CHECK(yard->Factory.Is_Valid());
 CHECK(yard->Factory->Get_Object()->What_Am_I()==RTTI_BUILDING);
 CHECK(yard->Factory->Is_Building());

 // Open ground with a known distant economic target, and a garrison at home.
 Map.MapCellX=0; Map.MapCellY=0; Map.MapCellWidth=128; Map.MapCellHeight=128;
 for (int c=0;c<MAP_CELL_TOTAL;++c) Map[(CELL)c].Zones[rifle.MZone]=1;
 Ground[LAND_CLEAR].Cost[rifle.Speed]=1;
 rifle.Level=1; rifle.MaxStrength=100;
 soviet->Center=base->Coord; soviet->Radius=3*CELL_LEPTON_W; soviet->Attack=500;
 attacker->IsInLimbo=true;
 guard->Assign_Target(TARGET_NONE); guard->Assign_Mission(MISSION_GUARD); guard->Commence(); guard->Strength=100;
 BuildingClass *refinery=new BuildingClass(STRUCT_REFINERY,HOUSE_GREECE);
 refinery->Coord=Cell_Coord(64*MAP_CELL_W+100); refinery->Strength=500; refinery->IsInLimbo=false;
 refinery->IsDiscoveredByComputer=true; human->Center=refinery->Coord;
 InfantryClass *garrison[12]; garrison[0]=guard;
 for (int i=1;i<12;++i) {
  garrison[i]=new InfantryClass(INFANTRY_E1,HOUSE_USSR);
  garrison[i]->Coord=Cell_Coord((63+i%3)*MAP_CELL_W+63+i/3);
  garrison[i]->IsInLimbo=false; garrison[i]->Strength=100;
  garrison[i]->Assign_Mission(MISSION_GUARD); garrison[i]->Commence();
 }
 soviet->Tactical_AI();
 CHECK(soviet->AI_Front_Zone()==ZONE_EAST);
 TARGET posts[12]; int distinct=0;
 for (int i=0;i<12;++i) {
  CHECK(garrison[i]->Mission==MISSION_GUARD_AREA); posts[i]=garrison[i]->ArchiveTarget;
  bool unique=true; for(int j=0;j<i;++j) if(posts[j]==posts[i]) unique=false;
  if(unique) ++distinct;
 }
 CHECK(distinct>=3);
 soviet->Tactical_AI();
 for(int i=0;i<12;++i) CHECK(posts[i]==garrison[i]->ArchiveTarget);
 soviet->Attack=0; soviet->Tactical_AI();
 // Simulate arrival at the commanded rally posts; the controller must wait for this.
 for(int i=4;i<12;++i) garrison[i]->Coord=As_Coord(garrison[i]->ArchiveTarget);
 soviet->Tactical_AI();
 int assault=0, reserve=0;
 for(int i=0;i<12;++i) {
  if(garrison[i]->Mission==MISSION_ATTACK) { ++assault; CHECK(garrison[i]->TarCom==refinery->As_Target()); }
  else ++reserve;
 }
 CHECK(assault>=4 && reserve>=3);
 garrison[5]->Strength=10; soviet->Tactical_AI();
 CHECK(garrison[5]->Mission==MISSION_GUARD_AREA);
 CHECK(Cell_X(Coord_Cell(As_Coord(garrison[5]->ArchiveTarget)))<64);
 attacker->IsInLimbo=false; attacker->IsDiscoveredByComputer=true;
 soviet->Tactical_AI(); CHECK(soviet->State==STATE_ATTACKED);
 CHECK(garrison[0]->TarCom==attacker->As_Target());

 // Counter-production responds to the enemy composition and protects construction cash.
 soviet->BuildStructure=STRUCT_NONE; soviet->State=STATE_BUILDUP;
 warhead.Modifier[ARMOR_STEEL]=fixed(1,10);
 WeaponTypeClass antiarmor("AntiArmor"); WarheadTypeClass armorhead("ArmorWH");
 armorhead.Modifier[ARMOR_NONE]=fixed(1,10); armorhead.Modifier[ARMOR_STEEL]=1;
 antiarmor.WarheadPtr=&armorhead; antiarmor.Attack=10; antiarmor.ROF=20;
 InfantryTypeClass &rocket=const_cast<InfantryTypeClass &>(InfantryTypeClass::As_Reference(INFANTRY_E3));
 rocket.PrimaryWeapon=&antiarmor; rocket.Ownable=1L<<HOUSE_USSR; rocket.Level=1; rocket.Cost=100; rocket.Prerequisite=STRUCTF_BARRACKS;
 human->CurUnits=20; human->CurInfantry=1;
 CHECK(soviet->AI_Production_Score(&rocket,0)>soviet->AI_Production_Score(&rifle,0));
 human->CurUnits=1; human->CurInfantry=20;
 CHECK(soviet->AI_Production_Score(&rifle,0)>soviet->AI_Production_Score(&rocket,0));
 CHECK(soviet->AI_Production_Score(&rifle,0)>soviet->AI_Production_Score(&rifle,5));
 soviet->Credits=50; CHECK(soviet->AI_Production_Score(&rifle,0)==0);
 // Scouting is limited to one unit, and unobserved targets do not launch an attack.
 attacker->IsDiscoveredByComputer=false; refinery->IsDiscoveredByComputer=false;
 for(int i=0;i<12;++i) {
  garrison[i]->Strength=100; garrison[i]->Assign_Target(TARGET_NONE);
  garrison[i]->Assign_Mission(MISSION_GUARD); garrison[i]->Commence();
 }
 soviet->Tactical_AI(); soviet->Tactical_AI();
 int scouts=0; for(int i=0;i<12;++i) if(garrison[i]->Mission==MISSION_MOVE) ++scouts;
 CHECK(scouts==1);
 soviet->Credits=350; soviet->BuildStructure=STRUCT_POWER; soviet->BQuantity[STRUCT_CONST]=1;
 const_cast<BuildingTypeClass &>(BuildingTypeClass::As_Reference(STRUCT_POWER)).Cost=300;
 CHECK(soviet->AI_Production_Score(&rifle,0)==0);
 soviet->BQuantity[STRUCT_CONST]=0; CHECK(soviet->AI_Production_Score(&rifle,0)>0);
 puts("campaign AI: activation, production, rebuilds, defensive posts, stable orders, rally/assault/reserve, retreat, incursion and counter-production passed");
 return 0;
}
