#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <istream>
#include <vector>
#include <map>
#include <cctype>
#include "CPU.h"

enum InstructionTypes
{
    MemoryInstructionType = 0, /// Eg: Load memory address %X into $A. Things that only reference one memory address and one register
    RegisterInstructionType, /// Eg: Load an immediate into a register. Things that only reference at least one register.
    ImmediateInstructionType, /// Reference neither registers nor memory.
    InstructionTypesSize /// Sentinel
};
#define NUM_INSTRUCTION_TYPE_SELECTION_BITS 2

#define NUM_PREDICATE_BITS (1 + NUM_REGISTER_BITS)
typedef void (*MIType)(CPU&, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t);
void MILoadMemoryRegister(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4, uint8_t val5);
void MILoadMemoryImmediate(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4, uint8_t val5);
void MIStoreMemoryRegister(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4, uint8_t val5);
void MIStoreMemoryImmediate(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4, uint8_t val5);

enum MemoryInstructions
{
    LoadMemoryRegister = 0, /// Loads a memory address into the given register. requires: 1 mem, 1 register
    LoadMemoryImmediate,
    StoreMemoryRegister,
    StoreMemoryImmediate, /// Stores a register into a memory address. requires: 1 mem, 1 register
    MemoryInstructionsSize /// Sentinel
};
static MIType MI_insts[MemoryInstructionsSize] = {&MILoadMemoryRegister, &MILoadMemoryImmediate,
    &MIStoreMemoryRegister, &MIStoreMemoryImmediate};
static const std::string MI_asm[MemoryInstructionsSize] = {"load", "store"};
static const int MI_args[MemoryInstructionsSize] = {2, 2};

#define NUM_MEMORY_INSTRUCTIONS_SELECTION_BITS 2
static_assert(MemoryInstructionsSize <= (1 << NUM_MEMORY_INSTRUCTIONS_SELECTION_BITS), "NUM_INSTRUCTION_TYPE_SELECTION_BITS too low for number of instructions.");
static_assert(NUM_INSTRUCTION_TYPE_SELECTION_BITS + 
              NUM_PREDICATE_BITS + 
              NUM_MEMORY_INSTRUCTIONS_SELECTION_BITS +
              PHYSICAL_MEMORY_SIZE_BITS +
              NUM_REGISTER_BITS <= INSTRUCTION_SIZE_BITS, "Too few bits in instruction for memory instruction.");

typedef void (*RIType)(CPU&, uint8_t, uint8_t, uint8_t, uint8_t);
void RILoadImmediate(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4);
void RILoadRegister(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4);
void RIAddImmediate(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4);
void RIAddRegister(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4);
void RIAddImmediateSaveCarry(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4);
void RIAddRegisterSaveCarry(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4);
void RIMulImmediate(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4);
void RIMulRegister(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4);
void RIMulImmediateSaveCarry(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4);
void RIMulRegisterSaveCarry(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4);
void RIDivImmediateRegister(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4);
void RIDivRegisterImmediate(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4);
void RIDivRegisterRegister(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4);
void RIModImmediateRegister(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4);
void RIModRegisterImmediate(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4);
void RIModRegisterRegister(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4);
void RIAndImmediate(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4);
void RIAndRegister(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4);
void RIOrImmediate(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4);
void RIOrRegister(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4);
void RIXorImmediate(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4);
void RIXorRegister(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4);
void RIBitwiseComplement(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4);

