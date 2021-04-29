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
	unsigned int R13;//sp
	unsigned int R14;//LR
	unsigned int R15;//PC
	
	unsigned int *CPsR=&R[16];
	unsigned int *sPsR=&R[17];
	
	void clearFlags(){
		R[16]&=0x0FFFFFFF;
	}
	
	void setZ(){
		R[16]|=0x40000000;
	}
	void clearZ(){
		R[16]&=0xBFFFFFFF;
	}
	void setN(){
		R[16]|=0x80000000;
	}
	void clearN(){
		R[16]&=0x7FFFFFFF;
	}
	uint8_t getC(){
		return (R[16]&0x20000000)>>29;
	}
	void setC(){
		R[16]|=0x20000000;
	}
	void clearC(){
		R[16]&=0xDFFFFFFF;
	}
	void setV(){
		R[16]|=0x10000000;
	}
	void clearV(){
		R[16]&=0xEFFFFFFF;
	}
	
	bool condition(int cond){
		int V=(*CPsR>>28)&1;
		int C=(*CPsR>>29)&1;
		int Z=(*CPsR>>30)&1;
		int N=(*CPsR>>31)&1;
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
	
	uint8_t aluCarry=0;
	uint8_t aluOverflow=0;
	uint32_t ADD(uint64_t a, uint64_t b){
		uint64_t total = a +b;
		uint32_t itotal=total;
		bool aneg = (a>>31)==1;
		bool bneg = (b>>31)==1;
		bool tneg = (itotal>>31)==1;
		if(aneg==bneg && aneg!=tneg) aluOverflow = 1;
		else aluOverflow=0;
		aluCarry = total>>32;
		return itotal;
	}
	
	uint8_t shiftCarry=0;
	uint32_t shifter(uint32_t in, uint32_t c, uint32_t type) {
		if(c==0) {
			shiftCarry = getC(); //TODO: IF LsL#0
			return in;
		}
		switch(type){
			case 0b00: //ll
				if(c==32) { 
					shiftCarry = in&1;
					return 0;
				}
				else if(c>32){
					shiftCarry = 0;
					return 0;
				}
				shiftCarry = (in>>(32-c))&1;
				return in << c;
			case 0b01: //lr
				if(c==32) { 
					shiftCarry = (in>>31)&1;
					return 0;
				}
				else if(c>32){
					shiftCarry = 0;
					return 0;
				}
				shiftCarry = (in>>(c-1))&1;
				return in >> c;
			case 0b10: //ar
				if(c>31){
					shiftCarry = (in>>31)&1;
					return 0xFFFFFFFF;
				}
				shiftCarry = (in>>(c-1))&1;
				return (uint32_t)(((signed int)in)>>c);
			case 0b11: //rr
				if(c==32) { 
					shiftCarry = (in>>31)&1;
					return in;
				}
				else if(c>32){
					c=c%32;
				}
				int out = rotr32(in, c);
				shiftCarry= (out>>31);
				return out;
		}
		return 0;
	}
	
	int multCycles(int op2){
			int m=4;
			if((op2&0xffffff00)==0xffffff00 ||(op2&0xffffff00)==0x0) m=1;
			if((op2&0xffff0000)==0xffff0000 ||(op2&0xffff0000)==0x0) m=2;
			if((op2&0xff000000)==0xff000000 ||(op2&0xff000000)==0x0) m=3;
			return m;
	}
	
	void dataProcessing(int rem) {
			//TODO: wait times
			bool imm = (rem>>25)==1;
			unsigned int opcode = (rem >>21)&0xf;
			bool setcond = ((rem>>20)&1)==1;
			unsigned int Rn = (rem>>16)&0xf;
			unsigned int Rd = (rem>>12)&0xf;
			unsigned int operand2 = (rem&0xfff);
			
			int flagsOut=0;
			unsigned int value = 0;
			if(imm){
				value = shifter(operand2&0xff,(operand2>>8)*2, 0b11 );
			}
			else {
				uint32_t shift = (operand2&0xff0)>>4;
				uint32_t Rm = operand2&0xf;
				
				uint32_t shiftType=(shift>>1)&3;
				uint32_t shiftAmount=0;
				if((shift&1)==1){
					//shift from register
					uint32_t Rs = (shift&0xf0)>>4;
					shiftAmount = R[Rs]&0xff;
					value = shifter(R[Rm], shiftAmount,shiftType);

				}
				else{
					shiftAmount = shift>>3;
					value = shifter(R[Rm], shiftAmount,shiftType);
				}
			}
			//TODO: 4.5.4, r15 handling, TsT
			int tst = 0;
			bool logical = false;
			bool test = false;
			switch(opcode) {
				case 0b0000: //AND
					logical = true;
					R[Rd]= R[Rn]&value;
					break;
				case 0b0001: //EOR/XOR
					logical = true;
					R[Rd]= R[Rn]^value;
					break;
				case 0b0010: //sub
					R[Rd] = ADD(R[Rn],-value);
					break;
				case 0b0011://rsb
					R[Rd] = ADD(value,-R[Rn]);
					break;
				case 0b0100: //ADD
					R[Rd] = ADD(R[Rn],value);
					break;
				case 0b0101: //ADC
					R[Rd] = ADD(R[Rn], value+getC());
					break;
				case 0b0110: //sbc
					R[Rd] = ADD(R[Rn], -value+getC()-1);
					break;
				case 0b0111: //rsc
					R[Rd] = ADD(value,-R[Rn]+getC()-1);
					break;
				case 0b1000: //tst
					logical = test = true;
					tst = R[Rn]&value;
					break;
				case 0b1001: //TEQ
					logical = test= true;
					tst = R[Rn]^value;
					break;
				case 0b1010: //CMP
					logical = test = true;
					tst = R[Rn]-value;
					break;
				case 0b1011: //CMN
					logical = tst =true;
					tst = R[Rn]+value;
					break;
				case 0b1100://ORR
					logical = true;
					R[Rd] = R[Rn]|value;
					break;
				case 0b1101: //MOV
					R[Rd] = value;
					break;
				case 0b1110: //BIC
					logical = true;
					R[Rd] = R[Rn]&(~value);
					break;
				case 0b1111: //MVN
					logical = true;
					R[Rd] = ~value;
					break;
				default:
					trap();
			}
			if(setcond) { //TODO:check Rd!=R15
				if(logical){
					if(test) {
						if(tst==0) setZ();
						if(tst>>31==1) setN();
					}else {
						if(R[Rd]==0) setZ();
						if(R[Rd]>>31==1) setN();
					}	
				}
				else{
					if(aluOverflow==1) setV(); //TODO: make sure this is valid
					if(aluCarry==1) setC();
					if(R[Rd]==0) setZ();
					if((R[Rd]>>31)==1) setN();
					
				}
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
		else if((rem&0xFC000F0)==0x90){
			//MUL
			bool accumulate = ((rem>>21)&1) == 1;
			bool setCond = ((rem>>20)&1) == 1;
			uint8_t Rd = ((rem>>16)&0xf);
			uint8_t Rn = ((rem>>12)&0xf);
			uint8_t Rs = ((rem>>8)&0xf);
			uint8_t Rm = (rem&0xf);
			if(Rd==15||Rs==15||Rm==15) trap();
			if(Rd==Rm||Rd==Rs) trap();
			if(!accumulate) Rn=0;
			R[Rd] = R[Rm]*R[Rs];
			if(setCond){
				if(R[Rd]>>31) setN(); else clearN();
				if(R[Rd]==0) setZ(); else clearZ();
			}
			wait(1, 0,multCycles(R[Rs])+2);
		}
		else if((rem&0xF8000F0)==0x800090){
			//MULL
			bool sign = ((rem>>22)&1) == 1;
			bool accumulate = ((rem>>21)&1) == 1;
			bool setCond = ((rem>>20)&1) == 1;
			uint8_t Rd_hi = ((rem>>16)&0xf);
			uint8_t Rd_lo = ((rem>>12)&0xf);
			uint8_t Rs = ((rem>>8)&0xf);
			uint8_t Rm = (rem&0xf);
			if(Rd_hi==15||Rd_lo==15||Rs==15||Rm==15) trap();
			if(Rd_hi==Rd_lo || Rd_hi==Rm||Rd_lo==Rm) trap();
			uint64_t out =0;
			if(sign) {
				//TODO:smarter sign extension
				int64_t sRm = R[Rm];
				if((sRm&0xf0000000)==0xf0000000) sRm|=0xffffffff00000000LL;
				int64_t sRs = R[Rs];
				if((sRs&0xf0000000)==0xf0000000) sRs|=0xffffffff00000000LL;
				int64_t sout = sRm*sRs;
				out = (uint64_t)sout;
			}else{
				out = ((uint64_t)R[Rm]) * (uint64_t)R[Rs];
			}
			if(accumulate){
				out+=R[Rd_hi]+R[Rd_lo];
			}
			R[Rd_lo] = out&0xffffffff;
			R[Rd_hi] = out>>32;
			if(setCond){
				if(R[Rd_hi]>>31) setN(); else clearN();
				if(R[Rd_hi]==0&&R[Rd_lo]==0) setZ(); else clearZ();
			}
			int m = multCycles(R[Rs]);
			if(accumulate) wait(1, 0,m+2);
			else wait(1, 0 ,m+1);
			
		}
		//probably last in order
		else if((rem&0xc000000)==0x0) {
			//Data processing
			dataProcessing(rem);
			
		}
		
		else if((rem&0xC000000)==0x4000000){
			//LDR
			bool imm = ((rem>>25)&1)==0;
			bool post = ((rem>>24)&1)==0;
			bool up = ((rem>>21)&1)==1;
			bool byte = ((rem>>22)&1)==1;
			bool writeback = ((rem>>21)&1)==1;
			bool store = ((rem>>20)&1)==0;
			uint8_t Rn = ((rem>>16)&0xf);
			uint8_t Rd = ((rem>>12)&0xf);
			uint32_t off = (rem&0xfff);
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
			//trap undefined, may need explicit check for op
			trap();
			wait(2, 1,1);
		}
		
		
	}
	
	void trap(){
		int*x=NULL;
		*x++;
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
	//cpu.execute(0xA800003);
	cpu.R15=-1;
	for(int i=0; i<4; i++){
		cout << hex<<cpu.shifter(0xF0000001, 2, i)<<endl;
	}
	
	cpu.R[1]=-10;
	cpu.R[2]=20;
	cpu.R[16]=0x40000000;
	cpu.execute(0x100192);
	LOG_F(INFO, "pc=%d", cpu.R[15]);
	
	cpu.R[1]=-10;
	cpu.R[2]=20;
	cpu.R[16]=0x40000000;
	cpu.execute(0xD45192);
	cout << cpu.R[4]<<endl;
	cout <<cpu.R[5]<<endl;
	
	//flag tet
	cpu.R[1]=0xffffffff;
	cpu.R[2]=1;
	cpu.R[16]=0x40000000;
	cpu.execute(0x910002);
	return 0;
}