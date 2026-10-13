#pragma once

#include <stdint.h>
#include <string_view>
#include <vector>

using RiscvInstr = uint32_t;

struct ParsedElf {
    uint32_t entry_point;
    uint32_t base_addr;
    std::vector<uint8_t> vmem;
};


class ElfReader {
public:
    ParsedElf parseElf(std::string_view file_name);
private:
    std::vector<uint8_t> m_file;
};