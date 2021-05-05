#include "cpu.hpp"
#include "alu.hpp"

using namespace ALU;
void CPU::executeThumb(uint16_t op){
	if((op>>11)==3){
		//ADD - fully validated
		bool imm = ((op&0x400)>>10)==1;
		bool sub = ((op>>9)&1)==1;
		uint8_t Rn_off = ((op&0x1c0)>>6);
		uint8_t Rs = ((op&0x38)>>3);
		uint8_t Rd = ((op&7));
		
		uint8_t opcode = sub ? 2 : 4;
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
		
		ARM_DataProcessing(0b1101, Rd, 0, shiftCode, false, true);
		//todo:wait
	}
	else if((op>>13)==1){
		uint8_t subop = (op&0x1800)>>11;
		uint8_t Rd = (op>>5)&7;
		uint8_t off8 = (op&0xff);
		//move/cmp imm
		uint8_t opcode =0;
		if(subop==0) opcode = 0b1101; //MOV
		else if(subop==1) opcode=0b1010; //CMP
		else if(subop==2) opcode=0b0100; //ADD
		else opcode=0b0010; //sub
		
		ARM_DataProcessing(opcode, Rd, Rd, off8, true, true);
	}
	else if((op>>10)==16){
		//alu
	
	}
	else if((op>>10)==17){
		//hi/bx
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