enum RegisterInstructions
{
    LoadImmediate = 0, /// Loads an immediate value into a register. requires: 1 register, 1 immediate
    LoadRegister, ///Copy a register
    AddImmediate, /// Unsigned adds a immediate value to a register. requires: 1 register, 1 immediate
    AddRegister, /// Unsigned adds register value to a register. requires: 2 register
    AddImmediateSaveCarry, /// Unsigned adds an immediate to a register and saves any carry into a another register requires: 3 registers
    AddRegisterSaveCarry, /// Unsigned adds a register to a register and saves any carry into a another register requires: 3 registers
    MulImmediate, /// Unsigned multiplies a register and an immediate, High order bits are discarded
    MulRegister, /// Unsigned multiplies two registers. The high-order bits are discarded.
    MulImmediateSaveCarry, /// Unsigned multiplies a register and an immediate, High order bits are saved to the specified register.
    MulRegisterSaveCarry, /// Unsigned multiplies two registers. The high-order bits are saved to the specified register.
    DivImmediateRegister, /// Performs signed integer division of an immediate and a register. Needs 2 registers and 1 immediate.
    DivRegisterImmediate, /// Performs signed integer division of a register and an immediate. Needs 2 registers and 1 immediate.
    DivRegisterRegister, /// Performs unsigned integer division of two registers.
    ModImmediateRegister, /// Calculates the remainder of unsigned division of an immediate and a register. Needs 2 registers and 1 immediate,
    ModRegisterImmediate, /// Calculates the remainder of unsigned division of a register and an immediate. Needs 2 registers and 1 immediate,
    ModRegisterRegister, /// Calculates the remainder of unsigned division of two register. Needs 3 registers.
    AndImmediate, /// Bitwise-ands a register and immediate. Needs 2 registers and 1 immediate.
    AndRegister, /// Bitwise-ands two registers. Needs 3 registers.
    OrImmediate, /// Bitwise-ors a register and immediate. Needs 2 registers and 1 immediate.
    OrRegister, /// Bitwise-ors two registers. Needs 3 registers.
    XorImmediate, /// Bitwise-xors a register and immediate. Needs 2 registers and 1 immediate..
    XorRegister, /// Bitwise-xors two registers. Needs 3 registers.
    BitwiseComplement, /// Bitwise-complements a register. Needs 2 registers
    RegisterInstructionSize
};
static RIType RI_insts[RegisterInstructionSize] = {&RILoadImmediate, &RILoadRegister, &RIAddImmediate, &RIAddRegister,
&RIAddImmediateSaveCarry, &RIAddRegisterSaveCarry, &RIMulImmediate, &RIMulRegister, &RIMulImmediateSaveCarry,
&RIMulRegisterSaveCarry, &RIDivImmediateRegister, &RIDivRegisterImmediate, &RIDivRegisterRegister, &RIModImmediateRegister,
&RIModRegisterImmediate, &RIModRegisterRegister, &RIAndImmediate, &RIAndRegister, &RIOrImmediate, &RIOrRegister,
&RIXorImmediate, &RIXorRegister, &RIBitwiseComplement};
static const std::string RI_asm[RegisterInstructionSize] = {"loadi", "loadr", "addi", "addr",
"addic", "addrc", "muli", "mulr", "mulic", "mulrc", "divir", "divri", "divrr", "modir",
"modri", "modrr", "andi", "andr", "ori", "orr", "xori", "xorr", "bcomp"};
static const int RI_args[RegisterInstructionSize] = {2, 2, 3, 3,
4, 4, 3, 3, 4, 4, 3, 3, 3, 3,
3, 3, 3, 3, 3, 3, 3, 3, 2};

#define NUM_REGISTRY_INSTRUCTIONS_SELECTION_BITS 6
static_assert(RegisterInstructionSize <= (1 << NUM_REGISTRY_INSTRUCTIONS_SELECTION_BITS), "NUM_REGISTRY_INSTRUCTIONS_SELECTION_BITS too low for number of instructions.");
static_assert(NUM_INSTRUCTION_TYPE_SELECTION_BITS + 
              NUM_PREDICATE_BITS + 
              NUM_MEMORY_INSTRUCTIONS_SELECTION_BITS + 
              3*NUM_REGISTER_BITS +
              NUM_WORD_BITS <= INSTRUCTION_SIZE_BITS, "Too few bits in instruction for registry instructions.");

