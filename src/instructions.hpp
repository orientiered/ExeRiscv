#pragma once

#include "model.hpp"
#include <iostream>
#include <unistd.h>

// source: 
// https://www.cs.sfu.ca/~ashriram/Courses/CS295/assets/notebooks/RISCV/RISCV_GREEN_CARD.pdf
namespace RV32I {

// =========== R TYPE INSTRUCTIONS ===================
#define DECLARE_INSTR_R(Name_, op_, f3_, f7_, ... ) \
    struct Name_ : InstructionBase<Name_, RType> {  \
        static constexpr uint32_t opcode = op_;     \
        static constexpr uint32_t funct3 = f3_;     \
        static constexpr uint32_t funct7 = f7_;     \
        static uint32_t compute(uint32_t a, uint32_t b) { __VA_ARGS__ } \
    }

DECLARE_INSTR_R(Add, 0b0110011, 0x0, 0x00, return a + b;);
DECLARE_INSTR_R(Sub, 0b0110011, 0x0, 0x20, return a - b;);
DECLARE_INSTR_R(Xor, 0b0110011, 0x4, 0x00, return a ^ b;);
DECLARE_INSTR_R(Or,  0b0110011, 0x6, 0x00, return a | b;);
DECLARE_INSTR_R(And, 0b0110011, 0x7, 0x00, return a & b;);
DECLARE_INSTR_R(Sll, 0b0110011, 0x1, 0x00, return a << ImmSlice(b, 0, 4););
DECLARE_INSTR_R(Srl, 0b0110011, 0x5, 0x00, return a >> ImmSlice(b, 0, 4););
DECLARE_INSTR_R(Sra, 0b0110011, 0x5, 0x20, 
    return static_cast<int32_t>(a) >> ImmSlice(b, 0, 4);
);
DECLARE_INSTR_R(Slt, 0b0110011, 0x2, 0x00, 
    return static_cast<int32_t>(a) < static_cast<int32_t>(b) ? 1 : 0;
);
DECLARE_INSTR_R(Sltu, 0b0110011, 0x3, 0x00, return (a < b) ? 1 : 0;);


// =========== I TYPE INSTRUCTIONS ===================

#define DECLARE_INSTR_I(Name_, op_, f3_, ... ) \
    struct Name_ : InstructionBase<Name_, IType> {  \
        static constexpr uint32_t opcode = op_;     \
        static constexpr uint32_t funct3 = f3_;     \
        static uint32_t compute(uint32_t r, uint32_t imm) { __VA_ARGS__ } \
    }

DECLARE_INSTR_I(Addi, 0b0010011, 0x0, return r + imm;);
DECLARE_INSTR_I(Xori, 0b0010011, 0x4, return r ^ imm;);
DECLARE_INSTR_I(Ori,  0b0010011, 0x6, return r | imm;);
DECLARE_INSTR_I(Andi, 0b0010011, 0x7, return r & imm;);

struct Slli : InstructionBase<Slli, IType> {
    static constexpr uint32_t opcode = 0b0010011;
    static constexpr uint32_t funct3 = 0x1;

    static bool matches(const Instr& i_raw) {
        IType i = FromRaw<IType>(i_raw);
        return i.opcode == opcode && i.funct3 == funct3 && 
               ImmSlice(i.imm(), 5, 11) == 0x00;
    }
    static uint32_t compute(uint32_t r, uint32_t imm) { return r << ImmSlice(imm, 0, 4); }
};

struct Srli : InstructionBase<Srli, IType> {
    static constexpr uint32_t opcode = 0b0010011;
    static constexpr uint32_t funct3 = 0x5;

    static bool matches(const Instr& i_raw) {
        IType i = FromRaw<IType>(i_raw);
        return i.opcode == opcode && i.funct3 == funct3 && 
               ImmSlice(i.imm(), 5, 11) == 0x00;
    }
    static uint32_t compute(uint32_t r, uint32_t imm) { return r >> ImmSlice(imm, 0, 4); }
};

struct Sali : InstructionBase<Sali, IType> {
    static constexpr uint32_t opcode = 0b0010011;
    static constexpr uint32_t funct3 = 0x5;

    static bool matches(const Instr& i_raw) {
        IType i = FromRaw<IType>(i_raw);
        return i.opcode == opcode && i.funct3 == funct3 && 
               ImmSlice(i.imm(), 5, 11) == 0x20;
    }
    static uint32_t compute(uint32_t r, uint32_t imm) { 
        return static_cast<int32_t>(r) >> ImmSlice(imm, 0, 4); 
    }
};

DECLARE_INSTR_I(Slti, 0b0010011, 0x2, return static_cast<int32_t>(r) < static_cast<int32_t>(imm) ? 1 : 0;);
DECLARE_INSTR_I(Sltiu, 0b0010011, 0x3, return (r < imm) ? 1 : 0;);

// =========== I TYPE INSTRUCTIONS: loads ===================

struct Lb : InstructionBase<Lb, IType> {
    static constexpr uint32_t opcode = 0b0000011;
    static constexpr uint32_t funct3 = 0x0;

