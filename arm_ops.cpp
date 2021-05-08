#include "cpu.hpp"

class ARM_OP{
	virtual void execute();
};
/*
class ARM_BX{
	ARM_BX(uint8_t Rn){
		R[15]=R[Rn];
		R.CPSR.Thumb = R[Rn]&1;
		swait=2;
		nwait=1;
	}
}*/

void CPU::ARM_DataProcessing(uint8_t opcode, uint32_t Rd, uint32_t Rn, uint32_t operand2,
		 bool imm, bool setcond) {
	if(Rd==15) { 
		swait++;
		nwait++;
	}
	//TODO:might wish to pull out shifting to make THUMB mapping easier
	//i.e. use value instead of operand2 as argument
	int flagsOut=0;
	unsigned int value = 0;
	if(imm){
		value = ALU::shifter(operand2&0xff,(operand2>>8)*2, 0b11 );
	}
	else {
		value = handleshift(operand2);
	}
	int tst = 0;
	bool logical = false;
	bool test = false;
	switch(opcode) {
		case AND:
			logical = true;
			R[Rd]= R[Rn]&value;
			break;
		case EOR: //EOR/XOR
			logical = true;
			R[Rd]= R[Rn]^value;
			break;
		case SUB: //sub
			R[Rd] = ALU::ADD(R[Rn],-value);
			break;
		case RSB://rsb
			R[Rd] = ALU::ADD(value,-R[Rn]);
			break;
		case ADD: //ADD
			R[Rd] = ALU::ADD(R[Rn],value);
			break;
		case ADC: //ADC
			R[Rd] = ALU::ADD(R[Rn], value+R.CPSR.C);
			break;
		case SBC: //sbc
			R[Rd] = ALU::ADD(R[Rn], -value+R.CPSR.C-1);
			break;
		case RSC: //rsc
			R[Rd] = ALU::ADD(value,-R[Rn]+R.CPSR.C-1);
			break;
		case TST: //tst
			logical = test = true;
			tst = R[Rn]&value;
			break;
		case TEQ: //TEQ
			logical = test= true;
			tst = R[Rn]^value;
			break;
		case CMP: //CMP
			logical = test = true;
			tst = R[Rn]-value;
			break;
		case CMN: //CMN
			logical = tst =true;
			tst = R[Rn]+value;
			break;
		case ORR://ORR
			logical = true;
			R[Rd] = R[Rn]|value;
			break;
		case MOV: //MOV
			R[Rd] = value;
			break;
		case BIC: //BIC
			logical = true;
			R[Rd] = R[Rn]&(~value);
			break;
		case MVN: //MVN
			logical = true;
			R[Rd] = ~value;
			break;
		default:
			trap();
	}
	if(setcond) { 
		clearFlags();
		if(Rd==15){
			//handle Special caSe for atomic change
			R.CPSR=R.SPSR();
			return;
		}
		if(logical){
			if(test) {
				if(tst==0) R.CPSR.Z=1;
				if(tst>>31==1) R.CPSR.N=1;
			}else {
				if(R[Rd]==0) R.CPSR.Z=1;
				if(R[Rd]>>31==1) R.CPSR.N=1;
			}
			if(ALU::shiftCarry()==1) R.CPSR.Z=1;
		}
		else{
			if(ALU::aluOverflow()==1) R.CPSR.V=1; //TODO: make sure this is valid
			if(ALU::aluCarry()==1) R.CPSR.C=1;
			if(R[Rd]==0) R.CPSR.Z=1;
			if((R[Rd]>>31)==1) R.CPSR.N=1;
			
		}
	}
}

void CPU::ARM_BX(uint8_t Rn){
	R[15]=R[Rn];
	R.CPSR.Thumb = R[Rn]&1;
	swait=2;
	nwait=1;
}

void CPU::ARM_BL(uint32_t off, bool link){
	if(link){
		R[14]=R[15]+4; //TODO: properly get PC accounting for prefetch
		R[14]&=0xfffffffc; //clearing r14[1:0]
	}
	off=off<<2;
	//sign extend
	if(off&0x2000000){
		off|=0xFE000000;
	}
	R[15]+=off;
	swait=2;
	nwait=1;
}

