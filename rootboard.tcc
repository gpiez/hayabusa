#include "rootboard.h"

void RootBoard::rootSearch(unsigned int depth) {
	if (color < 0)
		reinterpret_cast<ColoredBoard<Black>*>(boards + iMove)->rootSearch(depth);
	else
		reinterpret_cast<ColoredBoard<White>*>(boards + iMove)->rootSearch(depth);
}