typedef void (*IIType)(CPU&, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t);
void IIJumpImmediateQuad(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4, uint8_t val5);
void IIJumpRegisterQuad(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4, uint8_t val5);
void IIJumpBackRegisterQuad(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4, uint8_t val5);
void IIJumpBackRegisterQuad(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4, uint8_t val5);
void IIHaltImmediateQuad(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4, uint8_t val5);
void IIHaltRegisterQuad(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4, uint8_t val5);
void IISetStackAddressImmediateQuadAddress(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4, uint8_t val5);
void IISetStackAddressRegisterQuadAddress(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4, uint8_t val5);
void IIPushStackRegisterArguments(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4, uint8_t val5);
void IIPushStackImmediateArguments(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4, uint8_t val5);
void IIPopStack(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4, uint8_t val5);
void IIPrintToScreenImmediate(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4, uint8_t val5);
void IIPrintToScreenRegister(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4, uint8_t val5);
void IISetInterruptHandlerRoutineImmediate(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4, uint8_t val5);
void IISaveInterruptReasonRegister(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4, uint8_t val5);

enum ImmediateInstructions
{
    JumpImmediateQuad, /// Increment program counter by immediate quad, unsigned
    JumpRegisterQuad, /// Increment program counter by register quad, unsigned
    JumpBackImmediateQuad, /// Decrement program counter by immediate quad, unsigned
    JumpBackRegisterQuad, /// Decrement program counter by register quad, unsigned
    HaltImmediateQuad, /// Stops execution return the immediate.
    HaltRegisterQuad, /// Stops execution returning the register quad.
    SetStackAddressImmediateQuadAddress, /// Sets the stack address to the immediate address
    SetStackAddressRegisterQuadAddress, /// Sets the stack address to the register quad address
    PushStackRegisterArguments, /// Pushes a frame onto the stack.
    PushStackImmediateArguments, /// Pushes a frame onto the stack.
    PopStack, /// Pops a stack frame and jumps to the return address
    PrintToScreenImmediate,
    PrintToScreenRegister,
    SetInterruptHandlerRoutineImmediate,
    SaveInterruptReasonRegister,
    ImmediateInstructionSize
};
static IIType II_insts[ImmediateInstructionSize] = {IIJumpImmediateQuad, &IIJumpRegisterQuad, &IIJumpBackRegisterQuad, &IIJumpBackRegisterQuad,
&IIHaltImmediateQuad, &IIHaltRegisterQuad, &IISetStackAddressImmediateQuadAddress, &IISetStackAddressRegisterQuadAddress,
&IIPushStackRegisterArguments, &IIPushStackImmediateArguments,
&IIPopStack, &IIPrintToScreenImmediate, &IIPrintToScreenRegister,
&IISetInterruptHandlerRoutineImmediate, &IISaveInterruptReasonRegister};
static const std::string II_asm[ImmediateInstructionSize] = {"jumpiq", "jumprq", "bjumpiq", "bjumprq",
"haltiq", "haltrq", "setstkiq", "setstkrq", "pushstkr", "pushstki",
"popstk", "prti", "prtr", "setihriq", "saveirr"};
static const int II_args[ImmediateInstructionSize] = {1, 4, 1, 4,
1, 4, 1, 4, -1, -1,
0, 1, 1, 1, 1};

#define NUM_IMMEDIATE_INSTRUCTIONS_SELECTION_BITS 5
static_assert(ImmediateInstructionSize <= (1 << NUM_IMMEDIATE_INSTRUCTIONS_SELECTION_BITS), "NUM_IMMEDIATE_INSTRUCTIONS_SELECTION_BITS too low for number of instructions.");
static_assert(NUM_INSTRUCTION_TYPE_SELECTION_BITS + 
              NUM_PREDICATE_BITS + 
              NUM_IMMEDIATE_INSTRUCTIONS_SELECTION_BITS + 
              5*NUM_REGISTER_BITS <= INSTRUCTION_SIZE_BITS, "Too few bits in instruction for immediate instruction.");
static_assert(NUM_INSTRUCTION_TYPE_SELECTION_BITS + 
              NUM_PREDICATE_BITS + 
              NUM_IMMEDIATE_INSTRUCTIONS_SELECTION_BITS + 
              1*NUM_REGISTER_BITS + 
              4*NUM_WORD_BITS <= INSTRUCTION_SIZE_BITS, "Too few bits in instruction for immediate instruction.");
