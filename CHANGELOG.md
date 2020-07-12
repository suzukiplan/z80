# Change log

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
