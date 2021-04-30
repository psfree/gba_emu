#pragma once
#include <iostream>
#include "loguru.hpp"
#include "alu.hpp"

class MMU {
	//ram
	uint8_t bios[0x4000];
	uint8_t wram0[0x40000];
	uint8_t wram1[0x8000];
	uint8_t io[0x3ff];
	//vram
	uint8_t pram[0x400];
	uint8_t vram[0x18000];
	uint8_t obj[0x400];
	uint32_t memfault(){
		return -1;
	}
	public:
	uint32_t getWord(uint32_t addr){
		if(addr<0x4000){
			return bios[addr];
		}
		if(addr>0x02000000&&addr<0x02040000){
			return wram0[addr&0x7ffff];
		}
		if(addr>0x03000000&&addr<0x03008000){
			return wram1[addr&0x7fff];
		}
		return memfault();
	}
	void setWord(uint32_t addr, uint32_t val){
		bios[addr] = val;
		bios[addr+1] = val >>8;
		bios[addr+2] = val >>16;
		bios[addr+3] = val >> 24;
	}
	
	//gamepak
};

class CPU{
private:
	uint8_t iwait=0;
	uint8_t swait=0;
	uint8_t nwait=0;
	
	uint8_t BIGEND=0;
	
public:
	unsigned int R[37] = {0};	
	unsigned int R13;//sp
	unsigned int R14;//LR
	unsigned int R15;//PC
	
	unsigned int *CPsR=&R[16];
	unsigned int *sPsR=&R[17];
	
	unsigned int *R8_fiq=&R[18];
	unsigned int *R9_fiq=&R[19];
	unsigned int *R10_fiq=&R[20];
	unsigned int *R11_fiq=&R[21];
	unsigned int *R12_fiq=&R[22];
	unsigned int *R13_fiq=&R[23];
	unsigned int *R14_fiq=&R[24];
	unsigned int *spsr_fiq=&R[25];

	unsigned int *R13_svc=&R[26];
	unsigned int *R14_svc=&R[27];
	unsigned int *sPsR_svc=&R[28];
	
	unsigned int *R13_abort=&R[29];
	unsigned int *R14_abort=&R[30];
	unsigned int *sPsR_abort=&R[31];
	
	unsigned int *R13_irq=&R[32];
	unsigned int *R14_irq=&R[33];
	unsigned int *sPsR_irq=&R[34];
	
	unsigned int *R13_und=&R[35];
	unsigned int *R14_und=&R[36];
	unsigned int *sPsR_und=&R[37];
	
	
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
	uint32_t handleshift(uint32_t operand2);
	
	uint32_t multCycles(int op2);
	
	void dataProcessing(int rem);
	
	void execute(unsigned int op);
	void executeThumb(uint16_t op);
			
	void trap();
	
};