    static int execute(InterpreterState& s, const Instr& i_raw) {
        IType i = FromRaw<IType>(i_raw);
        s.setReg(i.rd, s.read<int8_t>(s.regs[i.rs1] + i.imm()));
        return 0;
    }
};

struct Lh : InstructionBase<Lh, IType> {
    static constexpr uint32_t opcode = 0b0000011;
    static constexpr uint32_t funct3 = 0x1;

    static int execute(InterpreterState& s, const Instr& i_raw) {
        IType i = FromRaw<IType>(i_raw);
        s.setReg(i.rd, s.read<int16_t>(s.regs[i.rs1] + i.imm()));
        return 0;
    }
};

struct Lw : InstructionBase<Lw, IType> {
    static constexpr uint32_t opcode = 0b0000011;
    static constexpr uint32_t funct3 = 0x2;

    static int execute(InterpreterState& s, const Instr& i_raw) {
        IType i = FromRaw<IType>(i_raw);
        s.setReg(i.rd, s.read<int32_t>(s.regs[i.rs1] + i.imm()));
        return 0;
    }
};

struct Lbu : InstructionBase<Lbu, IType> {
    static constexpr uint32_t opcode = 0b0000011;
    static constexpr uint32_t funct3 = 0x4;

    static int execute(InterpreterState& s, const Instr& i_raw) {
        IType i = FromRaw<IType>(i_raw);
        s.setReg(i.rd, s.read<uint8_t>(s.regs[i.rs1] + i.imm()));
        return 0;
    }
};

struct Lhu : InstructionBase<Lhu, IType> {
    static constexpr uint32_t opcode = 0b0000011;
    static constexpr uint32_t funct3 = 0x5;

    static int execute(InterpreterState& s, const Instr& i_raw) {
        IType i = FromRaw<IType>(i_raw);
        s.setReg(i.rd, s.read<uint16_t>(s.regs[i.rs1] + i.imm()));
        return 0;
    }
};

// =========== S TYPE INSTRUCTIONS ===================

#define DECLARE_INSTR_S(Name_, op_, f3_, ... ) \
    struct Name_ : InstructionBase<Name_, SType> {  \
        static constexpr uint32_t opcode = op_;     \
        static constexpr uint32_t funct3 = f3_;     \
        static void store(InterpreterState &s, uint32_t r1, uint32_t r2, uint32_t imm) { __VA_ARGS__ } \
    }

DECLARE_INSTR_S(Sb, 0b0100011, 0x0, s.write<uint8_t>(r2, r1+imm););
DECLARE_INSTR_S(Sh, 0b0100011, 0x1, s.write<uint16_t>(r2, r1+imm););
DECLARE_INSTR_S(Sw, 0b0100011, 0x2, s.write<uint32_t>(r2, r1+imm););

// =========== B TYPE INSTRUCTIONS ===================

#define DECLARE_INSTR_B(Name_, op_, f3_, ... ) \
    struct Name_ : InstructionBase<Name_, BType> {  \
        static constexpr uint32_t opcode = op_;     \
        static constexpr uint32_t funct3 = f3_;     \
        static bool test(uint32_t r1, uint32_t r2) { __VA_ARGS__ } \
    }

DECLARE_INSTR_B(Beq, 0b1100011, 0x0, return r1 == r2;);
DECLARE_INSTR_B(Bne, 0b1100011, 0x1, return r1 != r2;);
DECLARE_INSTR_B(Blt, 0b1100011, 0x4, return static_cast<int32_t>(r1) < static_cast<int32_t>(r2););
DECLARE_INSTR_B(Bge, 0b1100011, 0x5, return static_cast<int32_t>(r1) >= static_cast<int32_t>(r2););
DECLARE_INSTR_B(Bltu, 0b1100011, 0x6, return r1 < r2;);
DECLARE_INSTR_B(Bgeu, 0b1100011, 0x7, return r1 >= r2;);


// =========== Special INSTRUCTIONS ===================

struct Jal : InstructionBase<Jal, JType> {
    static constexpr uint32_t opcode = 0b1101111;
};

struct Jalr : InstructionBase<Jalr, IType> {
    static constexpr uint32_t opcode = 0b1100111;
    static constexpr uint32_t funct3 = 0x0;