static_assert(NUM_INSTRUCTION_TYPE_SELECTION_BITS + 
              NUM_PREDICATE_BITS + 
              NUM_IMMEDIATE_INSTRUCTIONS_SELECTION_BITS + 
              5*NUM_WORD_BITS <= INSTRUCTION_SIZE_BITS, "Too few bits in instruction for immediate instruction.");

static_assert(NUM_REGISTER_BITS <= 8, "Too many bits for register.");

void CPU::perform_instruction(uint64_t instruction)
{
    uint64_t has_predicate = instruction & 1;
    uint64_t predicate_register = (instruction & ((1 << NUM_PREDICATE_BITS) - 1)) >> 1;
    
    if(!has_predicate || registers[predicate_register])
    {
        uint64_t pure_instruction = instruction >> NUM_PREDICATE_BITS;
        uint64_t type = pure_instruction & ((1 << NUM_INSTRUCTION_TYPE_SELECTION_BITS) - 1);
        uint64_t args = pure_instruction >> NUM_INSTRUCTION_TYPE_SELECTION_BITS;
        
        if(type == MemoryInstructionType)
        {
            uint32_t func = args & ((1 << NUM_MEMORY_INSTRUCTIONS_SELECTION_BITS) - 1);
            args >>= NUM_MEMORY_INSTRUCTIONS_SELECTION_BITS;
            uint8_t val_1 = args & ((1 << NUM_REGISTER_BITS) - 1);
            args >>= NUM_REGISTER_BITS;
            uint8_t val_2 = args & ((1 << NUM_REGISTER_BITS) - 1);
            args >>= NUM_REGISTER_BITS;
            uint8_t val_3 = args & ((1 << NUM_REGISTER_BITS) - 1);
            args >>= NUM_REGISTER_BITS;
            uint8_t val_4 = args & ((1 << NUM_REGISTER_BITS) - 1);
            args >>= NUM_REGISTER_BITS;
            uint8_t val_5 = args & ((1 << NUM_REGISTER_BITS) - 1);
            
            if(func < MemoryInstructionsSize)
            {
                MI_insts[func](*this, val_1, val_2, val_3, val_4, val_5);
            }
        }
        else if (type == RegisterInstructionType)
        {
            uint32_t func = args & ((1 << NUM_REGISTRY_INSTRUCTIONS_SELECTION_BITS) - 1);
            args >>= NUM_REGISTRY_INSTRUCTIONS_SELECTION_BITS;
            uint8_t val_1 = args & ((1 << NUM_REGISTER_BITS) - 1);
            args >>= NUM_REGISTER_BITS;
            uint8_t val_2 = args & ((1 << NUM_REGISTER_BITS) - 1);
            args >>= NUM_REGISTER_BITS;
            uint8_t val_3 = args & ((1 << NUM_REGISTER_BITS) - 1);
            args >>= NUM_REGISTER_BITS;
            uint8_t val_4 = args & ((1 << NUM_REGISTER_BITS) - 1);
            
            if(func < RegisterInstructionSize)
            {
                RI_insts[func](*this, val_1, val_2, val_3, val_4);
            }
        }
        else if(type == ImmediateInstructionType)
        {
            uint64_t func = args & ((1 << NUM_IMMEDIATE_INSTRUCTIONS_SELECTION_BITS) - 1);
            args >>= NUM_IMMEDIATE_INSTRUCTIONS_SELECTION_BITS;
            uint8_t val_1 = args & ((1 << NUM_REGISTER_BITS) - 1);
            args >>= NUM_REGISTER_BITS;
            uint8_t val_2 = args & ((1 << NUM_REGISTER_BITS) - 1);
            args >>= NUM_REGISTER_BITS;
            uint8_t val_3 = args & ((1 << NUM_REGISTER_BITS) - 1);
            args >>= NUM_REGISTER_BITS;
            uint8_t val_4 = args & ((1 << NUM_REGISTER_BITS) - 1);
            args >>= NUM_REGISTER_BITS;
            uint8_t val_5 = args & ((1 << NUM_REGISTER_BITS) - 1);
            
            if(func < RegisterInstructionSize)
            {
                II_insts[func](*this, val_1, val_2, val_3, val_4, val_5);
            }
        }
        else
        {
            ///Invalid instruction type
        }
    }
    
    program_counter += 8;
}

