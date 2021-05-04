#include "./include/cpu.hpp"
using namespace std;
using namespace ALU;
//S
#define RESET_VECTOR 0x0
#define UND_HANDLER 0x4
#define SVC_HANDLER 0x8
/*
enum Exception {
	RESET = 0x0,
	UNDEFINED = 0x4,
	SVC = 0x8,
	ABT_PREFETCH = 0xc,
	ABT_DATA = 0x10,
	IRQ = 0x18,
	FIQ = 0x1C
};

void CPU::exception(Exception e){
	if(RESET) {
		mode = CPUMode.SVC;
	}
} */


void CPU::clearFlags(){
	R.CPSR.Z=0;
	R.CPSR.N=0;
	R.CPSR.C=0;
	R.CPSR.V=0;
}
	
bool CPU::condition(int cond){
	int V=(R.CPSR.V);
	int C=(R.CPSR.C);
	int Z=(R.CPSR.Z);
	int N=(R.CPSR.N);
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
	
void CPU::wait(int s, int n, int i){

}



uint32_t CPU::multCycles(int op2){
		int m=4;
		if((op2&0xffffff00)==0xffffff00 ||(op2&0xffffff00)==0x0) m=1;
		if((op2&0xffff0000)==0xffff0000 ||(op2&0xffff0000)==0x0) m=2;
		if((op2&0xff000000)==0xff000000 ||(op2&0xff000000)==0x0) m=3;
		return m;
}


uint32_t CPU::handleshift(uint32_t operand2){
	uint32_t value=0;
	uint32_t shift = (operand2&0xff0)>>4;
	uint32_t Rm = operand2&0xf;
	
	uint32_t shiftType=(shift>>1)&3;
	uint32_t shiftAmount=0;
	if((shift&1)==1){
		//shift from register
		uint32_t Rs = (shift&0xf0)>>4;
		shiftAmount = R[Rs]&0xff;
		value = shifter(R[Rm], shiftAmount,shiftType);
		iwait++;
	}
	else{
		shiftAmount = shift>>3;
		value = shifter(R[Rm], shiftAmount,shiftType);
	}
	return value;
} 


void CPU::execute(unsigned int op){
	iwait=swait=nwait=0;
	unsigned int cond = op>>28;
	if(!condition(cond)) return;
	int rem = op&0x0fffffff;
	if((rem&0xfffff0)==0x12fff10){
		int Rn = rem&0xf;
		if(Rn&1){
			Rn=0;
			//TODO: thumb mode switch
		}
		ARM_BX(Rn);
	}
	else if((rem&0xFC000F0)==0x90){
		//MUL
		bool accumulate = ((rem>>21)&1) == 1;
		bool setCond = ((rem>>20)&1) == 1;
		uint8_t Rd = ((rem>>16)&0xf);
		uint8_t Rn = ((rem>>12)&0xf);
		uint8_t Rs = ((rem>>8)&0xf);
		uint8_t Rm = (rem&0xf);
		ARM_MUL(Rd,Rm,Rs,Rn, accumulate, setCond);
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
		ARM_MULL(Rd_hi,Rd_lo, Rs,Rm, accumulate, setCond,sign);
	}
	else if((rem&0xC000000)==0x4000000){
		//LDR
		bool imm = ((rem>>25)&1)==0;
		bool post = ((rem>>24)&1)==0;
		bool down = ((rem>>23)&1)==0;
		bool byte = ((rem>>22)&1)==1;
		bool writeback = ((rem>>21)&1)==1;
		bool store = ((rem>>20)&1)==0;
		uint8_t Rn = ((rem>>16)&0xf);
		uint8_t Rd = ((rem>>12)&0xf);
		uint32_t off = (rem&0xfff);
		ARM_LDR(Rd,Rn,off,imm,post,down,byte,writeback,store);		
	}
	else if((rem&0xE000F90)==0x90){
		//LDRH
		bool imm = ((rem>>22)==1);
		bool post = ((rem>>24)&1)==0;
		bool down = ((rem>>23)&1)==0;
		bool writeback = ((rem>>21)&1)==1;
		bool store = ((rem>>20)&1)==0;
		uint8_t Rn = ((rem>>16)&0xf);
		uint8_t Rd = ((rem>>12)&0xf);
		bool sign = ((rem>>6)&1)==1;
		bool halfwords = ((rem>>5)&1)==1;
		uint8_t off_hi = ((rem>>8)&0xff);
		uint8_t Rm_off = (rem&0xff);
	
		uint32_t off =0;
		if(imm) off = (off_hi<<4)&Rm_off;
		else {
			if(Rm_off==15) trap(); //not allowed
			off = R[Rm_off];
		}
		if(down) off=-off;
		
		ARM_LDRH(Rd, Rn, off, imm, post, down, writeback, store, sign, halfwords);
	}
	else if((rem&0xE000000)==0x8000000){
		//LDM
		bool post = ((rem>>24)&1)==0;
		bool down = ((rem>>23)&1)==0;
		bool psr = ((rem>>22)&1)==1;
		bool writeback = ((rem>>21)&1)==0;
		bool store = ((rem>>20)&1)==0;
		uint8_t Rn = ((rem>>16)&0xf);
		uint16_t Rlist = rem&0xfff;
		
		ARM_LDM(Rn, Rlist, post, down, psr, writeback, store);
		
	}
	else if((rem&0xFB00FF0)==0x1000090){
		//swp
		bool byte = ((rem>>22)&1)==1;
		uint8_t Rn = ((rem>>16)&0xf);
		uint8_t Rd = ((rem>>12)&0xf);
		uint8_t Rm = (rem&0xf);
		ARM_SWP(Rd,Rn,Rm, byte);
	}
	else if((rem&0xF000000)==0xF000000){
		//swi
	}
	else if((rem&0xF000010)==0xE000000){
		//cdp
		//Not required (unless im mistaken)
	}
	else if((rem&0xE000000)==0xC000000){
		//LDC
	}
	else if((rem&0xF000010)==0xE000010){
		//MRC
	}
	else if((rem&0xFBF0FFF)==0x10F0000) { //MRs
		bool spsr = ((rem>>22)&1)==1;
		uint8_t Rd = (rem>>12)&0xf;
		ARM_MRS(Rd, spsr);
	}
	else if((rem&0xFBFFFF0)==0x129F000) { //msr using register
		bool spsr = ((rem>>22)&1)==1;
		uint8_t Rm = rem&0xf;
		ARM_MSR_REG(Rm, spsr);
	}
	else if((rem&0xDBFF000)==0x128F000) { //msr immediate
		bool imm = ((rem>>25)&1)==1;
		bool spsr = ((rem>>22)&1)==1;
		uint16_t operand2 = rem&0xfff;
		ARM_MSR_IMM(operand2, spsr, imm);
	}
	//probably last in order
	else if((rem&0xc000000)==0x0) {
		//Data processing
		bool imm = (rem>>25)==1;
		uint8_t opcode = (rem >>21)&0xf;
		bool setcond = ((rem>>20)&1)==1;
		uint32_t Rn = (rem>>16)&0xf;
		uint32_t Rd = (rem>>12)&0xf;
		uint32_t operand2 = (rem&0xfff);
		ARM_DataProcessing(opcode, Rd,Rn, operand2, imm, setcond);
	}
	else if((rem&0x0f000000)==0x0a000000){
		//BL
		int off= (rem&0xFFFFFF) <<2;
		if(off>>25==1) off|=0xFE000000;
		bool link = (rem&0x1000000)==1;
		ARM_BL(off, link);
	}
	else {
		//trap undefined, may need explicit check for op
		trap();
		wait(2, 1,1);
	}
	
	wait(swait, nwait,iwait);

}

void CPU::trap(){
	int*x=NULL;
	*x++;
}

#include <cstdlib>

int main(int argc, char* argv[]){
	loguru::init(argc, argv);
    LOG_F(INFO, "Hello from main.cpp!");
    LOG_F(INFO, "main function about to end!");
	CPU cpu;
	string in;
	cpu.R.CPSR.Z=1;
	while(getline(cin,in)) {
		//00005EE3
		long x = stol(in, nullptr,16);
		uint32_t instr = (uint32_t)x;
		uint32_t in = (instr&0xff) << 24;
		in|=(instr&0xff00) <<8;
		in|=(instr&0xff0000) >>8;
		in|=(instr&0xff000000) >>24;
		cpu.execute(in);
	}
	cpu.execute(0xE3A0000A);
	cpu.execute(0xE3A01003);
	cpu.execute(0xE0800001);
	cpu.execute(0xE3A03018);
	
	
	
	//cpu.execute(0xA800003);
	//cpu.R15=-1;
	for(int i=0; i<4; i++){
		cout << hex<<shifter(0xF0000001, 2, i)<<endl;
	}
	
	cpu.R[1]=-10;
	cpu.R[2]=20;
	cpu.R.CPSR.Z=1;
	cpu.execute(0x100192);
	LOG_F(INFO, "pc=%d", cpu.R[15]);
	
	cpu.R[1]=-10;
	cpu.R[2]=20;
	cpu.R.CPSR.Z=1;
	cpu.execute(0xD45192);
	cout << cpu.R[4]<<endl;
	cout <<cpu.R[5]<<endl;
	
	//flag tet
	cpu.R[1]=0xffffffff;
	cpu.R[2]=1;
	cpu.R.CPSR.Z=1;
	cpu.execute(0x910002);
	
	cpu.R[1]=5;
	cpu.R[2]=3;
	cpu.R[3]=-9;
	cpu.R.CPSR.Z=1;
	cpu.executeThumb(0x1f48);
	cpu.R[1]=0x0;
	cpu.R.CPSR.Z=1;
	cpu.execute(0xE5910000);
	
	//test str/ld
	cpu.R[1] = 0x4;
	cpu.R[2] = 0x0a0b0c0d;
	cpu.ARM_LDR(2, 1, /*off*/0, true, false, false,false, false, true);
	cpu.R[1]=2;
	cpu.ARM_LDR(3, 1, /*off*/0, true, false, false,false, false, false);
	cout << cpu.mmu.getWord(4)<<endl;
	
	//test stm
	cpu.R[0] =0x1000;
	cpu.R[1] = 0x10000000;
	cpu.R[5] = 0x50000000;
	cpu.R[7] = 0x70000000;
	cpu.ARM_LDM(0, 0b10100010, true, false, false, true, true);
	cpu.R[1] = 0x0;
	cpu.R[5] = 0x0;
	cpu.R[7] = 0x0;
	cpu.ARM_LDM(0, 0b10100010, false, true, false, true, false);
	
	//test swp
	cpu.R[0] =0x1000;
	cpu.R[1] = 0xFFFFFFFF;
	cpu.R[5] = 0x50000000;
	cpu.ARM_SWP(5, 0, 1, true);
	return 0;
}