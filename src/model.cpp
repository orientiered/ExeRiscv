#include "model.hpp"
#include "instr_decode.hpp"
#include "read_elf.hpp"
#include <cstring>
#include "instructions.hpp"

InterpreterState::InterpreterState(ParsedElf& elf, int argc, const char **argv ) {

    // Initializing registers
    for (auto & reg: regs) reg = 0;

    // Setting up entry point and base addr
    pc = elf.entry_point;
    base_addr = elf.base_addr;

    // Creating memory for interpreter
    const uint32_t align = 64; 
    auto round_up = [align](uint32_t n) {
        return (n + align - 1) / align * align;
    };
    // Using regions from elf aligned + stack
    uint32_t data_boundary = round_up(elf.vmem.size());

    uint32_t strings_size = 0;
    for (int c = 0; c < argc; c++) {
        strings_size += strlen(argv[c]) + 1;
    }
    uint32_t total_argv_len = strings_size + argc * sizeof(uint32_t);
    total_argv_len += argc * sizeof(uint32_t);
    total_argv_len = round_up(total_argv_len);

    uint32_t mem_size = data_boundary + total_argv_len + STACK_SIZE; 
    memory.resize(mem_size);
    // ============================================================
    //  elf sections              argv ptrs     argv strings
    // || ======= | .....| argc p1 p2 ... pn | s1 s2 s3 ... sn | 
    //    data_boundary +stack         +argc*4 + 4       +strings_size   
    //                     sp                                                       
    // elf sections 
    std::memcpy(memory.data(), elf.vmem.data(), elf.vmem.size());
    // argc, argv
    reinterpret_cast<uint32_t*>(memory.data() + data_boundary + STACK_SIZE)[0] = argc;
    for (uint32_t c = 0, ptr = data_boundary + STACK_SIZE + (argc+1)*4; c < argc; c++) {
            uint32_t len = strlen(argv[c]);
            void* offset = memory.data() + data_boundary + STACK_SIZE + 4;
            reinterpret_cast<uint32_t*>(offset)[c] = VirtAddr(ptr);
            std::memcpy(memory.data() + ptr, argv[c], len);
            ptr += len + 1;
    }

    // setting stack pointer at the top, aligned 
    regs[REG_SP] = VirtAddr(data_boundary + STACK_SIZE);    
}

int InterpreterState::execute() {
    for (;;) {
        Instr i = fetch();
        next_pc = pc + INSTR_SIZE;

        int result = dispatch(i, RV32IM_set());
        if (result == ILLEGAL_INSTR) {
            fprintf(stderr, "Illegal instruction encountered at pc=%x: %x\n", pc, i);
            return ILLEGAL_INSTR;
        } else if (result == HALT_CODE) {
            if (return_code) {
                return *return_code;
            } else {
                fprintf(stderr, "Halt without exit\n");
                return HALT_CODE;
            }
        }

        pc = next_pc;
    }
}