    static int execute(InterpreterState& s, const Instr& i_raw) {
        IType i = FromRaw<IType>(i_raw);
        uint32_t target = s.regs[i.rs1] + i.imm();
        s.setReg(i.rd, s.pc + INSTR_SIZE);
        s.next_pc = target;
        return 0;
    }
};

struct Lui : InstructionBase<Lui, UType> {
    static constexpr uint32_t opcode = 0b0110111;

    static int execute(InterpreterState& s, const Instr& i_raw) {
        UType i = FromRaw<UType>(i_raw);
        s.setReg(i.rd, i.imm());
        return 0;
    }
};

struct Auipc : InstructionBase<Auipc, UType> {
    static constexpr uint32_t opcode = 0b0010111;

    static int execute(InterpreterState& s, const Instr& i_raw) {
        UType i = FromRaw<UType>(i_raw);
        s.setReg(i.rd, s.pc + i.imm());
        return 0;
    }
};

struct Ecall : InstructionBase<Ecall, IType> {
    static constexpr uint32_t opcode = 0b1110011;
    static constexpr uint32_t funct3 = 0x0;

    static bool matches(const Instr& i_raw) {
        IType i = FromRaw<IType>(i_raw);
        return i.opcode == opcode && i.funct3 == funct3 && i.imm() == 0x0;
    }

    static int execute(InterpreterState& s, const Instr& i_raw) {
        const uint32_t syscall_read = 63;
        const uint32_t syscall_write = 64;
        const uint32_t syscall_exit = 93;

        uint32_t call_code = s.regs[REG_A0 + 7];
        switch (call_code) {
            case syscall_exit:
                s.return_code = s.regs[REG_A0];
                return HALT_CODE;
            break;
            case syscall_read: {
                ssize_t val = read(s.regs[REG_A0], s.MemAddr(s.regs[REG_A0+1]), s.regs[REG_A0+2]);
                s.setReg(REG_A0, val);
            }
            break;
            case syscall_write: {
                ssize_t val = write(s.regs[REG_A0], s.MemAddr(s.regs[REG_A0+1]), s.regs[REG_A0+2]);
                s.setReg(REG_A0, val);
            }
            break;
        }
        return 0;
    }
};

struct Ebreak : InstructionBase<Ecall, IType> {
    static constexpr uint32_t opcode = 0b1110011;
    static constexpr uint32_t funct3 = 0x0;

    static bool matches(const Instr& i_raw) {
        IType i = FromRaw<IType>(i_raw);
        return i.opcode == opcode && i.funct3 == funct3 && i.imm() == 0x1;
    }

    static int execute(InterpreterState& s, const Instr& i_raw) {
        std::cerr << "Debugging is not supported\n";
        return HALT_CODE;
    }
};


using set = std::tuple<
    Add, Sub, Xor, Or, And, Sll, Srl, Sra, Slt, Sltu,
    Addi, Xori, Ori, Andi, Slli, Srli, Sali, Slti, Sltiu,
    Lb, Lh, Lw, Lbu, Lhu,
    Sb, Sh, Sw, 
    Beq, Bne, Blt, Bge, Bltu, Bgeu,
    Jal, Jalr, Lui, Auipc, Ecall, Ebreak
>;

} // namespace RV32I


namespace RV32ext_M {

DECLARE_INSTR_R(Mul, 0b0110011, 0x0, 0x01, return a * b; );
DECLARE_INSTR_R(Mulh, 0b0110011, 0x1, 0x01, return (static_cast<int64_t>(a) * static_cast<int64_t>(b)) >> 32; );
DECLARE_INSTR_R(Mulu, 0b0110011, 0x2, 0x01, return (static_cast<uint64_t>(a) * static_cast<uint64_t>(b)) >> 32; );
DECLARE_INSTR_R(Mulsu, 0b0110011, 0x3, 0x01, return (static_cast<int64_t>(a) * static_cast<uint64_t>(b)) >> 32; );
DECLARE_INSTR_R(Div, 0b0110011, 0x4, 0x01, return static_cast<int32_t>(a) / static_cast<int32_t>(b); );
DECLARE_INSTR_R(Divu, 0b0110011, 0x5, 0x01, return a / b; );
DECLARE_INSTR_R(Rem, 0b0110011, 0x6, 0x01, return static_cast<int32_t>(a) % static_cast<int32_t>(b); );
DECLARE_INSTR_R(Remu, 0b0110011, 0x7, 0x01, return a % b; );

using set = std::tuple<
    Mul, Mulh, Mulu, Mulsu, Div, Divu, Rem, Remu
>;

} // namespace RV32ext_M

#define CAT_TUPLE_DEF(T1, T2) \
    decltype(std::tuple_cat(std::declval<T1>(), std::declval<T2>()))

namespace RV32IM {
    using set = CAT_TUPLE_DEF(RV32I::set, RV32ext_M::set);
}
