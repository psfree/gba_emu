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

enum Exception {
	RESET = 0x0,
	UNDEFINED = 0x4,
	eSVC = 0x8,
	ABT_PREFETCH = 0xc,
	ABT_DATA = 0x10,
	eIRQ = 0x18,
	eFIQ = 0x1C
};

enum DataOps{
	AND,
	EOR,
	SUB,
	RSB,
	ADD,
	ADC,
	SBC,
	RSC,
	TST,
	TEQ,
	CMP,
	CMN,
	ORR,
	MOV,
	BIC,
	MVN
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

	public:
	PSR CPSR;
	PSR SPSR_user;
	PSR SPSR_fiq;
	PSR SPSR_svc;
	PSR SPSR_abt;
	PSR SPSR_irq;
	PSR SPSR_und;
	
	PSR& SPSR() {
		uint8_t mode = CPSR.mode;
		if(mode==CPUMode::USER){
			return SPSR_user;
		}
		else if(mode==CPUMode::FIQ){
			return SPSR_fiq;
		}
		else if(mode==CPUMode::SVC){
			return SPSR_svc;
		}
		else if(mode==CPUMode::ABT){
			return SPSR_abt;
		}
		else if(mode==CPUMode::IRQ){
			return SPSR_irq;
		}
		return SPSR_und;
	}
	
	uint32_t& operator[](int r){
		uint8_t mode = CPSR.mode;
		if(mode==CPUMode::USER){
			return R[r];
		}
		if(mode==CPUMode::FIQ){
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
		else { //CPUMode::UND
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
	
	void exception(Exception e);
	
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