void MILoadMemoryRegister(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4, uint8_t val5)
{
    uint32_t value = (cpu.registers[val1] << 24) | (cpu.registers[val2] << 16) | (cpu.registers[val3] << 8) | (cpu.registers[val4]);
    cpu.memory[value] = cpu.registers[val5];
}

void MILoadMemoryImmediate(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4, uint8_t val5)
{
    uint32_t value = (val1 << 24) | (val2 << 16) | (val3 << 8) | (val4);
    cpu.registers[val5] = cpu.memory[value];
}

void MIStoreMemoryRegister(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4, uint8_t val5)
{
    uint32_t value = (cpu.registers[val1] << 24) | (cpu.registers[val2] << 16) | (cpu.registers[val3] << 8) | (cpu.registers[val4]);
    cpu.memory[value] = cpu.registers[val5];
}

void MIStoreMemoryImmediate(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4, uint8_t val5)
{
    uint32_t value = (val1 << 24) | (val2 << 16) | (val3 << 8) | (val4);
    cpu.memory[value] = cpu.registers[val5];
}

void RILoadImmediate(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4)
{
    cpu.registers[val1] = val2;
}

void RILoadRegister(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4)
{
    cpu.registers[val1] = cpu.registers[val2];
}

void RIAddImmediate(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4)
{
    cpu.registers[val1] = cpu.registers[val2] + val3;
}

void RIAddRegister(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4)
{
    cpu.registers[val1] = cpu.registers[val2] + cpu.registers[val3];
}

void RIAddImmediateSaveCarry(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4)
{
    uint16_t sum = cpu.registers[val3] + val4;
    cpu.registers[val1] = sum & 0xFF;
    cpu.registers[val2] = (sum >> 8) & 0xFF;
}

void RIAddRegisterSaveCarry(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4)
{
    uint16_t sum = cpu.registers[val3] + cpu.registers[val4];
    cpu.registers[val1] = sum & 0xFF;
    cpu.registers[val2] = (sum >> 8) & 0xFF;
}

void RIMulImmediate(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4)
{
    uint16_t product = cpu.registers[val2]*val3;
    cpu.registers[val1] = product & 0xFF;
}

void RIMulRegister(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4)
{
    uint16_t product = cpu.registers[val2]*cpu.registers[val3];
    cpu.registers[val1] = product & 0xFF;
}

void RIMulImmediateSaveCarry(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4)
{
    uint16_t product = cpu.registers[val4]*val3;
    cpu.registers[val1] = product & 0xFF;
    cpu.registers[val2] = (product >> 8) & 0xFF;
}

void RIMulRegisterSaveCarry(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4)
{
    uint16_t product = cpu.registers[val4]*cpu.registers[val3];
    cpu.registers[val1] = product & 0xFF;
    cpu.registers[val2] = (product >> 8) & 0xFF;
}

void RIDivImmediateRegister(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4)
{
    uint8_t quotient = val2/cpu.registers[val3];
    cpu.registers[val1] = quotient;
}

void RIDivRegisterImmediate(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4)
{
    uint8_t quotient = cpu.registers[val2]/val3;
    cpu.registers[val1] = quotient;
}

void RIDivRegisterRegister(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4)
{
    uint8_t quotient = cpu.registers[val2]/cpu.registers[val3];
    cpu.registers[val1] = quotient;
}

void RIModImmediateRegister(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4)
{
    uint8_t modulus = val2 % cpu.registers[val3];
    cpu.registers[val1] = modulus;
}

void RIModRegisterImmediate(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4)
{
    uint8_t modulus = cpu.registers[val2] % val3;
    cpu.registers[val1] = modulus;
}

