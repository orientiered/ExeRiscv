#include "model.hpp"
#include "instr_decode.hpp"
#include "read_elf.hpp"
#include <cstring>

InterpreterState::InterpreterState(ParsedElf& elf) {

    // Initializing registers
    for (auto & reg: regs) reg = 0;

    // Setting up entry point and base addr
    pc = elf.entry_point;
    base_addr = elf.base_addr;

    // Creating memory for interpreter
    const uint32_t align = 64; 
    // Using regions from elf aligned + stack
    uint32_t mem_size = (elf.vmem.size() + align - 1) / align * align + STACK_SIZE; 
    memory.resize(mem_size);
    std::memcpy(memory.data(), elf.vmem.data(), elf.vmem.size());

    // setting stack pointer at the top, aligned 
    regs[REG_SP] = VirtAddr(mem_size & ~(align-1)); 
}
