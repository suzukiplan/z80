# Change log

## Version 1.6.0 (inprogress)

- Use std::function at addBreakPoint, addBreakOperand, ...(TODO)
- Change specification of removeBreakPoint:
  - before: specify function pointer for remove
  - after: specify address number for remove
- Change specification of removeBreakOperand:
  - before: specify function pointer for remove
  - after: specify operand number for remove

## Version 1.5.0 (Spt 11, 2022 JST)

- Implemented several undocumented instructions of ED instruction set.
- bugfix: Corrected opcode `ED70` behavior
  - `IN F, (C)` -> `IN (C)`
- bugfix: Corrected opcode `ED71` behavior
  - `OUT (C), F` -> `OUT (C),0`

## Version 1.4.0 (Spt 4, 2022 JST)

- Pull Request: #35
  - Remove LR35902 features
  - Add zexall to the standard test case
  - Fixed refresh register update timing
  - Add wait prefeatch / preread / prewrite feature (optional)
  - Change CI Tool: TravisCI -> CircleCI
  - Add CI test mode

## Version 1.3.0 (Spt 3, 2022 JST)

- Pull Request: #30
  - implemented the all of undocumented instructions
  - add test case: [Z80 Instruction Exerciser](https://mdfs.net/Software/Z80/Exerciser/)
  - passed `zexdoc` and `zexall`

## Version 1.2.8 (Feb 20, 2022 JST)

- implemented undocumented features: https://github.com/suzukiplan/z80/pull/27

## Version 1.2.7 (Dec 1, 2021 JST)

- bugfix: memory leak when using `addBreakOperand`, `addBreakPoint`, `addCallHandler` or `addReturnHandler`, and not remove them

## Version 1.2.6 (Nov 30, 2021 JST)

- Make the `readByte` and `writeByte` methods public instead of private for allow external programs to read and write memory via the CPU (synchronously). 
- Add the ability to handle CALL/RET

## Version 1.2.5 (Oct 23, 2021 JST)

- bugfix: interrupt mode 2 (IM2) did not work as expected.

## Version 1.2.4 (Jan 30, 2021 JST)

- bugfix: incorrect log POP 16bit register

## Version 1.2.3 (Jan 21, 2021 JST)

- bugfix: P/V flag condition of after CPI/CPD/CPIR/CPDR instruction has incorrect

## Version 1.2.2 (Spt 7, 2020 JST)

- Fixed a bug that jumps to an invalid address when a relative jump destination address e of JR and DJNZ are a boundary case.

## Version 1.2.1 (Aug 4, 2020 JST)

- bugfix: NMI will not generate after 2nd times or later (NMI flag did not clear by RETN)

## Version 1.2.0 (Jul 30, 2020 JST)

- support LR35902 (GBZ80) compatible mode

## Version 1.1.1 (Jul 29, 2020 JST)

- bugfix: R register not increment after executed opcode.

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
