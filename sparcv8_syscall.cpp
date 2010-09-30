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
 * @version   1.0
 * @date      Mon, 19 Jun 2006 15:50:50 -0300
 * 
 * @brief     The ArchC SPARC-V8 functional model.
 * 
 * @attention Copyright (C) 2002-2006 --- The ArchC Team
 * 
 * This program is free software; you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License as published by 
 * the Free Software Foundation; either version 2 of the License, or 
 * (at your option) any later version. 
 * 
 * This program is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
 * GNU General Public License for more details. 
 * 
 * You should have received a copy of the GNU General Public License 
 * along with this program; if not, write to the Free Software 
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "sparcv8_syscall.H"

#define writeReg(addr, val) REGS[addr] = (addr)? ac_word(val) : 0
#define readReg(addr) REGS[addr]

// Namespace for sparcv8 types.
using namespace sparcv8_parms;

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
  npc = readReg(15) + 8;
  ac_pc = npc;
  npc += 4;
}

void sparcv8_syscall::set_prog_args(int argc, char **argv)
{
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

// Method for compatibility with new models
int *sparcv8_syscall::get_syscall_table(){
	static int syscall_table[] = { 0, 0, 0, 0};
	return syscall_table;
}



