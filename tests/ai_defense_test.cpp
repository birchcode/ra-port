#include "../CODE/AI_DEFENSE.H"
#include <cassert>

int main()
{
	int values[6] = {10, 10, 30, 40, 50, 60};
	assert(AI_Defender_Slot(values, 0, 6, 20) == 0);
	assert(AI_Defender_Slot(values, 5, 6, 20) == 5);
	assert(AI_Defender_Slot(values, 6, 6, 5) == -1);
	assert(AI_Defender_Slot(values, 6, 6, 10) == -1);
	int slot = AI_Defender_Slot(values, 6, 6, 100);
	assert(slot == 0);
	values[slot] = 100;
	assert(values[1] == 10); // Equal scores must not duplicate a defender.
	assert(AI_Defender_Slot(values, 6, 6, 90) == 1);
	values[1] = 90;
	assert(AI_Defender_Slot(values, 6, 6, 80) == 2);
}
