#include <cstdint>
#include <string>
#include <vector>

#define INSTRUCTION_SIZE_BITS 64
#define PHYSICAL_MEMORY_SIZE_BITS 30
#define PHYSICAL_MEMORY_SIZE (1 << PHYSICAL_MEMORY_SIZE_BITS)
#define NUM_REGISTER_BITS 8
#define NUM_REGISTERS (1 << NUM_REGISTER_BITS)
#define NUM_WORD_BITS 8

static_assert(NUM_WORD_BITS == NUM_REGISTER_BITS, "Word size and num register bits must be same size.");

class CPU
{
public:
    uint8_t memory[PHYSICAL_MEMORY_SIZE];
    uint8_t registers[NUM_REGISTERS];
    uint32_t stack_address;
    uint32_t program_counter;
    uint32_t exception_handler_routine_address;
    uint8_t exception_reason;
    uint32_t errored_program_counter;
    
    void perform_instruction(uint64_t instruction);
};

std::vector<uint64_t> parse_asm(FILE* in);