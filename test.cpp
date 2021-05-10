#include <cstdlib>
#include <assert.h>
#include "cpu.hpp"
using namespace std;
using namespace ALU;

void testThumb_f2(CPU& cpu){
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
void testThumb_f3(CPU& cpu){
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
void testThumb_f4(CPU& cpu){
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
void testThumb_f5(CPU& cpu){
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

void testThumb_f6(CPU& cpu){
	//LDR R3,[PC,#844]
	cpu.mmu.setWord(12+844, 0x0a0b0c0d);
	cpu.R[15]=12;
	cpu.executeThumb(0x4BD3);
	assert(cpu.R[3]==0x0a0b0c0d);
}
void testThumb_f7(CPU& cpu){
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
void testThumb_f8(CPU& cpu){
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
void testThumb_f9(CPU& cpu){
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
void testThumb_f10(CPU& cpu){
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
void testThumb_f11(CPU& cpu){
	//STR   R4, [SP,#492]
	cpu.R[13]=4;
	cpu.R[4]=0xDEADBEEF;
	cpu.executeThumb(0x947B);
	assert(cpu.mmu.getWord(496)==0xDEADBEEF);
}

void testThumb_f12(CPU& cpu){
	//ADD   R2, PC, #572
	cpu.R[15]=33;
	cpu.executeThumb(0xA28F);
	assert(cpu.R[2]==605);
	//ADD   R6, SP, #212
	cpu.R[13]=33;
	cpu.executeThumb(0xAE35);
	assert(cpu.R[6]==245);
}
void testThumb_f13(CPU& cpu){
	//ADD SP, #268
	cpu.R[13]=-4;
	cpu.executeThumb(0xB043);
	assert(cpu.R[13]==264);
	//ADD SP, #-104
	cpu.R[13]=108;
	cpu.executeThumb(0xB09A);
	assert(cpu.R[13]==4);
}
void testThumb_f14(CPU& cpu){
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
void testThumb_f15(CPU& cpu){
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

void testThumb_f16(CPU& cpu){
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
void testThumb_f17(CPU& cpu){
	//TODO: swi
	return;
}
void testThumb_f18(CPU& cpu){
	//b #0
	cpu.R[15]=100;
	cpu.executeThumb(0xE7FE);
	assert(cpu.R[15]==100);
	//b #-0x18
	cpu.executeThumb(0xE7F2);
	assert(cpu.R[15]==76);
}
void testThumb_f19(CPU& cpu){
	//BL 0x6bcffa
	cpu.R[15]=0;
	cpu.executeThumb(0xF6BC);
	cpu.executeThumb(0xFFFD);
	assert(cpu.R[15]==0x6bcffa);
}
void testThumb_f1(CPU& cpu){
	//LSR   R2, R5, #27
	cpu.R[5]=0x80000000;
	cpu.executeThumb(0xEEA);
	assert(cpu.R[2]==16);
}


void testThumb(CPU& cpu){
	testThumb_f1(cpu);
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
	testThumb_f17(cpu);
	testThumb_f18(cpu);
	testThumb_f19(cpu);
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