/**
 * @file      sparc_gdb_funcs.cpp
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

#include "sparc.H"

using namespace sparc_parms;

int sparc::nRegs(void) {
  return 72;
}

ac_word sparc::reg_read( int reg ) {
  /* General Purpose: G, O, L, I */
  if ( ( reg >= 0 ) && ( reg < 32 ) ) {
    return REGS[reg];
  }
  
  /* Y, PSR, WIM, PC, NPC */
  else if ( reg == 64 ) return Y;
  else if ( reg == 65 ) return PSR;
  else if ( reg == 66 ) return WIM;
  else if ( reg == 68 ) return ac_pc;
  else if ( reg == 69 ) return npc;
  
  /* Floating point, TBR, FPSR, CPSR */
  return 0;
}


void sparc::reg_write( int reg, ac_word value ) {
  /* General Purpose: G, O, L & I regs */
  if ( ( reg >= 0 ) && ( reg < 32 ) ) {
    REGS[reg] = value;
  }
  
  /* Y, PSR, WIM, PC, NPC */
  else if ( reg == 64 ) Y   = value;
  else if ( reg == 65 ) PSR = value;
  else if ( reg == 66 ) WIM = value;
  else if ( reg == 68 ) ac_pc = value;
  else if ( reg == 69 ) npc = value;
}


unsigned char sparc::mem_read( unsigned int address ) {
  return DATA_PORT->read_byte( address );
}


void sparc::mem_write( unsigned int address, unsigned char byte ) {
  DATA_PORT->write_byte( address, byte );
}
