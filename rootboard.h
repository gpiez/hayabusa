#ifndef ROOTBOARD_H_
#define ROOTBOARD_H_

#include <board.h>

class RootBoard: public Board {

	unsigned int timeBudget;
	unsigned int movesToDo;

public:	
	void rootSearch(unsigned int depth);

};
#endif