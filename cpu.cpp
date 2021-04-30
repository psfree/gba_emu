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

void CPU::execute(unsigned int op){
	iwait=swait=nwait=0;
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
		swait=2;
		nwait=1;
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
		swait=2;
		nwait=1;
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
		swait=1;
		iwait=multCycles(R[Rs])+2;
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
		swait = 1;
		iwait = multCycles(R[Rs])+1;
		if(accumulate) iwait++;
		
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
		
		uint32_t value=0;
		if(imm){
			value = off;
		}
		else {
			value = handleshift(off);
		}
		if(down){
			value=-value;
		}
		//TODO: post indexed write back mode priveleged
		uint32_t ld_addr =0;
		if(post){
			ld_addr=R[Rn];
			R[Rn]+=value;
			if(writeback){
				//TODO: post indexed write back mode priveleged edgecase
			}
		}
		else {
			R[Rn]+=value;
			ld_addr=R[Rn]+value;
			if(writeback) R[Rn]=ld_addr;
		}
		
		if(byte){
		}
		else{
			//TODO: word alignment 
			MMU m;
			m.setWord(ld_addr, 0xffffffff);
			R[Rd] = m.getWord(ld_addr);
		}
		
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
	cpu.R15=-1;
	for(int i=0; i<4; i++){
		cout << hex<<shifter(0xF0000001, 2, i)<<endl;
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
	
	cpu.R[1]=5;
	cpu.R[2]=3;
	cpu.R[3]=-9;
	cpu.R[16]=0x40000000;
	cpu.executeThumb(0x1f48);
	cpu.R[1]=0x0;
	cpu.R[16]=0x40000000;
	cpu.execute(0xE5910000);
	
	
	return 0;
}