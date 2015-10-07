/**
 * @file      sparc_isa.cpp
 * @author    Sandro Rigo
 *            Marcus Bartholomeu
 *
 *            The ArchC Team
 *            http://www.archc.org/
 *
 *            Computer Systems Laboratory (LSC)
 *            IC-UNICAMP
 *            http://www.lsc.ic.unicamp.br
 *
 * @version   1.0
 * @date      Mon, 19 Jun 2006 15:50:50 -0300
 * 
 * @brief     The ArchC SPARC-V8 functional model.
 * 
 * @attention Copyright (C) 2002-2006 --- The ArchC Team
 *
 */

//IMPLEMENTATION NOTES:
// 1. readReg(RB, 0) returns always 0, so in condition codes instructions
//    a temporary is necessary or else it will read 0
// 2. Register windows: 4 bits for window (16 windows) + 4 bits for register = 8 bit
//    using char type

#include  "sparc_isa.H"
#include  "sparc_isa_init.cpp"
#include  "sparc_bhv_macros.H"

//If you want debug information for this model, uncomment next line
//#define DEBUG_MODEL
#include "ac_debug_model.H"
#include "ansi-colors.h" 

// Namespace for sparc types.
using namespace sparc_parms;

static int processors_started = 0;
#define DEFAULT_STACK_SIZE (256*1024)

#ifdef SLEEP_AWAKE_MODE
/*********************************************************************************/
/* SLEEP / AWAKE mode control                                                    */
/* INTR_REG may store 1 (AWAKE MODE) or 0 (SLEEP MODE)                           */
/* if intr_reg == 0, the simulator will be suspended until it receives a         */   
/* interruption 1                                                                */    
/*********************************************************************************/
#define test_sleep() { if (intr_reg.read() == 0) ac_wait();  }
#else
#define test_sleep() {}
#endif

//!Generic instruction behavior method.
void ac_behavior( instruction )
{

  test_sleep();

  dbg_printf("----- PC=0x%x  NPC=0x%x ----- #executed=%lld\n", (unsigned) ac_pc.read(), (unsigned)npc.read(), ac_instr_counter);
}
 
//! Instruction Format behavior methods.
void ac_behavior( Type_F1 ){}
void ac_behavior( Type_F2A ){}
void ac_behavior( Type_F2B ){}
void ac_behavior( Type_F3A ){}
void ac_behavior( Type_F3B ){}
void ac_behavior( Type_FT ){}

//!User declared functions.

#define writeReg(addr, val) REGS[addr] = (addr)? ac_word(val) : 0
#define readReg(addr) (int)(REGS[addr])


inline void update_pc(bool branch, bool taken, bool b_always, bool annul, ac_word addr, ac_reg<unsigned>& ac_pc, ac_reg<ac_word>& npc)
{
  //Reference book: "Sparc Architecture, Assembly Language Programing, and C"
  //  Author: Richard P. Paul. Prentice Hall, Second Edition. Page 87

  // If (not to execute next instruction)

		if (branch && (!taken ||b_always) && annul) {
			if (taken) {
				npc = addr;
				dbg_printf(CB_RED "Branch Taken" C_RESET LF);
			}
			else {
				npc+=4;
			}
			dbg_printf("Delay instruction annuled\n");
			ac_pc = npc;
			npc+=4;
		}
		// else (next instruction will be executed)
		else {
			ac_pc = npc;
			if (taken) {
				npc = addr;
				dbg_printf(CB_RED "Branch Taken" C_RESET LF);
			}
			else {
				npc+=4;
			}
		}

}


//Use updatepc() only when needed
#ifdef NO_NEED_PC_UPDATE
#define update_pc(a,b,c,d,e, ac_pc, npc) /*nothing*/
#endif


void trap_reg_window_overflow(ac_memory* DATA_PORT, ac_regbank<256, ac_word, ac_Dword>& RB, ac_reg<unsigned char>& WIM)
{
  WIM = (WIM-0x10);
  int sp = (WIM+14) & 0xFF;
  int l0 = (WIM+16) & 0xFF;
  for (int i=0; i<16; i++) {
    DATA_PORT->write(RB.read(sp)+(i<<2), RB.read(l0+i));
  }
}


void trap_reg_window_underflow(ac_memory* DATA_PORT, ac_regbank<256, ac_word, ac_Dword>& RB, ac_reg<unsigned char>& WIM)
{
  int sp = (WIM+14) & 0xFF;
  int l0 = (WIM+16) & 0xFF;
  for (int i=0; i<16; i++) {
    RB.write(l0+i, DATA_PORT->read(RB.read(sp)+(i<<2)));
  }
  WIM = (WIM+0x10);
}


//!Function called before simulation start
void ac_behavior(begin)
{
  dbg_printf("@@@ begin behavior @@@\n");
  REGS[0] = 0;  //writeReg can't initialize register 0
  npc = ac_pc + 4;

  CWP = 0xF0;
 /* sp for multi-core platforms */ 
  writeReg(14,AC_RAM_END - 1024 - processors_started++ * DEFAULT_STACK_SIZE);

}

//!Function called after simulation end
void ac_behavior(end)
{
  dbg_printf("@@@ end behavior @@@\n");
}


/**************************************************
 * Instructions Behaviors                         *
 **************************************************/

//!Instruction call behavior method.
void ac_behavior( call )
{
  dbg_printf("call 0x%x\n", ac_pc+(disp30<<2));
  writeReg(15, ac_pc); //saves ac_pc in %o7(or %r15)
  update_pc(1,1,1,0, ac_pc+(disp30<<2), ac_pc, npc);
};

//!Instruction nop behavior method.
void ac_behavior( nop )
{
  dbg_printf("nop\n");
  update_pc(0,0,0,0,0, ac_pc, npc);
};

