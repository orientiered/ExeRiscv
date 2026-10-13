#pragma once

#include <cstdint>
#define INSTR_DECOMP __attribute__((packed)) 


using Instr = uint32_t;

inline uint32_t DecodeOpcode(uint32_t instr) {
    return instr & 0b1'111'111; // first 7 bits
}

inline int32_t SignExtend(uint32_t imm, uint32_t last_bit_pos) {
    return static_cast<int32_t>(imm << (32 - last_bit_pos) ) >> (32 - last_bit_pos);
}

// slice 32 bit unsigned [start, stop]
inline uint32_t ImmSlice(uint32_t imm, uint32_t start, uint32_t stop) {
    uint32_t length = stop - start + 1;
    
    uint32_t mask = (length >= 32) ? ~0U : (1U << length) - 1;
    
    return (imm >> start) & mask;
}

struct INSTR_DECOMP RType {
    uint32_t opcode : 6+1;
    uint32_t rd     : 11-6;
    uint32_t funct3 : 14-11;
    uint32_t rs1    : 19-14;
    uint32_t rs2    : 24-19;
    uint32_t funct7 : 31-24;
};

struct INSTR_DECOMP IType {
    uint32_t opcode : 6+1;
    uint32_t rd     : 11-6;
    uint32_t funct3 : 14-11;
    uint32_t rs1    : 19-14;
    uint32_t imm11_0 : 31-24;

    int32_t imm() const {
        return SignExtend(imm11_0, 12);
    }

};

struct INSTR_DECOMP SType {
    uint32_t opcode : 6+1;
    uint32_t imm4_0 : 11-6;
    uint32_t funct3 : 14-11;
    uint32_t rs1    : 19-14;
    uint32_t rs2    : 24-19;
    uint32_t imm11_5 : 31-24;

    int32_t imm() const {
        return SignExtend((imm11_5 >> 5) + imm4_0, 12);
    }
};

struct INSTR_DECOMP BType {
    uint32_t opcode : 6+1;
    uint32_t imm11  : 7-6;
    uint32_t imm4_1 : 11-7;
    uint32_t funct3 : 14-11;
    uint32_t rs1    : 19-14;
    uint32_t rs2    : 24-19;
    uint32_t imm10_5 : 30-24;
    uint32_t imm12  : 31-30;

    int32_t imm() const {
        return SignExtend((imm12 >> 12) + (imm11 >> 11) + (imm10_5 >> 5) + (imm4_1 >> 1), 13); 
    }
};

struct INSTR_DECOMP UType {
    uint32_t opcode : 6+1;
    uint32_t rd     : 11-6;
    uint32_t imm31_12 : 31-11; 

    int32_t imm() const { // no sign extension
        return (imm31_12 >> 12);
    }
};

struct INSTR_DECOMP JType {
    uint32_t opcode : 6+1;
    uint32_t rd     : 11-6;
    uint32_t imm19_12 : 19-12+1;
    uint32_t imm11: 1;
    uint32_t imm10_1: 10-1+1;
    uint32_t imm20: 1; 

    int32_t imm() const {
        return SignExtend((imm20 >> 20) + (imm19_12 >> 12) + (imm11 >> 11) + (imm10_1 >> 1), 20);
    }
};

template <typename IFormat>
IFormat FromRaw(uint32_t instr) {
    union {
        uint32_t raw;
        IFormat f;
    } universal;
    universal.raw = instr;
    return universal.f;
}