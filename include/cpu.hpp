#pragma once
#include <iostream>
#include "loguru.hpp"
#include "alu.hpp"
#include "mmu.hpp"

//TODO: make Sure thiS lineS up with how CPSr repreSentS mode bitS
enum CPUMode{
	USER = 0b10000,
	FIQ = 0b10001,
	IRQ = 0b10010,
	SVC = 0b10011,
	ABT = 0b10111,
	UND = 0b11011,
	SYS = 0b11111
};

class PSR {
public:
	bool N=false;
	bool Z=false;
	bool C=false;
	bool V=false;
	
	//reset state TODO: verify interrupts disabled on reset
	bool IRQ_disable=true;
	bool FIQ_disable=true;
	bool Thumb = false;
	CPUMode mode = SVC;
	
	uint32_t asInt(){
		uint32_t out=0;
		out|=N<<31;
		out|=Z<<30;
		out|=C<<29;
		out|=V<<28;
		
		out|=IRQ_disable<<7;
		out|=FIQ_disable<<6;
		out|=Thumb<<5;
		
		out|=mode;
		return out;
	}
	
	void fromInt(uint32_t in) {
		N=((in>>31)&1)==1;
		Z=((in>>30)&1)==1;
		C=((in>>29)&1)==1;
		V=((in>>28)&1)==1;
		IRQ_disable= ((in>>7)&1)==1;
		FIQ_disable=((in>>6)&1)==1;
		Thumb=((in>>5)&1)==1;
		mode= (CPUMode)(in&0x1F);
	}
	
	
};
 

class RegisterFile {
	uint32_t R[37] = {0};
	unsigned int R13;//sp
	unsigned int R14;//LR
	unsigned int R15;//PC
	
	//unsigned int *CPSR=&R[16];
	
	
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

	public:
	PSR CPSR;
	PSR SPSR_user;
	PSR SPSR_fiq;
	PSR SPSR_svc;
	PSR SPSR_abt;
	PSR SPSR_irq;
	PSR SPSR_und;
	
	PSR& SPSR() {
		return SPSR_user;
	}
	
	uint32_t& operator[](int r){
		uint8_t mode = CPSR.mode;
		if(mode==CPUMode::USER){
			return R[r];
		}
		else if(mode==CPUMode::FIQ){
			if(r<8) return R[r];
			else return R[r+10];
		}
		else if(mode==CPUMode::SVC){
			if(r<13) return R[r];
			else return R[r+13];
		}
		else if(mode==CPUMode::ABT){
			if(r<13) return R[r];
			else return R[r+16];
		}
		else if(mode==CPUMode::IRQ){
			if(r<13) return R[r];
			else return R[r+19];
		}
		else if(mode==CPUMode::UND){
			if(r<13) return R[r];
			else return R[r+22];
		}
	}
	
	

};

class CPU{
private:
	uint8_t iwait=0;
	uint8_t swait=0;
	uint8_t nwait=0;
	
	uint8_t BIGEND=0;
	
public:
	MMU mmu;
	RegisterFile R;	
	
	void clearFlags();
	
	bool condition(int cond);
	
	void wait(int s, int n, int i);
	uint32_t handleshift(uint32_t operand2);
	
	uint32_t multCycles(int op2);
	
	
	
	void execute(unsigned int op);
	void executeThumb(uint16_t op);
	
	//void exception(Exception e);
	void trap();
	
	void ARM_DataProcessing(uint8_t opcode, uint32_t Rd, uint32_t Rn, uint32_t operand2,
		 bool imm, bool setcond);
	void ARM_BX(uint8_t Rn);
	void ARM_BL(uint32_t off, bool link);
	void ARM_MUL(uint8_t Rd, uint8_t Rm, uint8_t Rs,uint8_t Rn, bool accumulate, bool setCond);
	void ARM_MULL(uint8_t Rd_hi, uint8_t Rd_lo, uint8_t Rs,uint8_t Rm, bool accumulate,
			bool setCond, bool sign );
	void ARM_LDR(uint8_t Rd, uint8_t Rn, uint32_t off, bool imm, bool post, bool down,
 		bool byte, bool writeback, bool store);
 	void ARM_LDRH(uint8_t Rd, uint8_t Rn, uint32_t off, bool imm, bool post, bool down, 
 		bool writeback, bool store, bool sign, bool halfwords);
 	void ARM_LDM(uint8_t Rn, uint16_t Rlist, bool post, bool down, bool psr, 
		bool writeback, bool store);
	void ARM_SWP(uint8_t Rd, uint8_t Rn, uint8_t Rm, bool byte);
	void ARM_MRS(uint8_t Rd, bool spsr);
	void ARM_MSR_REG(uint8_t Rm, bool spsr);
	void ARM_MSR_IMM(uint16_t operand2, bool spsr, bool imm);
};