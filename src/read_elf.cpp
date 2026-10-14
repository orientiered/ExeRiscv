#include <cstring>
#include <elf.h>
#include <filesystem>
#include <fstream>
#include <ios>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <fstream>

#include "read_elf.hpp"


template <typename T>
T reinterpretLoad(const void* ptr)
{
    T result;
    std::memcpy(&result, ptr, sizeof(T));
    return result;
}

ParsedElf ElfReader::parseElf(std::string_view file_name)
{

    std::ifstream file(file_name.data(), std::ios_base::binary | std::ios_base::in);
    if (!file.good())
        throw std::runtime_error("Cant open " + std::string(file_name));

    std::size_t fsize = std::filesystem::file_size(file_name);
    
    m_file.resize(fsize);
    file.read(reinterpret_cast<char*>(m_file.data()), fsize);
    

    // ===== Reading header =====
    Elf32_Ehdr header = reinterpretLoad<Elf32_Ehdr>(m_file.data());
    if (std::memcmp(header.e_ident, ELFMAG, SELFMAG) != 0) {
        throw std::runtime_error("Invalid elf file");
    }

    if (header.e_machine != EM_RISCV) {
        throw std::runtime_error("Unsupported machine type");
    }

    if (header.e_type != ET_EXEC) {
        throw std::runtime_error("Unsupported elf type");
    }

    ParsedElf result;
    result.entry_point = header.e_entry;

    // ====== Parsing program headers ======
    std::uint32_t min_virt_addr = std::numeric_limits<std::uint32_t>::max();
    std::uint32_t max_virt_addr = std::numeric_limits<std::uint32_t>::min();

    for (int i = 0; i < header.e_phnum; i++) {
        const void* offset = m_file.data() + header.e_phoff + i * header.e_phentsize;
        Elf32_Phdr prog_hdr = reinterpretLoad<Elf32_Phdr>(offset);

        if (prog_hdr.p_type == PT_LOAD) {
            min_virt_addr = std::min(min_virt_addr, prog_hdr.p_vaddr);
            max_virt_addr = std::max(max_virt_addr, prog_hdr.p_vaddr + prog_hdr.p_memsz);
        }
    }

    // calculating base offset addr for virtual memory 
    result.base_addr = min_virt_addr;
    result.vmem.resize(max_virt_addr - min_virt_addr);
        
    for (int i = 0; i < header.e_phnum; i++) {
        const void* offset = m_file.data() + header.e_phoff + i * header.e_phentsize;
        Elf32_Phdr prog_hdr = reinterpretLoad<Elf32_Phdr>(offset);

        if (prog_hdr.p_type == PT_LOAD) {
            std::memcpy(result.vmem.data() + prog_hdr.p_vaddr - result.base_addr, 
                        m_file.data() + prog_hdr.p_offset, 
                        prog_hdr.p_filesz);
        }
    }

    // ======= Parsing section and symtab to find __global_pointer$ symbol
    
    for (int i = 0; i < header.e_shnum; i++) {
        const void* offset = m_file.data() + header.e_shoff + i * header.e_shentsize;
        Elf32_Shdr symtab_shdr = reinterpretLoad<Elf32_Shdr>(offset);

        if (symtab_shdr.sh_type != SHT_SYMTAB) 
            continue;

        Elf32_Shdr strtab_shdr = reinterpretLoad<Elf32_Shdr>(m_file.data() + header.e_shoff + 
            symtab_shdr.sh_link * header.e_shentsize);

        size_t num_syms = symtab_shdr.sh_size / sizeof(Elf32_Sym);
        char *strs = reinterpret_cast<char*>(m_file.data() + strtab_shdr.sh_offset);
        Elf32_Sym *syms = reinterpret_cast<Elf32_Sym*>(m_file.data() + symtab_shdr.sh_offset); 

        bool found = false;

        for (size_t i = 0; i < num_syms; i++) {

            char *name = &strs[syms[i].st_name];
            if (strcmp(name, "__global_pointer$") == 0) {
                result.global_ptr = syms[i].st_value;
                found = true;
                break;
            }
        }

        if (found) break;
    }

    return result;
}