void RIModRegisterRegister(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4)
{
    uint8_t modulus = cpu.registers[val2] % cpu.registers[val3];
    cpu.registers[val1] = modulus;
}

void RIAndImmediate(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4)
{
    uint8_t result = cpu.registers[val2] & val3;
    cpu.registers[val1] = result;
}

void RIAndRegister(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4)
{
    uint8_t result = cpu.registers[val2] & cpu.registers[val3];
    cpu.registers[val1] = result;
}

void RIOrImmediate(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4)
{
    uint8_t result = cpu.registers[val2] | val3;
    cpu.registers[val1] = result;
}

void RIOrRegister(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4)
{
    uint8_t result = cpu.registers[val2] | cpu.registers[val3];
    cpu.registers[val1] = result;
}

void RIXorImmediate(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4)
{
    uint8_t result = cpu.registers[val2] ^ val3;
    cpu.registers[val1] = result;
}

void RIXorRegister(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4)
{
    uint8_t result = cpu.registers[val2] ^ cpu.registers[val3];
    cpu.registers[val1] = result;
}

void RIBitwiseComplement(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4)
{
    uint8_t result = ~cpu.registers[val2];
    cpu.registers[val1] = result;
}

void IIJumpImmediateQuad(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4, uint8_t val5)
{
    uint32_t value = (val1 << 24) | (val2 << 16) | (val3 << 8) | (val4);
    cpu.program_counter += value;
    cpu.program_counter -= 8;
    
}

void IIJumpRegisterQuad(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4, uint8_t val5)
{
    uint32_t value = (cpu.registers[val1] << 24) | (cpu.registers[val2] << 16) | (cpu.registers[val3] << 8) | (cpu.registers[val4]);
    cpu.program_counter -= value;
    cpu.program_counter -= 8;
}

void IIJumpBackImmediateQuad(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4, uint8_t val5)
{
    uint32_t value = (val1 << 24) | (val2 << 16) | (val3 << 8) | (val4);
    cpu.program_counter -= value;
    cpu.program_counter -= 8;
}

void IIJumpBackRegisterQuad(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4, uint8_t val5)
{
    uint32_t value = (cpu.registers[val1] << 24) | (cpu.registers[val2] << 16) | (cpu.registers[val3] << 8) | (cpu.registers[val4]);
    cpu.program_counter -= value;
    cpu.program_counter -= 8;
}

void IIHaltImmediateQuad(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4, uint8_t val5)
{
    uint32_t value = (val1 << 24) | (val2 << 16) | (val3 << 8) | (val4);
    exit(value);
}

void IIHaltRegisterQuad(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4, uint8_t val5)
{
    uint32_t value = (cpu.registers[val1] << 24) | (cpu.registers[val2] << 16) | (cpu.registers[val3] << 8) | (cpu.registers[val4]);
    exit(value);
}

void IISetStackAddressImmediateQuadAddress(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4, uint8_t val5)
{
    uint32_t value = (val1 << 24) | (val2 << 16) | (val3 << 8) | (val4);
    cpu.stack_address = value;
}

void IISetStackAddressRegisterQuadAddress(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4, uint8_t val5)
{
    uint32_t value = (cpu.registers[val1] << 24) | (cpu.registers[val2] << 16) | (cpu.registers[val3] << 8) | (cpu.registers[val4]);
    cpu.stack_address = value;
}

void IIPushStackRegisterArguments(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4, uint8_t val5)
{
    cpu.memory[cpu.stack_address++] = cpu.registers[val1];
    switch(val1)
    {
        case 1:
            cpu.memory[cpu.stack_address++] = cpu.registers[val2];
        case 2:
            cpu.memory[cpu.stack_address++] = cpu.registers[val3];
        case 3:
            cpu.memory[cpu.stack_address++] = cpu.registers[val4];
        default:
            cpu.memory[cpu.stack_address++] = cpu.registers[val5]; /// Default case = write mem address
        case 0:
            cpu.memory[cpu.stack_address++] = cpu.registers[val1];
    }
}

