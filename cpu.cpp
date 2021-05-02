#include "./include/cpu.hpp"
using namespace std;
using namespace ALU;
//S

void CPU::clearFlags(){
	R[16]&=0x0FFFFFFF;
}
void CPU::setZ(){
	R[16]|=0x40000000;
}
void CPU::clearZ(){
	R[16]&=0xBFFFFFFF;
}
void CPU::setN(){
	R[16]|=0x80000000;
}
void CPU::clearN(){
	R[16]&=0x7FFFFFFF;
}
uint8_t CPU::getC(){
	return (R[16]&0x20000000)>>29;
}
void CPU::setC(){
	R[16]|=0x20000000;
}
void CPU::clearC(){
	R[16]&=0xDFFFFFFF;
}
void CPU::setV(){
	R[16]|=0x10000000;
}
void CPU::clearV(){
	R[16]&=0xEFFFFFFF;
}	
bool CPU::condition(int cond){
	int V=(R.CPSR>>28)&1;
	int C=(R.CPSR>>29)&1;
	int Z=(R.CPSR>>30)&1;
	int N=(R.CPSR>>31)&1;
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

void CPU::dataProcessing(int rem) {
		//TODO: MRs, R15 handling, other special cases
		bool imm = (rem>>25)==1;
		unsigned int opcode = (rem >>21)&0xf;
		bool setcond = ((rem>>20)&1)==1;
		unsigned int Rn = (rem>>16)&0xf;
		unsigned int Rd = (rem>>12)&0xf;
		unsigned int operand2 = (rem&0xfff);
		
		if(Rd==15) { 
			swait++;
			nwait++;
		}
		
		int flagsOut=0;
		unsigned int value = 0;
		if(imm){
			value = shifter(operand2&0xff,(operand2>>8)*2, 0b11 );
		}
		else {
			value = handleshift(operand2);
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
			clearFlags();
			if(logical){
				if(test) {
					if(tst==0) setZ();
					if(tst>>31==1) setN();
				}else {
					if(R[Rd]==0) setZ();
					if(R[Rd]>>31==1) setN();
				}
				if(shiftCarry()==1) setC();
			}
			else{
				if(aluOverflow()==1) setV(); //TODO: make sure this is valid
				if(aluCarry()==1) setC();
				if(R[Rd]==0) setZ();
				if((R[Rd]>>31)==1) setN();
				
			}
		}
}

void CPU::ARM_BX(uint8_t Rn){
	R[15]=R[Rn];
	swait=2;
	nwait=1;
}

void CPU::ARM_BL(uint32_t off, bool link){
	if(link){
		R[14]=R[15];
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
		if(R[Rd]>>31) setN(); else clearN();
		if(R[Rd]==0) setZ(); else clearZ();
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
		if(R[Rd_hi]>>31) setN(); else clearN();
		if(R[Rd_hi]==0&&R[Rd_lo]==0) setZ(); else clearZ();
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
			R[Rd] = rotr32(mmu.getWord(ld_addr),rotate);
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


void CPU::execute(unsigned int op){
	iwait=swait=nwait=0;
	unsigned int cond = op>>27;
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
	else if((rem&0x0f000000)==0x0a000000){
		//BL
		int off= (rem&0xFFFFFF) <<2;
		if(off>>25==1) off|=0xFE000000;
		bool link = (rem&0x1000000)==1;
		ARM_BL(off, link);
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
		bool down = ((rem>>21)&1)==0;
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
	//probably last in order
	else if((rem&0xc000000)==0x0) {
		//Data processing
		dataProcessing(rem);
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


int main(int argc, char* argv[]){
	loguru::init(argc, argv);
    LOG_F(INFO, "Hello from main.cpp!");
    LOG_F(INFO, "main function about to end!");
	CPU cpu;
	//cpu.execute(0xA800003);
	//cpu.R15=-1;
	for(int i=0; i<4; i++){
		cout << hex<<shifter(0xF0000001, 2, i)<<endl;
	}
	
	cpu.R[1]=-10;
	cpu.R[2]=20;
	cpu.R.CPSR=0x40000000;
	cpu.execute(0x100192);
	LOG_F(INFO, "pc=%d", cpu.R[15]);
	
	cpu.R[1]=-10;
	cpu.R[2]=20;
	cpu.R.CPSR=0x40000000;
	cpu.execute(0xD45192);
	cout << cpu.R[4]<<endl;
	cout <<cpu.R[5]<<endl;
	
	//flag tet
	cpu.R[1]=0xffffffff;
	cpu.R[2]=1;
	cpu.R.CPSR=0x40000000;
	cpu.execute(0x910002);
	
	cpu.R[1]=5;
	cpu.R[2]=3;
	cpu.R[3]=-9;
	cpu.R.CPSR=0x40000000;
	cpu.executeThumb(0x1f48);
	cpu.R[1]=0x0;
	cpu.R[16]=0x40000000;
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
	return 0;
}