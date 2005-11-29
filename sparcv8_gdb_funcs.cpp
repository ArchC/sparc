#include "sparcv8.H"

extern unsigned char CWP;     
extern unsigned char WIM;     
extern int npc;               


int sparcv8::nRegs(void) {
  return 72;
}


ac_word sparcv8::reg_read( int reg ) {
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


void sparcv8::reg_write( int reg, ac_word value ) {
  /* General Purpose: G, O, L & I regs */
  if ( ( reg >= 0 ) && ( reg < 32 ) ) {
    REGS[reg] = value;
  }
  
  /* Y, PSR, WIM, PC, NPC */
  else if ( reg == 64 ) Y   = value;
  else if ( reg == 65 ) PSR = value;
  else if ( reg == 66 ) WIM = value;
  else if ( reg == 68 ) ac_resources::ac_pc = value;
  else if ( reg == 69 ) npc = value;
}


unsigned char sparcv8::mem_read( unsigned int address ) {
  return IM->read_byte( address );
}


void sparcv8::mem_write( unsigned int address, unsigned char byte ) {
  IM->write_byte( address, byte );
}