void IIPushStackImmediateArguments(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4, uint8_t val5)
{
//     cpu.memory[cpu.stack_address++] = val1;
    switch(val1)
    {
        case 1:
            cpu.memory[cpu.stack_address++] = val2;
        case 2:
            cpu.memory[cpu.stack_address++] = val3;
        case 3:
            cpu.memory[cpu.stack_address++] = val4;
        default:
            cpu.memory[cpu.stack_address++] = val5; /// Default case = write mem address
        case 0:
            cpu.memory[cpu.stack_address++] = val1;
    }
}

void IIPopStack(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4, uint8_t val5)
{
    uint8_t num_args = cpu.memory[--cpu.stack_address];
    cpu.stack_address -= num_args >= 4 ? 4 : num_args;
}

void IIPrintToScreenImmediate(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4, uint8_t val5)
{
    printf("%c", cpu.registers[val1]);
    fflush(stdout);
}

void IIPrintToScreenRegister(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4, uint8_t val5)
{
    printf("%c", cpu.registers[val1]);
    fflush(stdout);
}

void IISetInterruptHandlerRoutineImmediate(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4, uint8_t val5)
{
    uint32_t value = (val1 << 24) | (val2 << 16) | (val3 << 8) | (val4);
    cpu.exception_handler_routine_address = value;
}

void IISaveInterruptReasonRegister(CPU& cpu, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4, uint8_t val5)
{
    cpu.registers[val1] = cpu.exception_reason;
}

std::string trim(std::string in)
{
    int beg = 0;
    while(beg < int(in.size()) && isspace(in[beg]))
        ++beg;
    
    int end = int(in.size()) - 1;
    while(end >= 0 && isspace(in[end]))
        --end;
    
    if(beg > end)
        return "";
    else
        return in.substr(beg, end - beg + 1);
}

std::vector<uint64_t> parse_asm(FILE* in)
{
    std::vector<uint64_t> result;
    
    std::map<std::string, uint8_t> masm_to_idx;
    for(uint8_t i = 0; i < MemoryInstructionsSize; ++i)
        masm_to_idx[MI_asm[i]] = i;
    
    std::map<std::string, uint8_t> rasm_to_idx;
    for(uint8_t i = 0; i < RegisterInstructionSize; ++i)
        rasm_to_idx[RI_asm[i]] = i;
    
    std::map<std::string, uint8_t> iasm_to_idx;
    for(uint8_t i = 0; i < RegisterInstructionSize; ++i)
        iasm_to_idx[II_asm[i]] = i;
    
    std::map<std::string, std::size_t> labels_to_statement_idx;
    std::vector<std::string> statements;
    
    while(true)
    {
        uint64_t out_inst = 0;
        std::string statement = "";
        int predicated_on = -1;
        
        while(true)
        {
            char chr = fgetc(in);
            
            if(chr == ';')
                break;
            else
                statement.push_back(chr);
        }
        
        if(feof(in))
            break;
        
        std::size_t colon_pos = statement.find(":");
        
        if(colon_pos != std::string::npos)
        {
            std::string label = trim(statement.substr(0, colon_pos - 1));
            std::size_t idx = statements.size();
            labels_to_statement_idx[label] = idx;
            statement = trim(statement.substr(colon_pos + 1));
        }
        
        std::size_t qmark_pos = statement.find("?");
        
        if(qmark_pos != std::string::npos)
        {
            std::string predicate_register = trim(statement.substr(qmark_pos + 1));
            sscanf(predicate_register.c_str(), "%d", &predicated_on);
            statement = trim(statement.substr(0, qmark_pos - 1));
        }
        
        std::size_t first_word_boundary = statement.find(" ");
        std::string inst = statement.substr(0, first_word_boundary);
        
        if(masm_to_idx.count(inst) == 0)
        {
            std::string args;
            
            
        }
        else if(rasm_to_idx.count(inst) == 0)
        {
        }
        else if(iasm_to_idx.count(inst) == 0)
        {
        }
        else
        {
            fprintf(stderr, "Illegal instruction \"%s\"", inst.c_str());
            exit(1);
        }
        
        
    }
    
    return result;
}