#include "alu.hpp"
namespace ALU {
	static inline uint32_t rotr32 (uint32_t n, unsigned int c)
	{
	  const unsigned int mask = (CHAR_BIT*sizeof(n) - 1);

	  // assert ( (c<=mask) &&"rotate by type width or more");
	  c &= mask;
	  return (n>>c) | (n<<( (-c)&mask ));
	}
	
	
	
	uint8_t aCarry=0;
	uint8_t aOverflow=0;
	uint32_t ADD(uint64_t a, uint64_t b){
		uint64_t total = a +b;
		uint32_t itotal=total;
		bool aneg = (a>>31)==1;
		bool bneg = (b>>31)==1;
		bool tneg = (itotal>>31)==1;
		if(aneg==bneg && aneg!=tneg) aOverflow = 1;
		else aOverflow=0;
		aCarry = total>>32;
		return itotal;
	}
	
	uint8_t sCarry=0;
	uint32_t shifter(uint32_t in, uint32_t c, uint32_t type) {
		if(c==0) {
			//sCarry = getC(); //TODO: IF LsL#0
			return in;
		}
		switch(type){
			case 0b00: //ll
				if(c==32) { 
					sCarry = in&1;
					return 0;
				}
				else if(c>32){
					sCarry = 0;
					return 0;
				}
				sCarry = (in>>(32-c))&1;
				return in << c;
			case 0b01: //lr
				if(c==32) { 
					sCarry = (in>>31)&1;
					return 0;
				}
				else if(c>32){
					sCarry = 0;
					return 0;
				}
				sCarry = (in>>(c-1))&1;
				return in >> c;
			case 0b10: //ar
				if(c>31){
					sCarry = (in>>31)&1;
					return 0xFFFFFFFF;
				}
				sCarry = (in>>(c-1))&1;
				return (uint32_t)(((signed int)in)>>c);
			case 0b11: //rr
				if(c==32) { 
					sCarry = (in>>31)&1;
					return in;
				}
				else if(c>32){
					c=c%32;
				}
				int out = rotr32(in, c);
				sCarry= (out>>31);
				return out;
		}
		return 0;
	}
	
	uint8_t aluCarry(){
		return aCarry;
	}
	uint8_t aluOverflow(){
		return aOverflow;
	}
	uint8_t shiftCarry(){
		return sCarry;
	}
	
};