## 2.4.0

* Revision numbers following the ArchC release
* Instructions with cycles annotations
* Two new .ac files to use with MPSoCBench (block and nonblock)
* sparc_isa.cpp using the reserved work DATA_PORT to data request. See the [commit message](https://github.com/ArchC/sparc/commit/b52eac067ce76e6e4de016697f2158c12d3fcdb1).
* Interrupt handler support. It is inactive in standalone simulator.
* New PowerSC tables 

[Full changelog](https://github.com/ArchC/sparc/compare/v2.3.0...v2.4.0)

## 2.3.0
* Revision numbers following the ArchC release
+ PowerSC support with power tables
+ Added id register for core identification

[Full changelog](https://github.com/ArchC/sparc/compare/v1.0.0...v2.3.0)

## 1.0.0
* ArchC 2.2 compliant

[Full changelog](https://github.com/ArchC/sparc/compare/v0.7.8...v1.0.0)

## 0.7.8
+ Added binary utilities support files
* ArchC 2.0 compliant

[Full changelog](https://github.com/ArchC/sparc/compare/v0.7.7...v0.7.8)

## 0.7.7-archc2.0beta3
+ Fixed an annoying bug which caused a segfault on the fft program (MiBench)

[Full changelog](https://github.com/ArchC/sparc/compare/v0.7.6...v0.7.7)

## 0.7.6-archc2.0beta3

+ Added license headers

[Full changelog](https://github.com/ArchC/sparc/compare/v0.7.5-1...v0.7.6)

## 0.7.5-archc2.0beta1

* Model compliant with ArchC 2.0beta1

[Full changelog](https://github.com/ArchC/sparc/compare/v0.7.5...v0.7.5-1)

## 0.7.5

* Fixed 'unimplemented' instruction format and assembly syntax
* Fixed bug in mulscc_reg and mulscc_imm instructions (condition codes)
* npc initialization depends on ac_pc, so program can start at arbitrary address

## 0.7.4

+ Included assembly language syntax information

## 0.7.3

* Fix bug in instruction MULSCC (used only for V7 multiplication)
+ New REGS register bank holds the current register window from RB
* Use ArchC support for debug messages: ac_debug_model.H
* Use operator[] syntax to read register banks, which is faster

## 0.7.2

+ Included optimization instruction methods for compiled simulation
* Separate nop instruction from sethi to optimize simulation

## 0.7.1

+ Included file for GDB integration

## 0.7.0

* Model passed selected Mediabench and Mibench applications
