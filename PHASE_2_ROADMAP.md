# Phase 2: Pass 2 Implementation Roadmap

## Overview

**Phase 2: Step 2** - Convert symbolic assembly into executable CHIP-8 binary bytecode.

**Status:** Ready to begin
**Scope:** 6 sub-steps
**Estimated complexity:** Medium-High (larger than Phase 1.4-1.7 combined)

---

## Quick Reference: The 6 Steps

| Step | Task | Complexity | Dependencies |
|------|------|-----------|--------------|
| 2.1 | Symbol collection & first scan | LOW | None |
| 2.2 | Sprite allocation | LOW | 2.1 |
| 2.3 | Opcode encoding | HIGH | None (can parallel) |
| 2.4 | Second scan & emission | MEDIUM | 2.1, 2.2, 2.3 |
| 2.5 | Special cases | LOW-MEDIUM | 2.3 |
| 2.6 | Output & verification | LOW | 2.4 |

---

## Step Details

### 2.1: First Scan - Symbol Collection (15-20 min)
Walk through assembly, record each LABEL's memory address.
- Input: Vector of `Instruction` objects
- Output: `symbol_table_` populated with label → address mappings
- Key: Calculate address = 0x200 + 2 * instruction_count

### 2.2: Sprite Allocation (10-15 min)
Assign ROM addresses to sprites after code ends.
- Input: `sprite_alloc_` with sprite data
- Output: `symbol_table_` updated with sprite → address mappings
- Key: Check we don't overflow into stack region (0xE00)

### 2.3: Opcode Encoding (60-90 min)
Implement `encodeInstruction()` for all CHIP-8 opcodes.
- Input: Mnemonic + operands
- Output: 16-bit CHIP-8 opcode
- Priority: Start with LD, ADD, SUB, then control flow

**Opcodes needed (minimum):**
```
LD, ADD, SUB, AND, OR, XOR, SHL, SHR
JUMP, CALL, RET
SE, SNE
CLS, DRW
```

### 2.4: Second Scan & Emission (20-30 min)
Resolve all symbols, encode to binary bytes.
- Input: Symbolic assembly + symbol table
- Output: Binary bytes (big-endian)
- Process: For each instruction (skip LABELs):
  1. Resolve symbolic operands
  2. Call encodeInstruction()
  3. Emit bytes
  4. Emit sprite data at end

### 2.5: Special Cases (15-20 min)
Handle memory ops, sprite references, conditional branches.
- `LD [I], Vx` → Store to RAM
- `LD I, sprite_name` → Resolve sprite address
- `SE/SNE` → Conditional skip instructions

### 2.6: Output & Verification (10 min)
Write binary file, print summary, verify integrity.
- Output: Binary ROM file
- Summary: Code size, sprite size, total
- Checks: No unresolved symbols, within memory bounds, valid operands

---

## Key Data Structures

```cpp
// In CodeGenerator (extend existing)

// Enhanced symbol table with address mapping
struct SymbolTable {
    std::map<std::string, uint16_t> labels;      // label → address
    std::map<std::string, uint16_t> sprites;     // sprite → address
    std::map<std::string, uint16_t> functions;   // function → address
};

// Helper for opcode encoding
struct OperandValue {
    enum Type { REGISTER, ADDRESS, LITERAL };
    Type type;
    uint16_t value;
};

OperandValue resolveOperand(const std::string& operand_str);
uint16_t encodeInstruction(const Instruction& instr);
```

---

## Test Cases (in order)

**Test 1: test_simple.c8**
```c
uint8 x = 5;
x = x + 1;
```
Expected output: `out.bin` with ~10 bytes, no symbols

**Test 2: test_sample.c8 (with loop)**
```c
while (counter < 100) {
    // ...
    if (x >= 60) { x = 0; }
}
```
Expected output: `out.bin` with ~74 bytes, 4 resolved labels, sprite data

**Test 3: Custom test with all instruction types**
- All arithmetic ops
- All comparison ops
- Control flow (JUMP, CALL)
- Built-in functions (CLS, DRW)
- Mixed with variable assignments

---

## Success Criteria

When complete, chip8c should:
- ✓ Compile C-like source to CHIP-8 binary
- ✓ Generate executable ROM files
- ✓ Resolve all symbolic references
- ✓ Allocate sprites without overflow
- ✓ Run compiled programs in CHIP-8 emulator
- ✓ Print clear diagnostics on errors

---

## Files to Modify/Create

**Modify:**
- `src/codegen.cpp` - Add Pass 2 implementation
- `include/codegen.h` - Add new methods/structures

**Potentially add:**
- `include/chip8_opcodes.h` - Opcode definitions (optional)
- `test/test_opcode_encoding.cpp` - Unit tests for encoding

---

## Build & Test Commands

```bash
# Build
cd build && make -j4 chip8c

# Test simple program
./tools/chip8c ../tools/compiler/test_simple.c8

# Test complex program
./tools/chip8c ../tools/compiler/test_sample.c8

# Verify binary was generated
file out.bin           # Should be "data"
hexdump -C out.bin     # View bytes
```

---

## Notes

- CHIP-8 uses **big-endian** byte order (MSB first)
- All instructions are **16-bit** (2 bytes)
- ROM starts at **0x200**, sprites allocated after code
- Memory layout: 0x200-0xDFF (3328 bytes available)
- Stack is 0xE00-0xEFF, framebuffer 0xF00-0xFFF

---

## Next: Phase 2.5 (Optional)

Once Pass 2 is working:
- Implement logical operators (`&&`, `||`) with short-circuit
- Add diagnostic output (register summary, spill usage)
- Add listing file generation (`.lst`)
- Test on real CHIP-8 emulators

---
