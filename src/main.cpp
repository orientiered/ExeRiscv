#include <iostream>
#include <stdexcept>
#include "read_elf.hpp"

int main(int argc, const char *argv[]) {
    
    ElfReader reader;

    if (argc < 2) {
        std::cerr << "Expected file name";
        return 1;
    }
    std::string file_name = argv[1];

    ParsedElf elf;
    try {
        elf = reader.parseElf(file_name);
    } catch (std::runtime_error &e) {
        std::cerr << "Elf parse error: " << e.what();
        return 1;
    }

    std::cout << elf.base_addr << " " << elf.entry_point << "\n";


    for (uint32_t i = elf.entry_point - elf.base_addr, g = 0; i < elf.vmem.size() && g < 30; i += 4, g++) {
        printf("%x %x\n", i + elf.base_addr, *reinterpret_cast<uint32_t*>(elf.vmem.data() + i));  
    }

    
    return 0;
}