//!Instruction sethi behavior method.
void ac_behavior( sethi )
{
  dbg_printf("sethi 0x%x,r%d\n", imm22, rd);
  writeReg(rd, (imm22 << 10));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

///!Instruction ba behavior method.
void ac_behavior( ba )
{
  dbg_printf("ba 0x%x\n", ac_pc+(disp22<<2));
  update_pc(1,1,1,an, ac_pc+(disp22<<2), ac_pc, npc);
};

///!Instruction bn behavior method.
void ac_behavior( bn )
{
  dbg_printf("bn 0x%x\n", ac_pc+(disp22<<2));
  update_pc(1,0,0,an,0, ac_pc, npc);
};

///!Instruction bne behavior method.
void ac_behavior( bne )
{
  dbg_printf("bne 0x%x\n", ac_pc+(disp22<<2));
  update_pc(1, !PSR_icc_z, 0, an, ac_pc+(disp22<<2), ac_pc, npc);
};

///!Instruction be behavior method.
void ac_behavior( be )
{
  dbg_printf("be 0x%x\n", ac_pc+(disp22<<2));
  update_pc(1, PSR_icc_z, 0, an, ac_pc+(disp22<<2), ac_pc, npc);
};

///!Instruction bg behavior method.
void ac_behavior( bg )
{
  dbg_printf("bg 0x%x\n", ac_pc+(disp22<<2));
  update_pc(1, !(PSR_icc_z ||(PSR_icc_n ^PSR_icc_v)), 0, an, ac_pc+(disp22<<2), ac_pc, npc);
};

///!Instruction ble behavior method.
void ac_behavior( ble )
{
  dbg_printf("ble 0x%x\n", ac_pc+(disp22<<2));
  update_pc(1, PSR_icc_z ||(PSR_icc_n ^PSR_icc_v), 0, an, ac_pc+(disp22<<2), ac_pc, npc);
};

///!Instruction bge behavior method.
void ac_behavior( bge )
{
  dbg_printf("bge 0x%x\n", ac_pc+(disp22<<2));
  update_pc(1, !(PSR_icc_n ^PSR_icc_v), 0, an, ac_pc+(disp22<<2), ac_pc, npc);
};

///!Instruction bl behavior method.
void ac_behavior( bl )
{
  dbg_printf("bl 0x%x\n", ac_pc+(disp22<<2));
  update_pc(1, PSR_icc_n ^PSR_icc_v, 0, an, ac_pc+(disp22<<2), ac_pc, npc);
};

///!Instruction bgu behavior method.
void ac_behavior( bgu )
{
  dbg_printf("bgu 0x%x\n", ac_pc+(disp22<<2));
  update_pc(1, !(PSR_icc_c ||PSR_icc_z), 0, an, ac_pc+(disp22<<2), ac_pc, npc);
};

///!Instruction bleu behavior method.
void ac_behavior( bleu )
{
  dbg_printf("bleu 0x%x\n", ac_pc+(disp22<<2));
  update_pc(1, PSR_icc_c ||PSR_icc_z, 0, an, ac_pc+(disp22<<2), ac_pc, npc);
};

///!Instruction bcc behavior method.
void ac_behavior( bcc )
{
  dbg_printf("bcc 0x%x\n", ac_pc+(disp22<<2));
  update_pc(1, !PSR_icc_c, 0, an, ac_pc+(disp22<<2), ac_pc, npc);
};

///!Instruction bcs behavior method.
void ac_behavior( bcs )
{
  dbg_printf("bcs 0x%x\n", ac_pc+(disp22<<2));
  update_pc(1, PSR_icc_c, 0, an, ac_pc+(disp22<<2), ac_pc, npc);
};

///!Instruction bpos behavior method.
void ac_behavior( bpos )
{
  dbg_printf("bpos 0x%x\n", ac_pc+(disp22<<2));
  update_pc(1, !PSR_icc_n, 0, an, ac_pc+(disp22<<2), ac_pc, npc);
};

///!Instruction bneg behavior method.
void ac_behavior( bneg )
{
  dbg_printf("bneg 0x%x\n", ac_pc+(disp22<<2));
  update_pc(1, PSR_icc_n, 0, an, ac_pc+(disp22<<2), ac_pc, npc);
};

///!Instruction bvc behavior method.
void ac_behavior( bvc )
{
  dbg_printf("bvc 0x%x\n", ac_pc+(disp22<<2));
  update_pc(1, !PSR_icc_v, 0, an, ac_pc+(disp22<<2), ac_pc, npc);
};

///!Instruction bvs behavior method.
void ac_behavior( bvs )
{
  dbg_printf("bvs 0x%x\n", ac_pc+(disp22<<2));
  update_pc(1, PSR_icc_v, 0, an, ac_pc+(disp22<<2), ac_pc, npc);
};

//!Instruction ldsb_reg behavior method.
void ac_behavior( ldsb_reg )
{
  dbg_printf("ldsb_reg [r%d+r%d], r%d\n", rs1, rs2, rd);
  writeReg(rd, (int)(char) DATA_PORT->read_byte(readReg(rs1) + readReg(rs2)));
  dbg_printf("Result = 0x%x\n", readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

//!Instruction ldsh_reg behavior method.
void ac_behavior( ldsh_reg )
{
  dbg_printf("ldsh_reg [r%d+r%d], r%d\n", rs1, rs2, rd);
  writeReg(rd, (int)(short) DATA_PORT->read_half(readReg(rs1) + readReg(rs2)));
  dbg_printf("Result = 0x%x\n", readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

//!Instruction ldub_reg behavior method.
void ac_behavior( ldub_reg )
{
  dbg_printf("ldub_reg [r%d+r%d], r%d\n", rs1, rs2, rd);
  writeReg(rd, DATA_PORT->read_byte(readReg(rs1) + readReg(rs2)));
  dbg_printf("Result = 0x%x\n", readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

//!Instruction lduh_reg behavior method.
void ac_behavior( lduh_reg )
{
  dbg_printf("lduh_reg [r%d+r%d], r%d\n", rs1, rs2, rd);
  writeReg(rd, DATA_PORT->read_half(readReg(rs1) + readReg(rs2)));
  dbg_printf("Result = 0x%x\n", readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

//!Instruction ld_reg behavior method.
void ac_behavior( ld_reg )
{
  dbg_printf("ld_reg [r%d+r%d], r%d\n", rs1, rs2, rd);
  writeReg(rd, DATA_PORT->read(readReg(rs1) + readReg(rs2)));
  dbg_printf("Result = 0x%x\n", readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

//!Instruction ldd_reg behavior method.
void ac_behavior( ldd_reg )
{
  dbg_printf("ldd_reg [r%d+r%d], r%d\n", rs1, rs2, rd);
  int tmp = DATA_PORT->read(readReg(rs1) + readReg(rs2) + 4);
  writeReg(rd,   DATA_PORT->read(readReg(rs1) + readReg(rs2)    ));
  writeReg(rd+1, tmp);
  dbg_printf("Result = 0x%x\n", readReg(rd));
  dbg_printf("Result = 0x%x\n", readReg(rd+1));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

//!Instruction stb_reg behavior method.
void ac_behavior( stb_reg )
{
  dbg_printf("stb_reg r%d, [r%d+r%d]\n", rd, rs1, rs2);
  DATA_PORT->write_byte(readReg(rs1) + readReg(rs2), (char) readReg(rd));
  dbg_printf("Result = 0x%x\n", (char) readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

//!Instruction sth_reg behavior method.
void ac_behavior( sth_reg )
{
  dbg_printf("sth_reg r%d, [r%d+r%d]\n", rd, rs1, rs2);
  DATA_PORT->write_half(readReg(rs1) + readReg(rs2), (short) readReg(rd));
  dbg_printf("Result = 0x%x\n", (short) readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

//!Instruction st_reg behavior method.
void ac_behavior( st_reg )
{
  dbg_printf("st_reg r%d, [r%d+r%d]\n", rd, rs1, rs2);
  DATA_PORT->write(readReg(rs1) + readReg(rs2), readReg(rd));
  dbg_printf("Result = 0x%x\n", readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

//!Instruction std_reg behavior method.
void ac_behavior( std_reg )
{
  dbg_printf("std_reg r%d, [r%d+r%d]\n", rd, rs1, rs2);
  DATA_PORT->write(readReg(rs1) + readReg(rs2),     readReg(rd  ));
  DATA_PORT->write(readReg(rs1) + readReg(rs2) + 4, readReg(rd+1));
  dbg_printf("Result = 0x%x\n", readReg(rd));
  dbg_printf("Result = 0x%x\n", readReg(rd+1));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

//!Instruction ldstub_reg behavior method.
void ac_behavior( ldstub_reg )
{
  dbg_printf("atomic ldstub_reg r%d, [r%d+r%d]\n", rd, rs1, rs2);
  writeReg(rd, DATA_PORT->read_byte(readReg(rs1) + readReg(rs2)));
  DATA_PORT->write_byte(readReg(rs1) + readReg(rs2), 0xFF);
  update_pc(0,0,0,0,0, ac_pc, npc);
};

///!Instruction swap_reg behavior method.
void ac_behavior( swap_reg )
{
  dbg_printf("swap_reg r%d, [r%d+r%d]\n", rd, rs1, rs2);
  int swap_temp = DATA_PORT->read(readReg(rs1) + readReg(rs2));
  DATA_PORT->write(readReg(rs1) + readReg(rs2), readReg(rd));
  writeReg(rd, swap_temp);
  update_pc(0,0,0,0,0, ac_pc, npc);
};

///!Instruction sll_reg behavior method.
void ac_behavior( sll_reg )
{
  dbg_printf("sll_reg r%d,r%d,r%d\n", rs1, rs2, rd);
  writeReg(rd, readReg(rs1) << readReg(rs2));
  dbg_printf("Result = 0x%x\n", readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

///!Instruction srl_reg behavior method.
void ac_behavior( srl_reg )
{
  dbg_printf("srl_reg r%d,r%d,r%d\n", rs1, rs2, rd);
  writeReg(rd, ((unsigned) readReg(rs1)) >> ((unsigned) readReg(rs2)));
  dbg_printf("Result = 0x%x\n", readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

///!Instruction sra_reg behavior method.
void ac_behavior( sra_reg )
{
  dbg_printf("sra_reg r%d,r%d,r%d\n", rs1, rs2, rd);
  writeReg(rd, ((int) readReg(rs1)) >> ((int) readReg(rs2)));
  dbg_printf("Result = 0x%x\n", readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

///!Instruction add_reg behavior method.
void ac_behavior( add_reg )
{
  dbg_printf("add_reg r%d,r%d,r%d\n", rs1, rs2, rd);
  writeReg(rd, readReg(rs1) + readReg(rs2));
  dbg_printf("Result = 0x%x\n", readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

///!Instruction addcc_reg behavior method.
void ac_behavior( addcc_reg )
{
  dbg_printf("addcc_reg r%d,r%d,r%d\n", rs1, rs2, rd);
  int dest = readReg(rs1) + readReg(rs2);

  PSR_icc_n = dest >> 31;
  PSR_icc_z = dest == 0;
  PSR_icc_v = (( readReg(rs1) &  readReg(rs2) & ~dest & 0x80000000) |
	       (~readReg(rs1) & ~readReg(rs2) &  dest & 0x80000000) );
  PSR_icc_c = ((readReg(rs1) & readReg(rs2) & 0x80000000) |
	       (~dest & (readReg(rs1) | readReg(rs2)) & 0x80000000) );

  writeReg(rd, dest);
  dbg_printf("Result = 0x%x\n", dest);
  update_pc(0,0,0,0,0, ac_pc, npc);
};

///!Instruction addx_reg behavior method.
void ac_behavior( addx_reg )
{
  dbg_printf("addx_reg r%d,r%d,r%d\n", rs1, rs2, rd);
  writeReg(rd, readReg(rs1) + readReg(rs2) + PSR_icc_c);
  dbg_printf("Result = 0x%x\n", readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

//!Instruction addxcc_reg behavior method.
void ac_behavior( addxcc_reg )
{
  dbg_printf("addxcc_reg r%d,r%d,r%d\n", rs1, rs2, rd);
  int dest = readReg(rs1) + readReg(rs2) + PSR_icc_c;

  PSR_icc_n = dest >> 31;
  PSR_icc_z = dest == 0;
  PSR_icc_v = (( readReg(rs1) &  readReg(rs2) & ~dest & 0x80000000) |
	       (~readReg(rs1) & ~readReg(rs2) &  dest & 0x80000000) );
  PSR_icc_c = ((readReg(rs1) & readReg(rs2) & 0x80000000) |
	       (~dest & (readReg(rs1) | readReg(rs2)) & 0x80000000) );

  writeReg(rd, dest);
  dbg_printf("Result = 0x%x\n", dest);
  update_pc(0,0,0,0,0, ac_pc, npc);
};

//!Instruction sub_reg behavior method.
void ac_behavior( sub_reg )
{
  dbg_printf("sub_reg r%d,r%d,r%d\n", rs1, rs2, rd);
  writeReg(rd, (readReg(rs1) - readReg(rs2)));
  dbg_printf("Result = 0x%x\n", readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

//!Instruction subcc_reg behavior method.
void ac_behavior( subcc_reg )
{
  dbg_printf("subcc_reg r%d,r%d,r%d\n", rs1, rs2, rd);
  int dest = readReg(rs1) - readReg(rs2);

  PSR_icc_n = dest >> 31;
  PSR_icc_z = dest == 0;
  PSR_icc_v = (( readReg(rs1) & ~readReg(rs2) & ~dest & 0x80000000) |
	       (~readReg(rs1) &  readReg(rs2) &  dest & 0x80000000) );
  PSR_icc_c = ((~readReg(rs1) & readReg(rs2) & 0x80000000) |
	       (dest & (~readReg(rs1) | readReg(rs2)) & 0x80000000) );

  writeReg(rd, dest);
  dbg_printf("Result = 0x%x\n", dest);
  update_pc(0,0,0,0,0, ac_pc, npc);
};

///!Instruction subx_reg behavior method.
void ac_behavior( subx_reg )
{
  dbg_printf("subx_reg r%d,r%d,r%d\n", rs1, rs2, rd);
  writeReg(rd, readReg(rs1) - readReg(rs2) - PSR_icc_c);
  dbg_printf("Result = 0x%x\n", readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

///!Instruction subxcc_reg behavior method.
void ac_behavior( subxcc_reg )
{
  dbg_printf("subxcc_reg r%d,r%d,r%d\n", rs1, rs2, rd);
  int dest = readReg(rs1) - readReg(rs2) - PSR_icc_c;

  PSR_icc_n = dest >> 31;
  PSR_icc_z = dest == 0;
  PSR_icc_v = (( readReg(rs1) & ~readReg(rs2) & ~dest & 0x80000000) |
	       (~readReg(rs1) &  readReg(rs2) &  dest & 0x80000000) );
  PSR_icc_c = ((~readReg(rs1) & readReg(rs2) & 0x80000000) |
	       (dest & (~readReg(rs1) | readReg(rs2)) & 0x80000000) );

  writeReg(rd, dest);
  dbg_printf("Result = 0x%x\n", dest);
  update_pc(0,0,0,0,0, ac_pc, npc);
};

//!Instruction and_reg behavior method.
void ac_behavior( and_reg )
{
  dbg_printf("and_reg r%d,r%d,r%d\n", rs1, rs2, rd);
  writeReg(rd, readReg(rs1) & readReg(rs2));
  dbg_printf("Result = 0x%x\n", readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

//!Instruction andcc_reg behavior method.
void ac_behavior( andcc_reg )
{
  dbg_printf("andcc_reg r%d,r%d,r%d\n", rs1, rs2, rd);
  int dest = readReg(rs1) & readReg(rs2);

  PSR_icc_n = dest >> 31;
  PSR_icc_z = dest == 0;
  PSR_icc_v = 0;
  PSR_icc_c = 0;

  writeReg(rd, dest);
  dbg_printf("Result = 0x%x\n", dest);
  update_pc(0,0,0,0,0, ac_pc, npc);
};

//!Instruction andn_reg behavior method.
void ac_behavior( andn_reg )
{
  dbg_printf("andn_reg r%d,r%d,r%d\n", rs1, rs2, rd);
  writeReg(rd, readReg(rs1) & ~readReg(rs2));
  dbg_printf("Result = 0x%x\n", readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

//!Instruction andncc_reg behavior method.
void ac_behavior( andncc_reg )
{
  dbg_printf("andncc_reg r%d,r%d,r%d\n", rs1, rs2, rd);
  int dest = readReg(rs1) & ~readReg(rs2);

  PSR_icc_n = dest >> 31;
  PSR_icc_z = dest == 0;
  PSR_icc_v = 0;
  PSR_icc_c = 0;

  writeReg(rd, dest);
  dbg_printf("Result = 0x%x\n", dest);
  update_pc(0,0,0,0,0, ac_pc, npc);
};

//!Instruction or_reg behavior method.
void ac_behavior( or_reg )
{
  dbg_printf("or_reg r%d,r%d,r%d\n", rs1, rs2, rd);
  writeReg(rd, readReg(rs1) | readReg(rs2));
  dbg_printf("Result = 0x%x\n", readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

//!Instruction orcc_reg behavior method.
void ac_behavior( orcc_reg )
{
  dbg_printf("orcc_reg r%d,r%d,r%d\n", rs1, rs2, rd);
  int dest = readReg(rs1) | readReg(rs2);

  PSR_icc_n = dest >> 31;
  PSR_icc_z = dest == 0;
  PSR_icc_v = 0;
  PSR_icc_c = 0;

  writeReg(rd, dest);
  dbg_printf("Result = 0x%x\n", dest);
  update_pc(0,0,0,0,0, ac_pc, npc);
};

//!Instruction orn_reg behavior method.
void ac_behavior( orn_reg )
{
  dbg_printf("orn_reg r%d,r%d,r%d\n", rs1, rs2, rd);
  writeReg(rd, readReg(rs1) | ~readReg(rs2));
  dbg_printf("Result = 0x%x\n", readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

//!Instruction orncc_reg behavior method.
void ac_behavior( orncc_reg )
{
  dbg_printf("orncc_reg r%d,r%d,r%d\n", rs1, rs2, rd);
  int dest = readReg(rs1) | ~readReg(rs2);

  PSR_icc_n = dest >> 31;
  PSR_icc_z = dest == 0;
  PSR_icc_v = 0;
  PSR_icc_c = 0;

  writeReg(rd, dest);
  dbg_printf("Result = 0x%x\n", dest);
  update_pc(0,0,0,0,0, ac_pc, npc);
};

//!Instruction xor_reg behavior method.
void ac_behavior( xor_reg )
{
  dbg_printf("xor_reg r%d,r%d,r%d\n", rs1, rs2, rd);
  writeReg(rd, readReg(rs1) ^ readReg(rs2));
  dbg_printf("Result = 0x%x\n", readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

///!Instruction xorcc_reg behavior method.
void ac_behavior( xorcc_reg )
{
  dbg_printf("xorcc_reg r%d,r%d,r%d\n", rs1, rs2, rd);
  int dest = readReg(rs1) ^ readReg(rs2);

  PSR_icc_n = dest >> 31;
  PSR_icc_z = dest == 0;
  PSR_icc_v = 0;
  PSR_icc_c = 0;

  writeReg(rd, dest);
  dbg_printf("Result = 0x%x\n", dest);
  update_pc(0,0,0,0,0, ac_pc, npc);
};

//!Instruction xnor_reg behavior method.
void ac_behavior( xnor_reg )
{
  dbg_printf("xnor_reg r%d,r%d,r%d\n", rs1, rs2, rd);
  writeReg(rd, ~(readReg(rs1) ^ readReg(rs2)));
  dbg_printf("Result = 0x%x\n", readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

///!Instruction xnorcc_reg behavior method.
void ac_behavior( xnorcc_reg )
{
  dbg_printf("xnorcc_reg r%d,r%d,r%d\n", rs1, rs2, rd);
  int dest = ~(readReg(rs1) ^ readReg(rs2));

  PSR_icc_n = dest >> 31;
  PSR_icc_z = dest == 0;
  PSR_icc_v = 0;
  PSR_icc_c = 0;

  writeReg(rd, dest);
  dbg_printf("Result = 0x%x\n", dest);
  update_pc(0,0,0,0,0, ac_pc, npc);
};

//!Instruction save_reg behavior method.
void ac_behavior( save_reg )
{
  dbg_printf("save_reg r%d,r%d,r%d\n", rs1, rs2, rd);
  int tmp = readReg(rs1) + readReg(rs2);

  //copy ins and locals to RB
  for (int i=16; i<32; i++) {
    RB.write((CWP + i) & 0xFF, REGS[i]);
  }

  //move out to in
  for (int i=0; i<8; i++) {
    REGS[i+24] = REGS[i+8];
  }

  //realy change reg window
  CWP = (CWP-0x10);
  if (CWP == WIM) trap_reg_window_overflow(DATA_PORT, RB, WIM);

  //copy local and out from buffer
  for (int i=8; i<24; i++) {
    REGS[i] = RB.read((CWP + i) & 0xFF);
  }

  writeReg(rd, tmp);
  dbg_printf(C_INVERSE "CWP: %d" C_RESET LF, CWP>>4);
  dbg_printf("Result = 0x%x\n", readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

//!Instruction restore_reg behavior method.
void ac_behavior( restore_reg )
{
  dbg_printf("restore_reg r%d,r%d,r%d\n", rs1, rs2, rd);
  int tmp = readReg(rs1) + readReg(rs2);

  //copy locals and out to buffer
  for (int i=8; i<24; i++) {
    RB.write((CWP + i) & 0xFF, REGS[i]);
  }

  //move in to out
  for (int i=0; i<8; i++) {
    REGS[i+8] = REGS[i+24];
  }

  //realy change reg window
  CWP = (CWP+0x10);
  if (CWP == WIM) trap_reg_window_underflow(DATA_PORT, RB, WIM);

  //copy in and local from buffer
  for (int i=16; i<32; i++) {
    REGS[i] = RB.read((CWP + i) & 0xFF);
  }

  writeReg(rd, tmp);
  dbg_printf(C_INVERSE "CWP: %d" C_RESET LF, CWP>>4);
  dbg_printf("Result = 0x%x\n", readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

///!Instruction umul_reg behavior method.
void ac_behavior( umul_reg )
{
  dbg_printf("umul_reg r%d,r%d,r%d\n", rs1, rs2, rd);
  unsigned long long tmp = (unsigned long long) (unsigned) readReg(rs1) * (unsigned long long) (unsigned) readReg(rs2);
  writeReg(rd, (unsigned int) tmp);
  Y.write( (unsigned int) (tmp >> 32));
  dbg_printf("Result = 0x%x\n", readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

///!Instruction smul_reg behavior method.
void ac_behavior( smul_reg )
{
  dbg_printf("smul_reg r%d,r%d,r%d\n", rs1, rs2, rd);
  long long tmp = (long long) readReg(rs1) * (long long) readReg(rs2);
  writeReg(rd, (int) tmp);
  Y.write( (int) (tmp >> 32));
  dbg_printf("Result = 0x%x\n", readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

///!Instruction umulcc_reg behavior method.
void ac_behavior( umulcc_reg )
{
  dbg_printf("umul_reg r%d,r%d,r%d\n", rs1, rs2, rd);
  unsigned long long tmp = (unsigned long long) (unsigned) readReg(rs1) * (unsigned long long) (unsigned) readReg(rs2);

  PSR_icc_n = (unsigned int) tmp >> 31;
  PSR_icc_z = (unsigned int) tmp == 0;
  PSR_icc_v = 0;
  PSR_icc_c = 0;

  writeReg(rd, (unsigned int) tmp);
  Y.write( (unsigned int) (tmp >> 32));
  dbg_printf("Result = 0x%x\n", readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

///!Instruction smulcc_reg behavior method.
void ac_behavior( smulcc_reg )
{
  dbg_printf("smulcc_reg r%d,r%d,r%d\n", rs1, rs2, rd);
  long long tmp = (long long) readReg(rs1) * (long long) readReg(rs2);

  PSR_icc_n = (unsigned int) tmp >> 31;
  PSR_icc_z = (unsigned int) tmp == 0;
  PSR_icc_v = 0;
  PSR_icc_c = 0;

  writeReg(rd, (int) tmp);
  Y.write( (int) (tmp >> 32));
  dbg_printf("Result = 0x%x\n", readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

///!Instruction udiv_reg behavior method.
void ac_behavior( udiv_reg )
{
  dbg_printf("udiv_reg r%d,r%d,r%d\n", rs1, rs2, rd);
  unsigned long long tmp;
  tmp = (unsigned long long) Y.read() << 32;
  tmp |= (unsigned) readReg(rs1);
  tmp /= (unsigned int) readReg(rs2);
  unsigned int result = tmp & 0xFFFFFFFF;
  bool temp_v = ((tmp >> 32) == 0) ? 0 : 1;
  if (temp_v) result = 0xFFFFFFFF;
  writeReg(rd, result);
  dbg_printf("Result = 0x%x\n", readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

///!Instruction udivcc_reg behavior method.
void ac_behavior( udivcc_reg )
{
  dbg_printf("udivcc_reg r%d,r%d,r%d\n", rs1, rs2, rd);
  unsigned long long tmp;
  tmp = (unsigned long long) Y.read() << 32;
  tmp |= (unsigned) readReg(rs1);
  tmp /= (unsigned int) readReg(rs2);
  unsigned int result = tmp & 0xFFFFFFFF;
  bool temp_v = ((tmp >> 32) == 0) ? 0 : 1;
  if (temp_v) result = 0xFFFFFFFF;

  PSR_icc_n = result >> 31;
  PSR_icc_z = result == 0;
  PSR_icc_v = temp_v;
  PSR_icc_c = 0;

  writeReg(rd, result);
  dbg_printf("Result = 0x%x\n", readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

///!Instruction sdiv_reg behavior method.
void ac_behavior( sdiv_reg )
{
  dbg_printf("sdiv_reg r%d,r%d,r%d\n", rs1, rs2, rd);
  long long tmp;
  tmp = (unsigned long long) Y.read() << 32;
  tmp |= (unsigned) readReg(rs1);
  tmp /= (signed) readReg(rs2);
  int result = tmp & 0xFFFFFFFF;
  bool temp_v = (((tmp >> 31) == 0) |
		 ((tmp >> 31) == -1LL)) ? 0 : 1;
  if (temp_v) {
    if (tmp > 0) result = 0x7FFFFFFF;
    else result = 0x80000000;
  }
  writeReg(rd, result);
  dbg_printf("Result = 0x%x\n", readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

//!Instruction sdivcc_reg behavior method.
void ac_behavior( sdivcc_reg )
{
  dbg_printf("sdivcc_reg r%d,r%d,r%d\n", rs1, rs2, rd);
  long long tmp;
  tmp = (unsigned long long) Y.read() << 32;
  tmp |= (unsigned) readReg(rs1);
  tmp /= (signed) readReg(rs2);
  int result = tmp & 0xFFFFFFFF;
  bool temp_v = (((tmp >> 31) == 0) |
		 ((tmp >> 31) == -1LL)) ? 0 : 1;
  if (temp_v) {
    if (tmp > 0) result = 0x7FFFFFFF;
    else result = 0x80000000;
  }

  PSR_icc_n = result >> 31;
  PSR_icc_z = result == 0;
  PSR_icc_v = temp_v;
  PSR_icc_c = 0;

  writeReg(rd, result);
  dbg_printf("Result = 0x%x\n", readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

///!Instruction jmpl_reg behavior method.
void ac_behavior( jmpl_reg )
{
  dbg_printf("jmpl_reg r%d,r%d,r%d\n", rs1, rs2, rd);
  writeReg(rd, ac_pc);
  //TODO: ugly: create a way to jump from a register without mapping
  update_pc(1,1,1,0, readReg(rs1) + readReg(rs2), ac_pc, npc);
};

///!Instruction wry_reg behavior method.
void ac_behavior( wry_reg )
{
  dbg_printf("wry_reg r%d,r%d,r%d\n", rs1, rs2, rd);
  Y.write( readReg(rs1) ^ readReg(rs2));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

///!Instruction ldsb_imm behavior method.
void ac_behavior( ldsb_imm )
{
  dbg_printf("ldsb_imm [r%d + %d], r%d\n", rs1, simm13, rd);
  writeReg(rd, (int)(char) DATA_PORT->read_byte(readReg(rs1) + simm13));
  dbg_printf("Result = 0x%x\n", readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

///!Instruction ldsh_imm behavior method.
void ac_behavior( ldsh_imm )
{
  dbg_printf("ldsh_imm [r%d + %d], r%d\n", rs1, simm13, rd);
  writeReg(rd, (int)(short) DATA_PORT->read_half(readReg(rs1) + simm13));
  dbg_printf("Result = 0x%x\n", readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

///!Instruction ldub_imm behavior method.
void ac_behavior( ldub_imm )
{
  dbg_printf("ldub_imm [r%d + %d], r%d\n", rs1, simm13, rd);
  writeReg(rd, DATA_PORT->read_byte(readReg(rs1) + simm13));
  dbg_printf("Result = 0x%x\n", readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

///!Instruction lduh_imm behavior method.
void ac_behavior( lduh_imm )
{
  dbg_printf("lduh_imm [r%d + %d], r%d\n", rs1, simm13, rd);
  writeReg(rd, DATA_PORT->read_half(readReg(rs1) + simm13));
  dbg_printf("Result = 0x%x\n", readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

///!Instruction ld_imm behavior method.
void ac_behavior( ld_imm )
{
  dbg_printf("ld_imm [r%d + %d], r%d\n", rs1, simm13, rd);
  writeReg(rd, DATA_PORT->read(readReg(rs1) + simm13));
  dbg_printf("Result = 0x%x\n", readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

///!Instruction ldd_imm behavior method.
void ac_behavior( ldd_imm )
{
  dbg_printf("ldd_imm [r%d + %d], r%d\n", rs1, simm13, rd);
  int tmp = DATA_PORT->read(readReg(rs1) + simm13 + 4);
  writeReg(rd,   DATA_PORT->read(readReg(rs1) + simm13));
  writeReg(rd+1, tmp);
  dbg_printf("Result = 0x%x\n", readReg(rd));
  dbg_printf("Result = 0x%x\n", readReg(rd+1));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

///!Instruction and_imm behavior method.
void ac_behavior( and_imm )
{
  dbg_printf("and_imm r%d,0x%x,r%d\n", rs1, simm13, rd);
  writeReg(rd, readReg(rs1) & simm13);
  dbg_printf("Result = 0x%x\n", readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

///!Instruction andcc_imm behavior method.
void ac_behavior( andcc_imm )
{
  dbg_printf("andcc_imm r%d,0x%x,r%d\n", rs1, simm13, rd);
  int dest = readReg(rs1) & simm13;

  PSR_icc_n = dest >> 31;
  PSR_icc_z = dest == 0;
  PSR_icc_v = 0;
  PSR_icc_c = 0;

  writeReg(rd, dest);
  dbg_printf("Result = 0x%x\n", dest);
  update_pc(0,0,0,0,0, ac_pc, npc);
};

///!Instruction andn_imm behavior method.
void ac_behavior( andn_imm )
{
  dbg_printf("andn_imm r%d,0x%x,r%d\n", rs1, simm13, rd);
  writeReg(rd, readReg(rs1) & ~simm13);
  dbg_printf("Result = 0x%x\n", readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

//!Instruction andncc_imm behavior method.
void ac_behavior( andncc_imm )
{
  dbg_printf("andncc_imm r%d,0x%x,r%d\n", rs1, simm13, rd);
  int dest = readReg(rs1) & ~simm13;

  PSR_icc_n = dest >> 31;
  PSR_icc_z = dest == 0;
  PSR_icc_v = 0;
  PSR_icc_c = 0;

  writeReg(rd, dest);
  dbg_printf("Result = 0x%x\n", dest);
  update_pc(0,0,0,0,0, ac_pc, npc);
};

//!Instruction or_imm behavior method.
void ac_behavior( or_imm )
{
  dbg_printf("or_imm r%d,0x%x,r%d\n", rs1, simm13, rd);
  writeReg(rd, readReg(rs1) | simm13);
  dbg_printf("Result = 0x%x\n", readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

//!Instruction orcc_imm behavior method.
void ac_behavior( orcc_imm )
{
  dbg_printf("orcc_imm r%d,0x%x,r%d\n", rs1, simm13, rd);
  int dest = readReg(rs1) | simm13;

  PSR_icc_n = dest >> 31;
  PSR_icc_z = dest == 0;
  PSR_icc_v = 0;
  PSR_icc_c = 0;

  writeReg(rd, dest);
  dbg_printf("Result = 0x%x\n", dest);
  update_pc(0,0,0,0,0, ac_pc, npc);
};

///!Instruction orn_imm behavior method.
void ac_behavior( orn_imm )
{
  dbg_printf("orn_imm r%d,0x%x,r%d\n", rs1, simm13, rd);
  writeReg(rd, readReg(rs1) | ~simm13);
  dbg_printf("Result = 0x%x\n", readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

///!Instruction orncc_imm behavior method.
void ac_behavior( orncc_imm )
{
  dbg_printf("orn_imm r%d,0x%x,r%d\n", rs1, simm13, rd);
  int dest = readReg(rs1) | ~simm13;

  PSR_icc_n = dest >> 31;
  PSR_icc_z = dest == 0;
  PSR_icc_v = 0;
  PSR_icc_c = 0;

  writeReg(rd, dest);
  dbg_printf("Result = 0x%x\n", dest);
  update_pc(0,0,0,0,0, ac_pc, npc);
};

///!Instruction xor_imm behavior method.
void ac_behavior( xor_imm )
{
  dbg_printf("xor_imm r%d,0x%x,r%d\n", rs1, simm13, rd);
  writeReg(rd, readReg(rs1) ^ simm13);
  dbg_printf("Result = 0x%x\n", readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

///!Instruction xorcc_imm behavior method.
void ac_behavior( xorcc_imm )
{
  dbg_printf("xorcc_imm r%d,0x%x,r%d\n", rs1, simm13, rd);
  int dest = readReg(rs1) ^ simm13;

  PSR_icc_n = dest >> 31;
  PSR_icc_z = dest == 0;
  PSR_icc_v = 0;
  PSR_icc_c = 0;

  writeReg(rd, dest);
  dbg_printf("Result = 0x%x\n", dest);
  update_pc(0,0,0,0,0, ac_pc, npc);
};

///!Instruction xnor_imm behavior method.
void ac_behavior( xnor_imm )
{
  dbg_printf("xnor_imm r%d,0x%x,r%d\n", rs1, simm13, rd);
  writeReg(rd, ~(readReg(rs1) ^ simm13));
  dbg_printf("Result = 0x%x\n", readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

///!Instruction xnorcc_imm behavior method.
void ac_behavior( xnorcc_imm )
{
  dbg_printf("xnorcc_imm r%d,0x%x,r%d\n", rs1, simm13, rd);
  int dest = ~(readReg(rs1) ^ simm13);

  PSR_icc_n = dest >> 31;
  PSR_icc_z = dest == 0;
  PSR_icc_v = 0;
  PSR_icc_c = 0;

  writeReg(rd, dest);
  dbg_printf("Result = 0x%x\n", dest);
  update_pc(0,0,0,0,0, ac_pc, npc);
};

///!Instruction umul_imm behavior method.
void ac_behavior( umul_imm )
{
  dbg_printf("umul_imm r%d,%d,r%d\n", rs1, simm13, rd);
  unsigned long long tmp = (unsigned long long) (unsigned) readReg(rs1) * (unsigned long long) (unsigned) simm13;
  writeReg(rd, (unsigned int) tmp);
  Y.write( (unsigned int) (tmp >> 32));
  dbg_printf("Result = 0x%x\n", readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

///!Instruction smul_imm behavior method.
void ac_behavior( smul_imm )
{
  dbg_printf("smul_imm r%d,%d,r%d\n", rs1, simm13, rd);
  long long tmp = (long long) readReg(rs1) * (long long) simm13;
  writeReg(rd, (int) tmp);
  Y.write( (int) (tmp >> 32));
  dbg_printf("Result = 0x%x\n", readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

///!Instruction umulcc_imm behavior method.
void ac_behavior( umulcc_imm )
{
  dbg_printf("umulcc_imm r%d,%d,r%d\n", rs1, simm13, rd);
  unsigned long long tmp = (unsigned long long) (unsigned) readReg(rs1) * (unsigned long long) (unsigned) simm13;

  PSR_icc_n = (unsigned int) tmp >> 31;
  PSR_icc_z = (unsigned int) tmp == 0;
  PSR_icc_v = 0;
  PSR_icc_c = 0;

  writeReg(rd, (unsigned int) tmp);
  Y.write( (unsigned int) (tmp >> 32));
  dbg_printf("Result = 0x%x\n", readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

///!Instruction smulcc_imm behavior method.
void ac_behavior( smulcc_imm )
{
  dbg_printf("smulcc_imm r%d,%d,r%d\n", rs1, simm13, rd);
  long long tmp = (long long) readReg(rs1) * (long long) simm13;

  PSR_icc_n = (unsigned int) tmp >> 31;
  PSR_icc_z = (unsigned int) tmp == 0;
  PSR_icc_v = 0;
  PSR_icc_c = 0;

  writeReg(rd, (int) tmp);
  Y.write( (int) (tmp >> 32));
  dbg_printf("Result = 0x%x\n", readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

///!Instruction udiv_imm behavior method.
void ac_behavior( udiv_imm )
{
  dbg_printf("udiv_imm r%d,%d,r%d\n", rs1, simm13, rd);
  unsigned long long tmp;
  tmp = (unsigned long long) Y.read() << 32;
  tmp |= (unsigned) readReg(rs1);
  tmp /= (unsigned int) simm13;
  unsigned int result = tmp & 0xFFFFFFFF;
  bool temp_v = ((tmp >> 32) == 0) ? 0 : 1;
  if (temp_v) result = 0xFFFFFFFF;
  writeReg(rd, result);
  dbg_printf("Result = 0x%x\n", readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

///!Instruction udivcc_imm behavior method.
void ac_behavior( udivcc_imm )
{
  dbg_printf("udivcc_imm r%d,%d,r%d\n", rs1, simm13, rd);
  unsigned long long tmp;
  tmp = (unsigned long long) Y.read() << 32;
  tmp |= (unsigned) readReg(rs1);
  tmp /= (unsigned int) simm13;
  unsigned int result = tmp & 0xFFFFFFFF;
  bool temp_v = ((tmp >> 32) == 0) ? 0 : 1;
  if (temp_v) result = 0xFFFFFFFF;

  PSR_icc_n = result >> 31;
  PSR_icc_z = result == 0;
  PSR_icc_v = temp_v;
  PSR_icc_c = 0;

  writeReg(rd, result);
  dbg_printf("Result = 0x%x\n", readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

///!Instruction sdiv_imm behavior method.
void ac_behavior( sdiv_imm )
{
  dbg_printf("sdiv_imm r%d,%d,r%d\n", rs1, simm13, rd);
  long long tmp;
  tmp = (unsigned long long) Y.read() << 32;
  tmp |= (unsigned) readReg(rs1);
  tmp /= (signed) simm13;
  int result = tmp & 0xFFFFFFFF;
  bool temp_v = (((tmp >> 31) == 0) |
		 ((tmp >> 31) == -1LL)) ? 0 : 1;
  if (temp_v) {
    if (tmp > 0) result = 0x7FFFFFFF;
    else result = 0x80000000;
  }
  writeReg(rd, result);
  dbg_printf("Result = 0x%x\n", readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

///!Instruction sdivcc_imm behavior method.
void ac_behavior( sdivcc_imm )
{
  dbg_printf("sdivcc_imm r%d,%d,r%d\n", rs1, simm13, rd);
  long long tmp;
  tmp = (unsigned long long) Y.read() << 32;
  tmp |= (unsigned) readReg(rs1);
  tmp /= simm13;
  int result = tmp & 0xFFFFFFFF;
  bool temp_v = (((tmp >> 31) == 0) |
		 ((tmp >> 31) == -1LL)) ? 0 : 1;
  if (temp_v) {
    if (tmp > 0) result = 0x7FFFFFFF;
    else result = 0x80000000;
  }

  PSR_icc_n = result >> 31;
  PSR_icc_z = result == 0;
  PSR_icc_v = temp_v;
  PSR_icc_c = 0;

  writeReg(rd, result);
  dbg_printf("Result = 0x%x\n", readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

///!Instruction stb_imm behavior method.
void ac_behavior( stb_imm )
{
  dbg_printf("stb_imm r%d, [r%d + %d]\n", rd, rs1, simm13);
  DATA_PORT->write_byte(readReg(rs1) + simm13, (char) readReg(rd));
  dbg_printf("Result = 0x%x\n", (char) readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

///!Instruction sth_imm behavior method.
void ac_behavior( sth_imm )
{
  dbg_printf("sth_imm r%d, [r%d + %d]\n", rd, rs1, simm13);
  DATA_PORT->write_half(readReg(rs1) + simm13, (short) readReg(rd));
  dbg_printf("Result = 0x%x\n", (short) readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

///!Instruction st_imm behavior method.
void ac_behavior( st_imm )
{
  dbg_printf("st_imm r%d, [r%d + %d]\n", rd, rs1, simm13);
  DATA_PORT->write(readReg(rs1) + simm13, readReg(rd));
  dbg_printf("Result = 0x%x\n", readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

///!Instruction std_imm behavior method.
void ac_behavior( std_imm )
{
  dbg_printf("std_imm r%d, [r%d + %d]\n", rd, rs1, simm13);
  DATA_PORT->write(readReg(rs1) + simm13,     readReg(rd  ));
  DATA_PORT->write(readReg(rs1) + simm13 + 4, readReg(rd+1));
  dbg_printf("Result = 0x%x\n", readReg(rd));
  dbg_printf("Result = 0x%x\n", readReg(rd+1));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

///!Instruction ldstub_imm behavior method.
void ac_behavior( ldstub_imm )
{
  dbg_printf("atomic ldstub_imm r%d, [r%d + %d]\n", rd, rs1, simm13);
  writeReg(rd, DATA_PORT->read_byte(readReg(rs1) + simm13));
  DATA_PORT->write_byte(readReg(rs1) + simm13, 0xFF);
  update_pc(0,0,0,0,0, ac_pc, npc);
};

///!Instruction swap_imm behavior method.
void ac_behavior( swap_imm )
{
  dbg_printf("swap_imm r%d, [r%d + %d]\n", rd, rs1, simm13);
  int swap_temp = DATA_PORT->read(readReg(rs1) + simm13);
  DATA_PORT->write(readReg(rs1) + simm13, readReg(rd));
  writeReg(rd, swap_temp);
  update_pc(0,0,0,0,0, ac_pc, npc);
};

///!Instruction sll_imm behavior method.
void ac_behavior( sll_imm )
{
  dbg_printf("sll_imm r%d,%d,r%d\n", rs1, simm13, rd);
  writeReg(rd, readReg(rs1) << simm13);
  dbg_printf("Result = 0x%x\n", readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

///!Instruction srl_imm behavior method.
void ac_behavior( srl_imm )
{
  dbg_printf("srl_imm r%d,%d,r%d\n", rs1, simm13, rd);
  writeReg(rd, ((unsigned) readReg(rs1)) >> ((unsigned) simm13));
  dbg_printf("Result = 0x%x\n", readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

///!Instruction sra_imm behavior method.
void ac_behavior( sra_imm )
{
  dbg_printf("sra_imm r%d,%d,r%d\n", rs1, simm13, rd);
  writeReg(rd, ((int) readReg(rs1)) >> simm13 );
  dbg_printf("Result = 0x%x\n", readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

//!Instruction add_imm behavior method.
void ac_behavior( add_imm )
{
  dbg_printf("add_imm r%d,0x%x,r%d\n", rs1, simm13, rd);
  writeReg(rd, readReg(rs1) + simm13);
  dbg_printf("Result = 0x%x\n", readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

///!Instruction addcc_imm behavior method.
void ac_behavior( addcc_imm )
{
  dbg_printf("addcc_imm r%d,0x%x,r%d\n", rs1, simm13, rd);
  int dest = readReg(rs1) + simm13;

  PSR_icc_n = dest >> 31;
  PSR_icc_z = dest == 0;
  PSR_icc_v = (( readReg(rs1) &  simm13 & ~dest & 0x80000000) |
	       (~readReg(rs1) & ~simm13 &  dest & 0x80000000) );
  PSR_icc_c = ((readReg(rs1) & simm13 & 0x80000000) |
	       (~dest & (readReg(rs1) | simm13) & 0x80000000) );

  writeReg(rd, dest);
  dbg_printf("Result = 0x%x\n", dest);
  update_pc(0,0,0,0,0, ac_pc, npc);
};

///!Instruction addx_imm behavior method.
void ac_behavior( addx_imm )
{
  dbg_printf("addx_imm r%d,0x%x,r%d\n", rs1, simm13, rd);
  writeReg(rd, readReg(rs1) + simm13 + PSR_icc_c);
  dbg_printf("Result = 0x%x\n", readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

///!Instruction addxcc_imm behavior method.
void ac_behavior( addxcc_imm )
{
  dbg_printf("addxcc_imm r%d,0x%x,r%d\n", rs1, simm13, rd);
  int dest = readReg(rs1) + simm13 + PSR_icc_c;

  PSR_icc_n = dest >> 31;
  PSR_icc_z = dest == 0;
  PSR_icc_v = (( readReg(rs1) &  simm13 & ~dest & 0x80000000) |
	       (~readReg(rs1) & ~simm13 &  dest & 0x80000000) );
  PSR_icc_c = ((readReg(rs1) & simm13 & 0x80000000) |
	       (~dest & (readReg(rs1) | simm13) & 0x80000000) );

  writeReg(rd, dest);
  dbg_printf("Result = 0x%x\n", dest);
  update_pc(0,0,0,0,0, ac_pc, npc);
};

///!Instruction sub_imm behavior method.
void ac_behavior( sub_imm )
{
  dbg_printf("sub_imm r%d,0x%x,r%d\n", rs1, simm13, rd);
  writeReg(rd, readReg(rs1) - simm13);
  dbg_printf("Result = 0x%x\n", readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

//!Instruction subcc_imm behavior method.
void ac_behavior( subcc_imm )
{
  dbg_printf("subcc_imm r%d,0x%x,r%d\n", rs1, simm13, rd);
  int dest = readReg(rs1) - simm13;

  PSR_icc_n = dest >> 31;
  PSR_icc_z = dest == 0;
  PSR_icc_v = (( readReg(rs1) & ~simm13 & ~dest & 0x80000000) |
	       (~readReg(rs1) &  simm13 &  dest & 0x80000000) );
  PSR_icc_c = ((~readReg(rs1) & simm13 & 0x80000000) |
	       (dest & (~readReg(rs1) | simm13) & 0x80000000) );

  writeReg(rd, dest);
  dbg_printf("Result = 0x%x\n", dest);
  update_pc(0,0,0,0,0, ac_pc, npc);
};

///!Instruction subx_imm behavior method.
void ac_behavior( subx_imm )
{
  dbg_printf("subx_imm r%d,0x%x,r%d\n", rs1, simm13, rd);
  writeReg(rd, readReg(rs1) - simm13 - PSR_icc_c);
  dbg_printf("Result = 0x%x\n", readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

///!Instruction subxcc_imm behavior method.
void ac_behavior( subxcc_imm )
{
  dbg_printf("subxcc_imm r%d,0x%x,r%d\n", rs1, simm13, rd);
  int dest = readReg(rs1) - simm13 - PSR_icc_c;

  PSR_icc_n = dest >> 31;
  PSR_icc_z = dest == 0;
  PSR_icc_v = (( readReg(rs1) & ~simm13 & ~dest & 0x80000000) |
	       (~readReg(rs1) &  simm13 &  dest & 0x80000000) );
  PSR_icc_c = ((~readReg(rs1) & simm13 & 0x80000000) |
	       (dest & (~readReg(rs1) | simm13) & 0x80000000) );

  writeReg(rd, dest);
  dbg_printf("Result = 0x%x\n", dest);
  update_pc(0,0,0,0,0, ac_pc, npc);
};

///!Instruction jmpl_imm behavior method.
void ac_behavior( jmpl_imm )
{
  dbg_printf("jmpl_imm r%d,%d,r%d\n", rs1, simm13, rd);
  writeReg(rd, ac_pc);
  update_pc(1,1,1,0, readReg(rs1) + simm13, ac_pc, npc);
};

//!Instruction save_imm behavior method.
void ac_behavior( save_imm )
{
  dbg_printf("save_imm r%d, %d, r%d\n", rs1, simm13, rd);
  int tmp = readReg(rs1) + simm13;

  //copy ins and locals to RB
  for (int i=16; i<32; i++) {
    RB.write((CWP + i) & 0xFF, REGS[i]);
  }

  //move out to in
  for (int i=0; i<8; i++) {
    REGS[i+24] = REGS[i+8];
  }

  //realy change reg window
  CWP = (CWP-0x10);
  if (CWP == WIM) trap_reg_window_overflow(DATA_PORT, RB, WIM);

  //copy local and out from buffer
  for (int i=8; i<24; i++) {
    REGS[i] = RB.read((CWP + i) & 0xFF);
  }

  writeReg(rd, tmp);
  dbg_printf(C_INVERSE "CWP: %d" C_RESET LF, CWP>>4);
  dbg_printf("Result = 0x%x\n", readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

//!Instruction restore_imm behavior method.
void ac_behavior( restore_imm )
{
  dbg_printf("restore_imm r%d, %d, r%d\n", rs1, simm13, rd);
  int tmp = readReg(rs1) + simm13;

  //copy locals and out to buffer
  for (int i=8; i<24; i++) {
    RB.write((CWP + i) & 0xFF, REGS[i]);
  }

  //move in to out
  for (int i=0; i<8; i++) {
    REGS[i+8] = REGS[i+24];
  }

  //realy change reg window
  CWP = (CWP+0x10);
  if (CWP == WIM) trap_reg_window_underflow(DATA_PORT, RB, WIM);

  //copy in and local from buffer
  for (int i=16; i<32; i++) {
    REGS[i] = RB.read((CWP + i) & 0xFF);
  }

  writeReg(rd, tmp);
  dbg_printf(C_INVERSE "CWP: %d" C_RESET LF, CWP>>4);
  dbg_printf("Result = 0x%x\n", readReg(rd));
  update_pc(0,0,0,0,0, ac_pc, npc);
};

//!Instruction rdy behavior method.
void ac_behavior( rdy )
{
  dbg_printf("rdy r%d\n", rd);
  writeReg(rd, Y.read());
  update_pc(0,0,0,0,0, ac_pc, npc);
};

//!Instruction wry_imm behavior method.
void ac_behavior( wry_imm )
{
  dbg_printf("wry_imm\n");
  Y.write( readReg(rs1) ^ simm13);
  update_pc(0,0,0,0,0, ac_pc, npc);
};

//!Instruction mulscc_reg behavior method.
void ac_behavior( mulscc_reg )
{
  dbg_printf("mulscc_reg r%d, r%d, r%d\n", rs1, rs2, rd);
  int rs1_0 = readReg(rs1) & 1;
  int op1 = ((PSR_icc_n ^ PSR_icc_v) << 31) | (readReg(rs1) >> 1);
  int op2 = ((Y.read() & 1) == 0) ? 0 : readReg(rs2);
  int dest = op1+op2;

  PSR_icc_n = dest >> 31;
  PSR_icc_z = dest == 0;
  PSR_icc_v = (( op1 &  op2 & ~dest & 0x80000000) |
	       (~op1 & ~op2 &  dest & 0x80000000) );
  PSR_icc_c = (( op1 &  op2 & 0x80000000) |
	       (~dest & (op1 | op2) & 0x80000000) );

  writeReg(rd, dest);
  Y.write( (rs1_0 << 31) | (Y.read() >> 1));
  update_pc(0,0,0,0,0, ac_pc, npc);
}

//!Instruction mulscc_imm behavior method.
void ac_behavior( mulscc_imm )
{
  dbg_printf("mulscc_imm r%d, %d, r%d\n", rs1, simm13, rd);
  int rs1_0 = readReg(rs1) & 1;
  int op1 = ((PSR_icc_n ^ PSR_icc_v) << 31) | (readReg(rs1) >> 1);
  int op2 = ((Y.read() & 1) == 0) ? 0 : simm13;
  int dest = op1+op2;

  PSR_icc_n = dest >> 31;
  PSR_icc_z = dest == 0;
  PSR_icc_v = (( op1 &  op2 & ~dest & 0x80000000) |
	       (~op1 & ~op2 &  dest & 0x80000000) );
  PSR_icc_c = (( op1 &  op2 & 0x80000000) |
	       (~dest & (op1 | op2) & 0x80000000) );

  writeReg(rd, dest);
  Y.write( (rs1_0 << 31) | (Y.read() >> 1));
  update_pc(0,0,0,0,0, ac_pc, npc);
}

//!Instruction trap behavior method.
//TODO: unimplemented
void ac_behavior( trap_reg )
{
  dbg_printf("trap\n");
  stop();
}

//!Instruction trap behavior method.
//TODO: unimplemented
void ac_behavior( trap_imm )
{
  dbg_printf("trap\n");
  stop();
}

//!Instruction unimplemented behavior method.
void ac_behavior( unimplemented )
{
  dbg_printf("unimplemented\n");
  printf("sparc-isa.cpp: program flow reach instruction 'unimplemented' at ac_pc=%#x\n", (int)ac_pc);
  stop(EXIT_FAILURE);
  update_pc(0,0,0,0,0, ac_pc, npc);
}
