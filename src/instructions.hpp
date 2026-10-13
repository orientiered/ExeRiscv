#pragma once

#include "model.hpp"


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
DECLARE_INSTR_R(Sll, 0b0110011, 0x7, 0x00, return a << b;);
DECLARE_INSTR_R(Srl, 0b0110011, 0x7, 0x00, return a >> b;);
DECLARE_INSTR_R(Sra, 0b0110011, 0x7, 0x00, 
    return static_cast<int32_t>(a) >> b;
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
    static uint32_t compute(uint32_t r, uint32_t imm) { return r << imm; }
};

struct Srli : InstructionBase<Srli, IType> {
    static constexpr uint32_t opcode = 0b0010011;
    static constexpr uint32_t funct3 = 0x5;

    static bool matches(const Instr& i_raw) {
        IType i = FromRaw<IType>(i_raw);
        return i.opcode == opcode && i.funct3 == funct3 && 
               ImmSlice(i.imm(), 5, 11) == 0x00;
    }
    static uint32_t compute(uint32_t r, uint32_t imm) { return r >> imm; }
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
        return static_cast<int32_t>(r) >> imm; 
    }
};

DECLARE_INSTR_I(Slti, 0b0010011, 0x2, return static_cast<int32_t>(r) < imm ? 1 : 0;);
DECLARE_INSTR_I(Sltiu, 0b0010011, 0x2, return (r < imm) ? 1 : 0;);

// =========== I TYPE INSTRUCTIONS: loads ===================

struct Lb : InstructionBase<Lb, IType> {
    static constexpr uint32_t opcode = 0b0000011;
    static constexpr uint32_t funct3 = 0x0;

    static int execute(InterpreterState& s, const Instr& i_raw) {
        IType i = FromRaw<IType>(i_raw);
        s.regs[i.rd] = s.read<int8_t>(s.regs[i.rs1] + i.imm());
        return 0;
    }
};

struct Lh : InstructionBase<Lb, IType> {
    static constexpr uint32_t opcode = 0b0000011;
    static constexpr uint32_t funct3 = 0x1;

    static int execute(InterpreterState& s, const Instr& i_raw) {
        IType i = FromRaw<IType>(i_raw);
        s.regs[i.rd] = s.read<int16_t>(s.regs[i.rs1] + i.imm());
        return 0;
    }
};

struct Lw : InstructionBase<Lb, IType> {
    static constexpr uint32_t opcode = 0b0000011;
    static constexpr uint32_t funct3 = 0x2;

    static int execute(InterpreterState& s, const Instr& i_raw) {
        IType i = FromRaw<IType>(i_raw);
        s.regs[i.rd] = s.read<int32_t>(s.regs[i.rs1] + i.imm());
        return 0;
    }
};

struct Lbu : InstructionBase<Lb, IType> {
    static constexpr uint32_t opcode = 0b0000011;
    static constexpr uint32_t funct3 = 0x4;

    static int execute(InterpreterState& s, const Instr& i_raw) {
        IType i = FromRaw<IType>(i_raw);
        s.regs[i.rd] = s.read<uint8_t>(s.regs[i.rs1] + i.imm());
        return 0;
    }
};

struct Lhu : InstructionBase<Lb, IType> {
    static constexpr uint32_t opcode = 0b0000011;
    static constexpr uint32_t funct3 = 0x5;

    static int execute(InterpreterState& s, const Instr& i_raw) {
        IType i = FromRaw<IType>(i_raw);
        s.regs[i.rd] = s.read<uint16_t>(s.regs[i.rs1] + i.imm());
        return 0;
    }
};

// =========== S TYPE INSTRUCTIONS ===================

#define DECLARE_INSTR_S(Name_, op_, f3_, ... ) \
    struct Name_ : InstructionBase<Name_, SType> {  \
        static constexpr uint32_t opcode = op_;     \
        static constexpr uint32_t funct3 = f3_;     \
        static void compute(InterpreterState &s, uint32_t r1, uint32_t r2, uint32_t imm) { __VA_ARGS__ } \
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
DECLARE_INSTR_B(Bltu, 0b1100011, 0x5, return r1 < r2;);
DECLARE_INSTR_B(Bqeu, 0b1100011, 0x6, return r1 >= r2;);


// =========== Special INSTRUCTIONS ===================





} // namespace RV32I

