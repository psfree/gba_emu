#pragma once
#include <iostream>
#include "loguru.hpp"
#include "alu.hpp"

class CPU{
	public:
	unsigned int R[18] = {0};	
	unsigned int R13;//sp
	unsigned int R14;//LR
	unsigned int R15;//PC
	
	unsigned int *CPsR=&R[16];
	unsigned int *sPsR=&R[17];
	
	void clearFlags();
		
	void setZ();
	void clearZ();
	void setN();
	void clearN();
	uint8_t getC();
	void setC();
	void clearC();
	void setV();
	void clearV();
	
	bool condition(int cond);
	
	void wait(int s, int n, int i);
	
	int multCycles(int op2);
	
	void dataProcessing(int rem);
	
	void execute(unsigned int op);
			
	void trap();
	
};