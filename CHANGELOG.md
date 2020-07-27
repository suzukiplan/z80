# Change log

## Version 1.2.0 (WIP)

- Enhancement for make more strict getting clock cycle consuming.

## Version 1.1.0 (Jul 27, 2020 JST)

- enhancement: implement undocumented instructions:
  - `INC IXH/IXL/IYH/IYL`
  - `DEC IXH/IXL/IYH/IYL`
  - `LD IXH/IXL/IYH/IYL, nn`
  - `LD A/B/C/D/E, IXH/IXL/IYH/IYL`
  - `LD IXH/IXL/IYH/IYL, A/B/C/D/E/IXH/IXL/IYH/IYL`
  - `ADD/ADC/SUB/SBC/AND/XOR/OR/CP A, IXH/IXL/IYH/IYL`
  - `RLC (IX+nn) with LD B/C/D/E/H/L/F/A, (IX+nn)`
  - `RRC/RL/RR/SLA/SRA/SLL/SRL (IX+nn) with LD B, (IX+nn)`
  - `RES/SET 0/1/2/3/4/5/6/7, (IX+nn) with LD B, (IX+nn)`
  - `SLL B/C/D/E/H/L/(HL)/A`
  - `IN F,(C)`
- bugfix: incorrect H status flag: OR/XOR (must reset but set)
- bugfix: logging PC of RST is incorrect
- refactor: make common flag setting function for rotate result
- change-feature: remove carry log from RRC/RLC/SRA/SLA/SRL operands because not effort

## Version 1.0.1 (Jul 25, 2020 JST)

- bugfix: incorrect return address when calling RST instruction directly from code

## Version 1.0.0 (Jul 25, 2020 JST)

- split clock synchronization timing into fine timings such as fetch and store
  - in the process, some operands with incorrect execution clock numbers are being corrected.
- Fixed implementation error of repeat operands (LDIR, LDDR, CPIR, CPDR, OUTIR, OUTDR, INIR, INDR)
  - actual: Loop until B flag is 0 in instruction
  - expected: PC cannot proceed unless B flag is 0
- refactor: remove verbose procedure
- refactor: make wrapper functions for memory read/write & port I/O
- bugfix: often crush if not set debug message
- bugfix: HALT is not released by IRQ/NMI
- performance up
- change register layout (disruptive change)
- bugfix: correct the invalid clock cycle: LD SP, nn (expected: 10Hz, actual: 20Hz)
- bugfix: missing consume clock 4Hz in CPDR
- bugfix: memory issue will occur when execute the relative jump operands if enabled debug mode
- bugfix: correct the invalid clock cycle: jump relative + if
- bugfix: correct the invalid clock cycle: JP (HL), JP (IX), JP (IY)
- bugfix: correct the invalid clock cycle: RST
- bugfix: correct the invalid clock cycle: OUT (N), A
- interrupt processing is not executed by the instruction immediately after executing EI
- do not clear IRQ/NMI request when masked
- bugfix: the status register set after executing LDI/LDD are incorrect.
- bugfix: P/V flag after LDI/LDD is incorrect
- bugfix: correct undocumented conditions: OUTI, OUTIR, OUTD, OUTDR
- bugfix: INC/DEC operands are not change carry flag
- add cancelIRQ function
- bugfix: correct status flag conditions after repeat I/O: INI, INIR, IND, INDR, OUTI, OUTIR, OUTD, OUTDR
- bugfix: correct carry flag conditions: CPI, CPIR, CPD, CPDR
- bugfix: correct negative flag conditions: the all of substract/compare operands
- bugfix: correct P/V flag conditions: CPI, CPIR, CPD, CPDR
- bugfix: DAA
- bugfix: correct flag conditions of 16bi calculation

## Version 0.9 (Jul 16, 2020 JST)

- bugfix: invalid clock cycle in repeart operands
- bugfix: invalid register will specified in `OUT (C), R` & `IN R, (C)`

## Version 0.8 (Jul 16, 2020 JST)

- Fixed a bug that IRQ/NMI request flag does not clear when masked

## Version 0.7 (Jul 16, 2020 JST)

- Fixed a bug that DJNZ command does not work properly:
  - 0.6 or less: An instruction that decrements the B register and makes a relative jump if it `is 0`.
  - 0.7 or later: An instruction that decrements the B register and makes a relative jump if it `is not 0`.

## Version 0.6 (Jul 14, 2020 JST)

- implemented & bugfix of interrupt features

## Version 0.5 (Jul 12, 2020 JST)

- implemented compatible(?) instructions:
  - `OUT (C), r`

## Version 0.4 (Jul 12, 2020 JST)

- added an explicit execution break function (requestBreak)
- corrected disassemble log of following:
  - `AND R, (IX+d)`
  - `AND R, (IY+d)`
  - `OR R, (RP)`
  - `OR R, (IX+d)`
  - `OR R, (IY+d)`
  - `XOR R, (RP)`
  - `XOR R, (IX+d)`
  - `XOR R, (IY+d)`

## Version 0.3 (Sept 8, 2019 JST)

- support multiple break operands (Destructive change)
  - remove function: `setBreakOperand`
  - add functions: `addBreakOperand` , `removeBreakOperand` , `removeAllBreakOperands`
- bugfix: do not increment the program counter while halting

## Version 0.2 (Sept 8, 2019 JST)

- support multiple break points (Destructive change)
  - remove function: `setBreakPoint`
  - add functions: `addBreakPoint` , `removeBreakPoint` , `removeAllBreakPoints`

## Version 0.1 (Sept 7, 2019 JST)

- implemented the all of documented instructions
