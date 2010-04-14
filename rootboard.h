#ifndef ROOTBOARD_H_
#define ROOTBOARD_H_

#include <board.h>
#include <eval.h>

class RootBoard: public Board {

	Eval eval;
	unsigned int timeBudget;
	unsigned int movesToDo;

public:	
	void rootSearch(unsigned int depth);

};
#endif