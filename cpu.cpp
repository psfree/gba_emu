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
	//TODO: disable prefetch if pipeline not used
	uint32_t value=0;
	uint32_t shift = (operand2&0xff0)>>4;
	uint32_t Rm = operand2&0xf;
	uint8_t PC_prefetch=0;
	
	uint32_t shiftType=(shift>>1)&3;
	uint32_t shiftAmount=0;
	if((shift&1)==1){
		//shift from register
		if(Rm==15) PC_prefetch=12;
		uint32_t Rs = (shift&0xf0)>>4;
		shiftAmount = R[Rs]&0xff;
		value = shifter(R[Rm]+PC_prefetch, shiftAmount,shiftType);
		iwait++;
	}
	else{
		if(Rm==15) PC_prefetch=8;
		shiftAmount = shift>>3;
		value = shifter(R[Rm]+PC_prefetch, shiftAmount,shiftType);
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
		//TODO:double check immediates make sense
		//TODO: immediate being flipped vs ldr can lead to confusion  
		bool imm = ((rem>>22)&1)==1;
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
		trap();
	}
	else if((rem&0xE000000)==0xC000000){
		//LDC
		trap();
	}
	else if((rem&0xF000010)==0xE000010){
		//MRC
		trap();
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

/*
uint32_t ops[2]={0};
void fetch() {
	ops[0]=ops[1];
	ops[1]=mmu.getWord(R[15]);
}

void decode() {
	execute(ops[0]);
}

void exec() {
}

void pipelineFlush(){
}*/

void CPU::trap(){
	int*x=NULL;
	*x++;
}

#include <cstdlib>
#include <assert.h>

void testThumb_f2(CPU cpu){
	cpu.R[3]=-1;
	cpu.R[4]=55;
	cpu.R.CPSR.Z=1;
	//ADD R0, R3, R4
	cpu.executeThumb(0x18E0);
	assert(cpu.R[0]==54);
	
	cpu.R[2]=99;
	cpu.R.CPSR.Z=1;
	//SUB R6, R2, #6
	cpu.executeThumb(0x1F96);
	assert(cpu.R[6]==93);
}
void testThumb_f3(CPU cpu){
	cpu.R.CPSR.Z=1;
	//MOV   R0, #128
	cpu.executeThumb(0x2080);
	assert(cpu.R[0]==128);
	
	cpu.R[2]=62;
	cpu.R.CPSR.Z=0;
	//CMP   R2, #62
	cpu.executeThumb(0x2A3E);
	assert(cpu.R.CPSR.Z==1);
	cpu.R[2]=61;
	cpu.executeThumb(0x2A3E);
	assert(cpu.R.CPSR.Z==0);
	
	cpu.R[1]=9000;
	//ADD   R1, #255
	cpu.executeThumb(0x31FF);
	assert(cpu.R[1]==9255);
	
	cpu.R[6]=146;
	//SUB R6, #145
	cpu.executeThumb(0x3E91);
	assert(cpu.R[6]==1);
}
void testThumb_f4(CPU cpu){
	cpu.R[3]=593521;
	cpu.R[4]=1783;
	//EOR   R3, R4
	cpu.executeThumb(0x4063);
	assert(cpu.R[3]==592006);
	
	cpu.R[3]=593521;
	cpu.R[4]=5;
	//ROR   R3, R4
	cpu.executeThumb(0x41E3);
	assert(cpu.R[3]==0x88004873);
	
	cpu.R[3]=0;
	cpu.R[4]=5;
	//NEG R3, R4
	cpu.executeThumb(0x4263);
	assert(cpu.R[3]==-5);
	
	cpu.R[3]=6;
	cpu.R[4]=6;
	//CMP R3, R4
	cpu.executeThumb(0x42A3);
	assert(cpu.R.CPSR.Z==1);
	
	cpu.R[3]=6;
	cpu.R[4]=9;
	//MUL R3, R4
	cpu.executeThumb(0x4363);
	assert(cpu.R[3]==54);
	
}
void testThumb_f5(CPU cpu){
	//hi register operation
	cpu.R[15]=12;
	cpu.R[5]=8;
	//ADD PC, R5
	cpu.executeThumb(0x44AF);
	assert(cpu.R[15]==20);
	
	cpu.R[4]=12;
	cpu.R[12]=12;
	//CMP R4, R12
	cpu.executeThumb(0x4564);
	assert(cpu.R.CPSR.Z==1);
	
	//MOV R15, R14
	cpu.R[14]=98;
	cpu.executeThumb(0x46F7);
	assert(cpu.R[15]==98);

}

void testThumb_f6(CPU cpu){
	//LDR R3,[PC,#844]
	cpu.mmu.setWord(12+844, 0x0a0b0c0d);
	cpu.R[15]=12;
	cpu.executeThumb(0x4BD3);
	assert(cpu.R[3]==0x0a0b0c0d);
}
void testThumb_f7(CPU cpu){
	//STR   R3, [R2,R6]
	cpu.R[2]=10;
	cpu.R[6]=90;
	cpu.R[3]=0xaabbccdd;
	cpu.executeThumb(0x5193);
	assert(cpu.mmu.getWord(100)==0xaabbccdd);
	//LDRB  R2, [R0,R7]
	cpu.R[0]=50;
	cpu.R[7]=51;
	cpu.executeThumb(0x5DC2);
	assert(cpu.R[2]==0xcc);
}
void testThumb_f8(CPU cpu){
	//STRH  R4, [R3, R0] 
	cpu.R[3]=10;
	cpu.R[0]=90;
	cpu.R[4]=0xf0e0f0c0;
	cpu.executeThumb(0x521C);
	assert(cpu.mmu.getHalf(100)==0xf0c0);
	//LDSH  R3, [R4, R2] 
	cpu.R[3]=10;
	cpu.R[4]=90;
	cpu.R[2]=10;
	cpu.executeThumb(0x5EA3);
	assert(cpu.R[3]==0xfffff0c0);
	
	
}
void testThumb_f9(CPU cpu){
	//LDR R2, [R5,#116]
	cpu.R[5]=16;
	cpu.mmu.setWord(132, 0xAABBCCDD);
	cpu.executeThumb(0x6F6A);
	assert(cpu.R[2]==0xAABBCCDD);
	//STRB R1, [R0,#13]
	cpu.R[0]=3;
	cpu.R[1]=0xfffe;
	cpu.executeThumb(0x7341);
	assert(cpu.mmu.getByte(16)==0xfe);
}
void testThumb_f10(CPU cpu){
	//STRH R6, [R1, #56]
	cpu.R[1]=4;
	cpu.R[6]=0xDDCC;
	cpu.executeThumb(0x870E);
	assert(cpu.mmu.getHalf(60)==0xDDCC);
	//LDRH R4, [R7, #4]
	cpu.R[7]=56;
	cpu.executeThumb(0x88BC);
	assert(cpu.R[4]==0xDDCC);
	
}
void testThumb_f11(CPU cpu){
	//STR   R4, [SP,#492]
	cpu.R[13]=4;
	cpu.R[4]=0xDEADBEEF;
	cpu.executeThumb(0x947B);
	assert(cpu.mmu.getWord(496)==0xDEADBEEF);
}

void testThumb_f12(CPU cpu){
	//ADD   R2, PC, #572
	cpu.R[15]=33;
	cpu.executeThumb(0xA28F);
	assert(cpu.R[2]==605);
	//ADD   R6, SP, #212
	cpu.R[13]=33;
	cpu.executeThumb(0xAE35);
	assert(cpu.R[6]==245);
}
void testThumb_f13(CPU cpu){
	//ADD SP, #268
	cpu.R[13]=-4;
	cpu.executeThumb(0xB043);
	assert(cpu.R[13]==264);
	//ADD SP, #-104
	cpu.R[13]=108;
	cpu.executeThumb(0xB09A);
	assert(cpu.R[13]==4);
}
void testThumb_f14(CPU cpu){
	//PUSH  {R0-R4,LR}
	cpu.R[13]=100;
	cpu.R[0]=0;
	cpu.R[1]=1;
	cpu.R[2]=2;
	cpu.R[3]=3;
	cpu.R[4]=4;
	cpu.R[14]=14;
	cpu.executeThumb(0xB51F);
	//POP  {R0-R4,PC}
	cpu.R[0]=cpu.R[1]=cpu.R[2]=cpu.R[3]=cpu.R[4]=cpu.R[14]=0;
	cpu.executeThumb(0xBD1F);
	assert(cpu.R[15]==14);
	assert(cpu.R[0]==0);
	assert(cpu.R[1]==1);
	assert(cpu.R[2]==2);
	assert(cpu.R[3]==3);
	assert(cpu.R[4]==4);
}
void testThumb_f15(CPU cpu){
	//STMIA R0!, {R3-R7}
	cpu.R[0]=100;
	cpu.R[3]=3;
	cpu.R[4]=4;
	cpu.R[5]=5;
	cpu.R[6]=6;
	cpu.R[7]=7;
	cpu.executeThumb(0xC0F8);
	assert(cpu.mmu.getWord(cpu.R[0]-4)==7);
	assert(cpu.mmu.getWord(cpu.R[0]-8)==6);
	assert(cpu.mmu.getWord(cpu.R[0]-12)==5);
	assert(cpu.mmu.getWord(cpu.R[0]-16)==4);
	assert(cpu.mmu.getWord(cpu.R[0]-20)==3);
}

void testThumb_f16(CPU cpu){
	cpu.R.CPSR.Z=0;
	cpu.R.CPSR.N=1;
	cpu.R.CPSR.V=1;
	cpu.R[15]=100;
	cpu.executeThumb(0xDCFE);
	assert(cpu.R[15]==100);
	cpu.executeThumb(0xDC02);
	assert(cpu.R[15]==108);
	cpu.executeThumb(0xDCF4);
	assert(cpu.R[15]==88);
	cpu.R[15]=0x2da;
	cpu.executeThumb(0xDCF4);
	assert(cpu.R[15]==0x2c6);
}
void testThumb_f17(CPU cpu){
}
void testThumb_f18(CPU cpu){
}
void testThumb_f19(CPU cpu){
}
void testThumb_f20(CPU cpu){
}

void testThumb(CPU cpu){
	testThumb_f2(cpu);
	testThumb_f3(cpu);
	testThumb_f4(cpu);
	testThumb_f5(cpu);
	testThumb_f6(cpu);
	testThumb_f7(cpu);
	testThumb_f8(cpu);
	testThumb_f9(cpu);
	testThumb_f10(cpu);
	testThumb_f11(cpu);
	testThumb_f12(cpu);
	testThumb_f13(cpu);
	testThumb_f14(cpu);
	testThumb_f15(cpu);
	testThumb_f16(cpu);
	/*	testThumb_f17(cpu);
	testThumb_f18(cpu);
	testThumb_f19(cpu);
	testThumb_f20(cpu);
	*/
}

int main(int argc, char* argv[]){
	loguru::init(argc, argv);
    LOG_F(INFO, "Hello from main.cpp!");
    LOG_F(INFO, "main function about to end!");
	CPU cpu;
	string in;
	cpu.R.CPSR.Z=1;
	cpu.ARM_MRS(1,0);
	testThumb(cpu);
	while(getline(cin,in)) {
		//00005EE3
		long x = stol(in, nullptr,16);
		uint32_t instr = (uint32_t)x;
		//uint32_t in = (instr&0xff) << 24;
		//in|=(instr&0xff00) <<8;
		//in|=(instr&0xff0000) >>8;
		//in|=(instr&0xff000000) >>24;
		//cpu.execute(in);
		cpu.executeThumb(instr);
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