#include "./include/cpu.hpp"
using namespace ALU;
//S
#define RESET_VECTOR 0x0
#define UND_HANDLER 0x4
#define SVC_HANDLER 0x8


void CPU::exception(Exception e){
	bool thumb = R.CPSR.Thumb;
	if(e==Exception::RESET) {
		R.SPSR_svc = R.CPSR;
		R.CPSR.mode = CPUMode::SVC;
		R.CPSR.FIQ_disable = true;
	}
	else if(e==Exception::UNDEFINED){
		R.SPSR_und = R.CPSR;
		R.CPSR.mode = CPUMode::UND;
		if(thumb) R[14]=R[15]+2;
		else R[14]=R[15]+4;
	}
	else if(e==Exception::eSVC){
		R.SPSR_svc = R.CPSR;
		R.CPSR.mode = CPUMode::SVC;
		if(thumb) R[14]=R[15]+2;
		else R[14]=R[15]+4;
	}
	else if(e==Exception::ABT_PREFETCH){
		R.SPSR_abt = R.CPSR;
		R.CPSR.mode = CPUMode::ABT;
		R[14]=R[15]+4;
	}
	else if(e==Exception::ABT_DATA){
		R.SPSR_abt = R.CPSR;
		R.CPSR.mode = CPUMode::ABT;
		R[14]=R[15]+8;
	}
	else if(e==Exception::eIRQ){
		R.SPSR_irq = R.CPSR;
		R.CPSR.mode = CPUMode::IRQ;
		R[14]=R[15]+4;
	}
	else if(e==Exception::eFIQ){
		R.SPSR_fiq = R.CPSR;
		R.CPSR.mode = CPUMode::FIQ;
		R.CPSR.FIQ_disable = true;
		R[14]=R[15]+4;
	}
	R.CPSR.IRQ_disable = true;
	R.CPSR.Thumb = false;
	
	//set pc to exception vector
	R[15] = e;
	
}


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
		exception(Exception::eSVC);
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