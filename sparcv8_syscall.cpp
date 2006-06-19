/**
 * @file      sparcv8_syscall.cpp
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
 * @version   version?
 * @date      Mon, 19 Jun 2006 15:33:29 -0300
 * 
 * @brief     The ArchC SPARC-V8 functional model.
 * 
 * @attention Copyright (C) 2002-2006 --- The ArchC Team
 *
 */

#include "sparcv8_syscall.H"
#include "ac_resources.H"

#define writeReg(addr, val) REGS[addr] = (addr)? ac_word(val) : 0
#define readReg(addr) REGS[addr]

void sparcv8_syscall::get_buffer(int argn, unsigned char* buf, unsigned int size)
{
  unsigned int addr = readReg(8+argn);

  for (unsigned int i = 0; i<size; i++, addr++) {
    buf[i] = DM.read_byte(addr);
  }
}

void sparcv8_syscall::set_buffer(int argn, unsigned char* buf, unsigned int size)
{
  unsigned int addr = readReg(8+argn);

  for (unsigned int i = 0; i<size; i++, addr++) {
    DM.write_byte(addr, buf[i]);
  }
}

void sparcv8_syscall::set_buffer_noinvert(int argn, unsigned char* buf, unsigned int size)
{
  unsigned int addr = readReg(8+argn);

  for (unsigned int i = 0; i<size; i+=4, addr+=4) {
    DM.write(addr, *(unsigned int *) &buf[i]);
  }
}

int sparcv8_syscall::get_int(int argn)
{
  return readReg(8+argn);
}

void sparcv8_syscall::set_int(int argn, int val)
{
  writeReg(8+argn, val);
}

void sparcv8_syscall::return_from_syscall()
{
  //similar to retl (jmpl %o7+8, %g0), but annul next
  extern int npc;
  npc = readReg(15) + 8;
  ac_pc = npc;
  npc += 4;
}

void sparcv8_syscall::set_prog_args(int argc, char **argv)
{
  extern const unsigned AC_RAM_END;
  int i, j, base;

  unsigned int ac_argv[30];
  char ac_argstr[512];

  base = AC_RAM_END - 512;
  for (i=0, j=0; i<argc; i++) {
    int len = strlen(argv[i]) + 1;
    ac_argv[i] = base + j;
    memcpy(&ac_argstr[j], argv[i], len);
    j += len;
  }

  //Ajust %sp and write argument string
  writeReg(14, AC_RAM_END-512);
  set_buffer(6, (unsigned char*) ac_argstr, 512);

  //Ajust %sp and write string pointers
  writeReg(14, AC_RAM_END-512-120);
  set_buffer_noinvert(6, (unsigned char*) ac_argv, 120);

  //Set %o0 to the argument count
  writeReg(8, argc);

  //Set %o1 to the string pointers
  writeReg(9, AC_RAM_END-512-120);
}
