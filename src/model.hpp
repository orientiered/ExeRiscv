#pragma once
#include <cstdint>
#include <cstdio>
#include <optional>
#include <stdexcept>
#include <vector>
#include <array>

#include "instr_decode.hpp"
#include "read_elf.hpp"

constexpr inline uint32_t REG_COUNT = 32;
constexpr inline uint32_t STACK_SIZE = 512 * 1024; // 512 kB
constexpr inline uint32_t REG_SP = 2;
constexpr inline uint32_t REG_GP = 3;
constexpr inline uint32_t REG_A0 = 10;
constexpr inline uint32_t INSTR_SIZE = sizeof(Instr);

constexpr inline int ILLEGAL_INSTR = -1;
constexpr inline int HALT_CODE = -2;

constexpr inline bool ASAN = true; 

class InterpreterState {
public:

    InterpreterState(ParsedElf &elf, int argc, const char **argv);
    // ===== State registers and memory ====
    std::array<uint32_t, REG_COUNT> regs;  
    uint32_t pc;
    uint32_t next_pc;
    uint32_t base_addr;
    std::vector<uint8_t> memory;
    
    std::optional<int> return_code = std::nullopt; 
    // ==== Convinience ====
    uint32_t PhysAddr(uint32_t virt_addr) {
        return virt_addr - base_addr;
    }

    uint32_t VirtAddr(uint32_t phys_addr) {
        return phys_addr + base_addr;
    }

    void* MemAddr(uint32_t virt_addr) {
        return memory.data() + PhysAddr(virt_addr);
    }

    void checkAddr(uint32_t virt_addr) {
        if constexpr (!ASAN) {
            return;
        } else {
            bool valid = virt_addr >= base_addr && virt_addr < base_addr + memory.size();
            if (!valid) {
                fprintf(stderr, "Invalid virtual address access encountered\n");
                fprintf(stderr, "pc %x : instr %x\n", pc, fetch());
                fprintf(stderr, "valid range: [%x:%zx), access to %x\n", base_addr, base_addr + memory.size(), virt_addr);
                throw std::runtime_error("Illegal memory access");
            }
        }
    }

    template<typename T>
    T read(uint32_t virt_addr) {
        checkAddr(virt_addr);
        return *reinterpret_cast<T*>(MemAddr(virt_addr));
    }

    template<typename T>
    void write(T val, uint32_t virt_addr) {
        checkAddr(virt_addr);
        *reinterpret_cast<T*>(MemAddr(virt_addr)) = val;
    }

    void setReg(uint32_t reg, uint32_t val) {
        if (reg == 0 || reg > REG_COUNT) return;
        regs[reg] = val;
    }

    uint32_t fetch() {
        return read<Instr>(pc);
    }

    int execute();
};

template <class Derived, class Format> struct InstructionBase;

template <class D>
struct InstructionBase<D, RType> {
    static bool matches(const Instr& i_raw) {
        RType i = FromRaw<RType>(i_raw);
        return i.opcode == D::opcode && i.funct3 == D::funct3 && i.funct7 == D::funct7;
    }
    static int execute(InterpreterState& s, const Instr& i_raw) {
        RType i = FromRaw<RType>(i_raw);
        s.setReg(i.rd, D::compute(s.regs[i.rs1], s.regs[i.rs2]));
        return 0;
    }
};

template <class D>
struct InstructionBase<D, IType> {
    static bool matches(const Instr& i_raw) {
        IType i = FromRaw<IType>(i_raw);
        return i.opcode == D::opcode && i.funct3 == D::funct3;
    }
    static int execute(InterpreterState& s, const Instr& i_raw) {
        IType i = FromRaw<IType>(i_raw);
        s.setReg(i.rd, D::compute(s.regs[i.rs1], i.imm()));
        return 0;
    }
};

template <class D>
struct InstructionBase<D, SType> {
    static bool matches(const Instr& i_raw) {
        SType i = FromRaw<SType>(i_raw);
        return i.opcode == D::opcode && i.funct3 == D::funct3;
    }
    static int execute(InterpreterState& s, const Instr& i_raw) {
        SType i = FromRaw<SType>(i_raw);
        D::store(s, s.regs[i.rs1], s.regs[i.rs2], i.imm());
        return 0;
    }
};

template <class D>
struct InstructionBase<D, BType> {
    static bool matches(const Instr& i_raw) {
        BType i = FromRaw<BType>(i_raw);
        return i.opcode == D::opcode && i.funct3 == D::funct3;
    }
    static int execute(InterpreterState& s, const Instr& i_raw) {
        BType i = FromRaw<BType>(i_raw);
        if (D::test(s.regs[i.rs1], s.regs[i.rs2]))
            s.next_pc = s.pc + i.imm();
        return 0;
    }
};

template <class D>
struct InstructionBase<D, UType> {
    static bool matches(const Instr& i_raw) { 
        UType i = FromRaw<UType>(i_raw);
        return i.opcode == D::opcode; 
    }
    static int execute(InterpreterState& s, const Instr& i_raw) {
        UType i = FromRaw<UType>(i_raw);
        s.setReg(i.rd, D::compute(i.imm()));
        return 0;
    }
};

template <class D>
struct InstructionBase<D, JType> {
    static bool matches(const Instr& i_raw) {
        JType i = FromRaw<JType>(i_raw);
        return i.opcode == D::opcode; 
    }
    static int execute(InterpreterState& s, const Instr& i_raw) {
        JType i = FromRaw<JType>(i_raw);
        s.setReg(i.rd, s.pc + 4);
        s.next_pc = s.pc + i.imm();
        return 0;
    }
};

