#include "model.hpp"
#include "instr_decode.hpp"
#include "read_elf.hpp"
#include <cstring>
#include "instructions.hpp"

// ========= Dispatch table building =================
using InstrExecuteFn = int (*)(InterpreterState&, const Instr&);
struct Handler {
    bool (*matches)(const Instr&);
    int  (*execute)(InterpreterState&, const Instr&);
};

struct OpcodeSlot {
    uint16_t offset;   // flat array index
    uint8_t  count;    // number of instruction with current opcode 
};

template <class... Is>
struct DispatchTable {
    std::array<Handler, sizeof...(Is)> flat;
    std::array<OpcodeSlot, 128>        slots;
};

template <class... Is>
consteval DispatchTable<Is...> buildDispatch(std::tuple<Is...>) {
    DispatchTable<Is...> t{};
    size_t pos = 0;
    for (uint32_t opc = 0; opc < 128; ++opc) {
        const size_t start = pos;
        // gathering instr with same opcode together in flat array
        ((Is::opcode == opc
            ? (void)(t.flat[pos++] = Handler{ &Is::matches, &Is::execute })
            : (void)0), ...);
        t.slots[opc] = OpcodeSlot{
            static_cast<uint16_t>(start),
            static_cast<uint8_t>(pos - start)
        };
    }
    return t;
}


// Core
int dispatch(InterpreterState& s, const Instr& i) {

    static constexpr auto kDispatch = buildDispatch(RV32IM::set{});

    const auto& slot = kDispatch.slots[DecodeOpcode(i)];
    const Handler* hs = kDispatch.flat.data() + slot.offset;
    for (uint8_t k = 0; k < slot.count; ++k) {
        if (hs[k].matches(i))
            return hs[k].execute(s, i);
    }
    return ILLEGAL_INSTR;  // illegal instruction
}

InterpreterState::InterpreterState(ParsedElf& elf, int argc, const char **argv ) {

    // Initializing registers
    for (auto & reg: regs) reg = 0;

    // Setting up entry point and base addr
    pc = elf.entry_point;
    base_addr = elf.base_addr;
    regs[REG_GP] = elf.global_ptr;

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

        int result = dispatch(*this, i);
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