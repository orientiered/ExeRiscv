#pragma once
#include <cstdint>
#include <vector>
#include <array>

#include "instr_decode.hpp"
#include "read_elf.hpp"


class InterpreterState {
public:
    constexpr static inline uint32_t REG_COUNT = 32;
    constexpr static inline uint32_t STACK_SIZE = 16 * 1024;
    constexpr static inline uint32_t REG_SP = 2;
    constexpr static inline uint32_t INSTR_SIZE = sizeof(Instr);

    InterpreterState(ParsedElf &elf);
    // ===== State registers and memory ====
    std::array<uint32_t, REG_COUNT> regs;  
    uint32_t pc;
    uint32_t next_pc;
    uint32_t base_addr;
    std::vector<uint8_t> memory;
    
    // ==== Convinience ====
    uint32_t PhysAddr(uint32_t virt_addr) {
        return virt_addr - base_addr;
    }

    uint32_t VirtAddr(uint32_t phys_addr) {
        return phys_addr + base_addr;
    }

    template<typename T>
    T read(uint32_t virt_addr) {
        return *reinterpret_cast<T*>(memory.data() + PhysAddr(virt_addr));
    }

    template<typename T>
    void write(T val, uint32_t virt_addr) {
        *reinterpret_cast<T*>(memory.data() + PhysAddr(virt_addr)) = val;
    }

    uint32_t fetch() {
        return *reinterpret_cast<uint32_t *>(memory.data() + PhysAddr(pc));
    }
    void incrementPC() {
        pc += INSTR_SIZE;
    }
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
        s.regs[i.rd] = D::compute(s.regs[i.rs1], s.regs[i.rs2]);
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
        s.regs[i.rd] = D::compute(s.regs[i.rs1], i.imm());
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
        s.regs[i.rd] = D::compute(i.imm());
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
        s.regs[i.rd] = s.pc + 4;
        s.next_pc = s.pc + i.imm();
        return 0;
    }
};

