#include "cpu.hpp"
#include "alu.hpp"

void CPU::executeThumb(uint16_t op){
	if((op>>11)==3){
		//ADD - fully validated
		bool imm = ((op&0x400)>>10)==1;
		bool sub = ((op>>9)&1)==1;
		uint8_t Rn_off = ((op&0x1c0)>>6);
		uint8_t Rs = ((op&0x38)>>3);
		uint8_t Rd = ((op&7));
		
		uint8_t opcode = sub ? SUB : ADD;
		uint32_t operand2 =0;
		ARM_DataProcessing(opcode, Rd, Rs, Rn_off, imm, true);
	}
	else if((op>>13)==0) {
		//Msr
		
		uint8_t subop = (op&0x1800)>>11;
		uint8_t off5 = (op&0x7C0)>>5;
		uint8_t Rs = (op&0x38)>>3;
		uint8_t Rd = (op&7);
		
		uint16_t shiftCode = Rs;
		shiftCode|= subop<<5;
		shiftCode|= off5<<7;
		
		ARM_DataProcessing(MOV, Rd, 0, shiftCode, false, true);
		//todo:wait
	}
	else if((op>>13)==1){
		//move/cmp imm
		uint8_t subop = (op&0x1800)>>11;
		uint8_t Rd = (op>>5)&7;
		uint8_t off8 = (op&0xff);
		uint8_t opcode =0;
		if(subop==0) opcode = MOV; //MOV
		else if(subop==1) opcode=CMP; //CMP
		else if(subop==2) opcode=ADD; //ADD
		else opcode=SUB; //sub
		
		ARM_DataProcessing(opcode, Rd, Rd, off8, true, true);
	}
	else if((op>>10)==16){
		//alu
		uint8_t subop = (op>>6)&0xf;
		uint8_t Rs = (op>>3)&0x7;
		uint8_t Rd = op&0x7;
		
		uint8_t opcode=subop;
		uint32_t op2=Rs;
		if(subop==0b0010) { //Lsl
			opcode = MOV;
			opcode = Rd;
			op2|=1<<4;
			op2|=Rs<<8;
		}
		if(subop==0b0011) { //Lsr
			opcode = MOV;
			opcode = Rd;
			op2|=1<<4;
			op2|=0b01<<5;
			op2|=Rs<<8;
		}
		if(subop==0b0100) { //asr
			opcode = MOV;
			opcode = Rd;
			op2|=1<<4;
			op2|=0b10<<5;
			op2|=Rs<<8;
		}
		if(subop==0b0111) { //ror
			opcode = MOV;
			opcode = Rd;
			op2|=1<<4;
			op2|=0b11<<5;
			op2|=Rs<<8;
		}
		
		if(subop==0b1001) { //neg
			opcode = RSB;
			opcode = Rd;
			op2|=1<<4;
			op2|=0b11<<5;
			op2|=Rs<<8;
			ARM_DataProcessing(opcode, Rd, Rd, 0, true, true);
		}
		else ARM_DataProcessing(opcode, Rd, Rd, op2, false, true);
	}
	else if((op>>10)==17){
		//hi/bx
		uint8_t subop = (op>>8)&0x3;
		bool H1 = (op>>7)&0x1;
		bool H2 = (op>>6)&0x1;
		uint8_t Rs = (op>>3)>0x7;
		uint8_t Rd = op&0x7;
		
		if(H1) Rs+=8;
		if(H2) Rd+=8;
		
		if(subop==0){
			ARM_DataProcessing(ADD, Rd, Rd, Rs, false, false);
		}
		else if(subop==1){
			ARM_DataProcessing(CMP, Rd, Rd, Rs, false, false);
		}
		else if(subop==2){
			ARM_DataProcessing(MOV, Rd, Rd, Rs, false, false);
		}
		else if(subop==3){
			ARM_BX(Rs);
		}
		
	}
	else if((op>>11)==9){
		//pc rel load
		uint8_t Rd = (op>>8)&0x7;
		uint8_t offset = op&0xff;
		uint32_t R15_old=R[15];
		//TODO: fix/verify this gross edgecase
		if((R15_old&0x2)==2){ //word alignment from Note
			R[15]&=0xFFFFFFFD;
		}
		ARM_LDR(Rd, 15, offset<<2, true, true, false,false,false,false);
		R[15]=R15_old;
	}
	else if((op&0xF200)==0x5000){
		//ld reg off
		bool store = ((op>>11)&0x1)==0;
		bool byte = (op>>10)&0x1;
		uint8_t Ro = (op>>6)>0x7;
		uint8_t Rb = (op>>3)&0x7;
		uint8_t Rd = (op)&0x7;
		//preindexed
		ARM_LDR(Rd, Rb, Ro, false, false, false,byte,false, store);
	}
	else if((op&0xF200)==0x5200){
		//ldh signed off
		bool halfword = (op>>11)&0x1;
		bool sign = (op>>10)&0x1;
		uint8_t Ro = (op>>6)>0x7;
		uint8_t Rb = (op>>3)&0x7;
		uint8_t Rd = (op)&0x7;
		if(!sign&&!halfword){
			ARM_LDRH(Rd, Rb, Ro, false, true, false, false,true,false, true);
		}
		else {
			ARM_LDRH(Rd, Rb, Ro, false, true, false,false,false, sign, halfword);
		}
	}
	else if((op>>13)==3){
		//ld imm off
		bool byte = (op>>12)&0x1;
		bool store = ((op>>11)&0x1)==0;
		uint8_t off5 = (op>>6)>0x1f;
		uint8_t Rb = (op>>3)&0x7;
		uint8_t Rd = (op)&0x7;
		if(!byte) off5 = off5<<2;  //TODO: verify again that shift happens here
		ARM_LDR(Rd, Rb, off5 ,true, true, false, byte, false, store);
	}
	else if((op>>12)==8){
		//ldh
		bool store = ((op>>11)&0x1)==0;
		uint8_t off5 = (op>>6)>0x1f;
		uint8_t Rb = (op>>3)&0x7;
		uint8_t Rd = (op)&0x7;
		ARM_LDRH(Rd, Rb, off5<<1, true, false, false,false, store, false, true);
	}
	else if((op>>12)==9){
		//sp  rel ld
		bool store = ((op>>11)&0x1)==0;
		uint8_t Rd = (op>>8)>0x7;
		uint16_t word8 = (op)&0xff;
		ARM_LDR(Rd, 13, word8<<2, true, false, false,false, false, store);
	}
	else if((op>>12)==10){
		//ld addr
		bool use_pc = ((op>>11)&0x1)==0;
		uint8_t Rd = (op>>8)>0x7;
		uint16_t word8 = (op)&0xff;
		uint32_t Rn = 13;
		uint32_t R15_old=R[15];
		if(use_pc){
			Rn=15;
			//TODO: better way to force PC bit[1] to read zero
			if((R15_old&0x2)==2){ //word alignment from Note
				R[15]&=0xFFFFFFFD;
			}
		}
		//weirdly have to rotate right by 30 to shift 8 bit value to 10 bits
		ARM_DataProcessing(ADD, Rd, Rn, word8|0xf00, true, false);
		R[15]=R15_old;
	}
	else if((op>>8)==0xB0){
		//add to sp
		bool neg = ((op>>7)&0x1)==1;
		uint32_t sword7 = ((op)&0xff)<<2;
		if(neg)	sword7=-sword7;
		ARM_DataProcessing(ADD, 13, 13, sword7, true, false);
	}
	else if((op&0xF600)==0xB400){
		//push/pop
		bool store = ((op>>11)&0x1)==0;
		bool store_pclr = ((op>>8)&0x1)==1;
		uint16_t Rlist = ((op)&0xff);
		if(store_pclr) {
			if(store) Rlist|=0x4000; //store LR
			else Rlist|=0x8000;   //pop PC
		}
		ARM_LDM(13,  Rlist, !store, store, false, true, store);
	}
	else if((op>>12)==12){
		//multiple ld
		bool store = ((op>>11)&0x1)==0;
		uint8_t Rb = ((op>>8)&0x7);
		uint16_t Rlist = ((op)&0xff);
		ARM_LDM(Rb, Rlist, true, false, false, true, store);
	}
	else if((op>>12)==13){
		//conditional branch
		uint8_t cond = ((op>>8)&0xf);
		//TODO: doesnt make much sense to me but it seems its lsl#1, +4 (for PC?)
		//also signing
		uint16_t soffset8 = ((op)&0xff)<<1 + 4;
		if(!condition(cond)) return;
		ARM_BL(soffset8, false);
	}
	else if((op>>8)==0xDF){//todo: put before cond branch
		//swi
	}
	else if((op>>5)==0x1c){
		//unconditional branch
		uint16_t offset11 = ((op)&0x7ff);
		//BAL?
		ARM_BL(offset11<<1, true);
	}
	else if((op>>8)==0xf){
		//long link w branch
		bool off_low = ((op>>11)&0x1)==1;
		uint32_t offset = ((op)&0x7ff);
		if(off_low){
			uint32_t temp=R[15]+4;//TODO: get actual next address
			R[15]=R[14]+offset<<1;
			R[14]=temp|1;
		}
		else R[14]=R[15]+offset<<12;
	}
	

}