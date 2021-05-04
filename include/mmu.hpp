#pragma once
//#include "mmu.hpp"

#define BIOS_LO 0x0
#define BIOS_HI 0x4000
#define WRAM0_LO 0x02000000
#define WRAM0_HI 0x02040000
#define WRAM1_LO 0x03000000
#define WRAM1_HI 0x03008000
#define IO_LO 0x04000000
#define IO_HI 0x040003ff
#define PRAM_LO 0x05000000
#define PRAM_HI 0x05000400
#define VRAM_LO 0x06000000
#define VRAM_HI 0x06017FFF
#define OAM_LO 0x07000000
#define OAM_HI 0x070003FF

#define GAMEW0_LO 0x08000000
#define GAMEW0_HI 0x09FFFFFF
#define GAMEW1_LO 0x0A000000
#define GAMEW1_HI 0x0BFFFFFF
#define GAMEW2_LO 0x0C000000
#define GAMEW2_HI 0x0DFFFFFF
#define GAME_SRAM_LO 0x0E000000
#define GAME_SRAM_HI 0X0E00FFFF

class MMU {
    public:
	//ram
	#include "bios.hpp"

	uint8_t * bios = bios_bin ;
	uint8_t wram0[0x40000];
	uint8_t wram1[0x8000];
	uint8_t io[0x3ff];
	//vram
	uint8_t pram[0x400];
	uint8_t vram[0x18000];
	uint8_t oam[0x400];
	uint32_t * memfault(){
		return 0;
	}
	
	void * map(uint32_t addr) {
		if(addr<BIOS_HI){
			return addr+bios;
		}
		else if(addr>=WRAM0_LO && addr<WRAM0_HI){
			return (int)(addr&0x7ffff) +wram0;
		}
		else if(addr>=WRAM1_LO&&addr<WRAM1_HI){
			return (int)(addr&0x7fff) +wram1;
		}
		else if(addr>=IO_LO&&addr<IO_HI){
			return (int)(addr&0x3ff) + io;
		}
		else if(addr>=PRAM_LO&&addr<PRAM_HI){
			return (int)(addr&0x3ff) + pram;
		}
		else if(addr>=VRAM_LO&&addr<VRAM_HI){
			return (int)(addr&0x1FFFF) + vram;
		}
		else if(addr>=OAM_LO&&addr<OAM_HI){
			return (int)(addr&0x3FF) + oam;
		}
		//TODO: gamepack
		
		return memfault();
	}
	uint32_t getWord(uint32_t addr){
		uint32_t * paddr  = static_cast<uint32_t*>(map(addr));
		return *paddr;
	}
	void setWord(uint32_t addr, uint32_t val){
		uint32_t * paddr  = static_cast<uint32_t*>(map(addr));
		*paddr=val;
	}
	
	uint8_t getByte(uint32_t addr) {
		uint8_t * paddr = static_cast<uint8_t*>(map(addr));
		return *paddr;
	}
	
	void setByte(uint32_t addr, uint8_t val) {
		uint8_t * paddr = static_cast<uint8_t*>(map(addr));
		*paddr = val;
	}
	
	uint16_t getHalf(uint32_t addr){
		uint16_t * paddr = static_cast<uint16_t*>(map(addr));
		return *paddr;
	}
	void setHalf(uint32_t addr, uint16_t val){
		uint16_t * paddr = static_cast<uint16_t*>(map(addr));
		*paddr = val;
	}
	
	
	//gamepak
};