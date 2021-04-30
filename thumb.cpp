#include <cpu.hpp>
void executeThumb(uint16_t op){
	if((op>>13)==0) {
		//Msr
		
		uint8_t subop = (op&0x1800)>>11;
		uint8_t off5 = (op&0x7C0)>>5;
		uint8_t Rs = (op&0x38)>>3;
		uint8_t Rd = (op&7);
		if(subop==0){
			//lsl
			R[Rd] = shifter(R[Rm],off5,0b00);
			
		}
		else if(subop==1){
			//lsr
			R[Rd] = shifter(R[Rm],off5,0b01);
		}
		else if(subop==2){
			//asr
			R[Rd] = shifter(R[Rm],off5,0b10);
		}
		if(R[Rd]==0) setZ();
		if(R[Rd]>>31==1) setN();
		if(shiftCarry()==1) setC();
	
	}
	else if((op>>11)==3){
		//add
	}
	else if((op>>13)==1){
		//move/cmp imm
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