void CPU::ARM_MUL(uint8_t Rd, uint8_t Rm, uint8_t Rs,uint8_t Rn, bool accumulate, bool setCond){
	if(Rd==15||Rs==15||Rm==15) trap();
	if(Rd==Rm||Rd==Rs) trap();
	
	uint32_t a = 0;
	if(accumulate) a=R[Rn];
	R[Rd] = R[Rm]*R[Rs] + a;
	if(setCond){
		if(R[Rd]>>31) R.CPSR.N=1; else R.CPSR.N=0;
		if(R[Rd]==0) R.CPSR.Z=1; else R.CPSR.Z=0;
	}
	swait=1;
	iwait=multCycles(R[Rs])+2;
}

void CPU::ARM_MULL(uint8_t Rd_hi, uint8_t Rd_lo, uint8_t Rs,uint8_t Rm, bool accumulate,
			bool setCond, bool sign ){
	if(Rd_hi==15||Rd_lo==15||Rs==15||Rm==15) trap();
	if(Rd_hi==Rd_lo || Rd_hi==Rm||Rd_lo==Rm) trap();
	uint64_t out =0;
	if(sign) {
		//TODO:smarter sign extension
		int64_t sRm = R[Rm];
		if((sRm&0xf0000000)==0x80000000) sRm|=0xffffffff00000000LL;
		int64_t sRs = R[Rs];
		if((sRs&0xf0000000)==0x80000000) sRs|=0xffffffff00000000LL;
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
		if(R[Rd_hi]>>31) R.CPSR.N=1; else R.CPSR.N=0;
		if(R[Rd_hi]==0&&R[Rd_lo]==0) R.CPSR.Z=1; else R.CPSR.Z=0;
	}
	swait = 1;
	iwait = multCycles(R[Rs])+1;
	if(accumulate) iwait++;
}

void CPU::ARM_LDR(uint8_t Rd, uint8_t Rn, uint32_t off, bool imm, bool post, bool down,
 bool byte, bool writeback, bool store){
 	uint32_t value=0;
	if(imm) value = off;
	else value = handleshift(off);
	
	//TODO: handle R15 cases
	
	if(down) value=-value;
	uint32_t ld_addr =0;
	if(post){
		ld_addr=R[Rn];
		R[Rn]+=value;
		if(writeback){
			//TODO: post indexed write back mode priveleged edgecase
		}
	}
	else {
		//R[Rn]+=value;
		ld_addr=R[Rn]+value;
		if(writeback) R[Rn]=ld_addr;
	}
	//TODO: bigend control signal?
	if(byte){
		if(store) mmu.setByte(ld_addr, R[Rd]&0xff);
		else R[Rd] = mmu.getByte(ld_addr);
	}
	else{
		if(store) {
			//always store word aligned
			int pc=0;
			if(Rd==15) pc=12;
			mmu.setWord(ld_addr&0xFFFFFFFC, R[Rd]+pc);
		}
		else {
			//rotate misaligned addresses into register
			uint8_t rotate = (ld_addr&0x3)*8;
			R[Rd] = ALU::rotr32(mmu.getWord(ld_addr),rotate);
		}
	}
	
	if(store) {
		nwait=2;
	}
	else {
		iwait=nwait=swait=1;
		if(Rd==15){
			swait++;
			nwait++;
		}
	}

 }
 
 void CPU::ARM_LDRH(uint8_t Rd, uint8_t Rn, uint32_t off, bool imm, bool post, bool down, 
 	bool writeback, bool store, bool sign, bool halfwords) {
	uint32_t ld_addr=0;
	if(post){
		ld_addr=R[Rn];
		R[Rn]+=off;
		if(writeback){
			//do nothing
		}
	}
	else {
		ld_addr=R[Rn]+off;
		if(writeback) R[Rn]=ld_addr;
	}

	if(halfwords){
		if(store) {
			if(sign) trap();
			uint8_t pc=0;
			if(Rd==15) pc=12;
			mmu.setHalf(ld_addr, R[Rd]+pc);
	
		}
		else {
			R[Rd] = mmu.getHalf(ld_addr);
			if(sign){
				if(R[Rd]>>7 == 1) R[Rd]|=0xffffff00;
			}
		}
	}
	else{
		if(sign){
			if(store) trap(); //no signed stores;
			R[Rd] = mmu.getByte(ld_addr);
			if(R[Rd]>>7 == 1) R[Rd]|=0xffffff00;
		}
		else {
			//swp? probably need to decode swp first to prevent this
		}

	}
	swait=nwait=iwait=1;
	if(Rd==15) swait=nwait=2;
 }

