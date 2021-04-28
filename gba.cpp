#include <iostream>
#include "loguru.hpp"

using namespace std;
#include <limits.h>   // for CHAR_BIT
static inline uint32_t rotr32 (uint32_t n, unsigned int c)
{
  const unsigned int mask = (CHAR_BIT*sizeof(n) - 1);

  // assert ( (c<=mask) &&"rotate by type width or more");
  c &= mask;
  return (n>>c) | (n<<( (-c)&mask ));
}

//S
class CPU{
	public:
	unsigned int R[18] = {0};
	
	unsigned int R0;
	unsigned int R1;
	unsigned int R2;
	unsigned int R3;
	unsigned int R4;
	unsigned int R5;
	unsigned int R6;
	unsigned int R7;
	unsigned int R8;
	unsigned int R9;
	unsigned int R10;
	unsigned int R11;
	unsigned int R12;
	
	unsigned int R13;//sp
	unsigned int R14;//LR
	unsigned int R15;//PC
	
	unsigned int *CPsR=&R[16];
	unsigned int *sPsR=&R[17];
	
	bool condition(int cond){
		int V=(cond>>28)&1;
		int C=(cond>>29)&1;
		int Z=(cond>>30)&1;
		int N=(cond>>31)&1;
		switch(cond) {
			case 0b0000:
				return Z==1;
			case 0b0001:
				return Z==0;
			case 0b0010:
				return C==1;
			case 0b0011:
				return C==0;
			case 0b0100:
				return N==1;
			case 0b0101:
				return N==0;
			case 0b0110:
				return V==1;
			case 0b0111:
				return V==0;
			case 0b1000:
				return C==1&&Z==0;
			case 0b1001:
				return C==0||Z==1;
			case 0b1010:
				return N==V;
			case 0b1011:
				return N!=V;
			case 0b1100:
				return Z==0&&N==V;
			case 0b1101:
				return Z==1||N!=V;
		}
		return true;
	}
	
	void wait(int s, int n, int i){
	
	}
	
	uint32_t shifter(uint32_t in, uint32_t c, uint32_t type) {
		switch(type){
			case 0b00: //ll
				return in << c;
			case 0b01: //lr
				return in >> c;
			case 0b10: //ar
				return (uint32_t)(((signed int)in)>>c);
			case 0b11: //rr
				return rotr32(in, c);
		}
	}
	
	void dataProcessing(int rem) {
			//TODO: wait times
			bool imm = (rem>>25)==1;
			unsigned int opcode = (rem >>21)&0xf;
			bool setcond = ((rem>>20)&1)==1;
			unsigned int Rn = (rem>>15)&0xf;
			unsigned int Rd = (rem>>12)&0xf;
			unsigned int operand2 = (rem&0xfff);
			
			
			int flagsOut=0;
			unsigned int value = 0;
			if(imm){
				value = rotr32(operand2&0xff, (operand2>>8)*2);
			}
			else {
				uint32_t shift = (operand2&0xff0)>>4;
				uint32_t Rm = operand2&0xf;
				
				uint32_t shiftType=(shift>>1)&3;
				uint32_t shiftAmount=0;
				if((shift&1)==1){
					//shift from register
					uint32_t Rs = (shift&0xf0)>>4;
					shiftAmount = R[Rs]&0xf;
				}
				else{
					shiftAmount = shift>>3;
				}
			}
			int Z,C,N,V;
			switch(opcode) {
				case 0b0000:
					//AND
				case 0b0001:
				case 0b0010:
				case 0b0011:
				case 0b0100:
				case 0b0101:
				case 0b0110:
				case 0b0111:
				case 0b1000:
				case 0b1001:
				case 0b1010:
				case 0b1011:
				case 0b1100:
				case 0b1101:
					int x =1;
			}
	}
	
	void execute(unsigned int op){
		unsigned int cond = op>>27;
		if(!condition(cond)) return;
		int rem = op&0x0fffffff;
		if((rem&0xfffff0)==0x12fff10){
			//BX 2s+N
			int Rn = rem&0xf;
			if(Rn&1){
				Rn=0;
				//TODO: thumb mode;
			}
			R[15]=R[Rn];
			wait(2,1,0);
		}
		else if((rem&0x0f000000)==0x0a000000){
			//BL
			int off= (rem&0xFFFFFF) <<2;
			if(off>>25==1) off|=0xFE000000;

			if(rem&0x1000000){
				//LINK
				R[14]=R[15];
			}
			R[15]+=off;
			wait(2,1,0);
		}
		else if((rem&0xc000000)==0x0) {
			//Data processing
			
			
			
		}
		else if((rem&0xFC000F0)==0x90){
			//MUL
		}
		else if((rem&0xF8000F0)==0x8000090){
			//MULL
		}
		else if((rem&0xC000000)==0x4000000){
			//LDR
		}
		else if((rem&0xE400F90)==0x90){
			//LDRH reg
		}
		else if((rem&0xE400F90)==0x400090){
			//LDRH immediate
		}
		else if((rem&0xE000000)==0x8000000){
			//LDM
		}
		else if((rem&0xFB00FF0)==0x1000090){
			//swp
		}
		else if((rem&0xF000000)==0xF000000){
			//swi
		}
		else if((rem&0xF000010)==0xE000000){
			//cdp
		}
		else if((rem&0xE000000)==0xC000000){
			//LDC
		}
		else if((rem&0xF000010)==0xE000010){
			//MRC
		}
		else {
			//trap undefined
		}
		
		
	}
	

};


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
	
	//gamepak
};

int main(int argc, char* argv[]){
	loguru::init(argc, argv);
    LOG_F(INFO, "Hello from main.cpp!");
    LOG_F(INFO, "main function about to end!");
	CPU cpu;
	cpu.execute(0xA800003);
	cpu.R15=-1;
	for(int i=0; i<4; i++){
		cout << hex<<cpu.shifter(0xF0000001, 2, i)<<endl;
	}
	LOG_F(INFO, "pc=%d", cpu.R[15]);
	return 0;
}