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
	}
	else if((op&0xF200)==0x5000){
		//ld reg off
	}
	else if((op&0xF200)==0x5200){
		//ldh signed off
	}
	else if((op>>13)==3){
		//ld imm off
	}
	else if((op>>12)==8){
		//ldh
	}
	else if((op>>12)==9){
		//sp  rel ld
	}
	else if((op>>12)==10){
		//ld addr
	}
	else if((op>>8)==0xB0){
		//add to sp
	}
	else if((op&0xF600)==0xB400){
		//push/pop
	}
	else if((op>>12)==12){
		//multiple ld
	}
	else if((op>>12)==13){
		//conditional branch
	}
	else if((op>>8)==0xDF){
		//swi
	}
	else if((op>>5)==0x1c){
		//unconditional branch
	}
	else if((op>>8)==0xf){
		//long link w branch
	}
	

}