void CPU::ARM_LDM(uint8_t Rn, uint16_t Rlist, bool post, bool down, bool psr, 
	bool writeback, bool store){

	uint8_t Regs[16];
	int rcount =0;
	bool containsPC=false;
	for(int i=0; i<16; i++){
		if(Rlist&(1<<i)){
			Regs[rcount++] = i;
			if(i==15) {
				containsPC=true;
			}
		}
	}
	
	if(psr&&!store&&containsPC){
		//load spsr when r15 loaded
	}
	if(psr&&store&&containsPC){
		//use user bank, no writeback should be used 
	}
	else if(psr&&!containsPC){
		//use user bank
	}
	
	uint32_t base = R[Rn];
	
	uint32_t off=0;
	if(down) {
		for(int i=rcount-1; i>=0; i--){
			if(!post) off-=4;
			if(store) mmu.setWord(base+off, R[Regs[i]]);
			else R[Regs[i]] = mmu.getWord(base+off);
			if(post) off-=4;
		}
	}
	else {
		for(int i=0;i<rcount; i++){
			if(!post) off+=4;
			if(store) mmu.setWord(base+off, R[Regs[i]]);
			else R[Regs[i]] = mmu.getWord(base+off);
			if(post) off+=4;
		}
	}
	
	if(writeback) {
		R[Rn]+=off;
	}
	
	//waits
	if(store){
		swait=(rcount-1);
		nwait=2;
	}
	else {
		swait=rcount;
		nwait=iwait=1;
		if(containsPC){
			nwait++;
			swait++;
		} 
	}
}
void CPU::ARM_SWP(uint8_t Rd, uint8_t Rn, uint8_t Rm, bool byte){
	if(Rd==15||Rn==15||Rm==15) trap();
	//TODO: intended to implement LDR followed by str
	//is it possible that the behavior is different doing it like this?
	if(byte){
		uint8_t tmp = mmu.getByte(R[Rn]);
		mmu.setByte(R[Rn],R[Rm]&0xff);
		R[Rd] = tmp;
	}
	else {
		uint32_t tmp = mmu.getWord(R[Rn]);
		mmu.setWord(R[Rn],R[Rm]);
		R[Rd] = tmp;
	}
	
	iwait=swait=1;
	nwait=2;
	
}

//TODO: verify mode perms
void CPU::ARM_MRS(uint8_t Rd, bool spsr) {
	if(spsr) {
		R[Rd] = R.SPSR().asInt();
	}
	else {
		R[Rd] = R.CPSR.asInt();
	}
	swait=1;
}

void CPU::ARM_MSR_REG(uint8_t Rm, bool spsr){
	if(spsr) {
		R.SPSR().fromInt(R[Rm]);
	}
	else {
		R.CPSR.fromInt(R[Rm]);
	}
	swait=1;
}

//TODO:verify only flags are set and other behavior
void CPU::ARM_MSR_IMM(uint16_t operand2, bool spsr, bool imm){
	uint32_t value = 0;
	if(imm){
		value = ALU::shifter(operand2&0xff,(operand2>>8)*2, 0b11 );
	}
	else {
		value = handleshift(operand2);
	}
	
	if(spsr) {
		uint32_t old = R.SPSR().asInt() &0x0fffffff;
		R.SPSR().fromInt(old|(value&0xf0000000));
	}
	else {
		uint32_t old = R.CPSR.asInt() &0x0fffffff;
		R.CPSR.fromInt(old|(value&0xf0000000));
	}
	swait=1;
}