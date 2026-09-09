#ifndef AI_DEFENSE_H
#define AI_DEFENSE_H

// Keep the strongest rescue candidates, replacing exactly one weakest slot.
inline int AI_Defender_Slot(int const * values, int count, int capacity, int threat)
{
	if (count < capacity) return count;
	int weakest = 0;
	for (int i = 1; i < count; ++i) {
		if (values[i] < values[weakest]) weakest = i;
	}
	return threat > values[weakest] ? weakest : -1;
}

#endif
