#include <iostream>
#include <stdexcept>
#include "model.hpp"
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

    InterpreterState interpreter(elf, argc-1, argv+1);

    if constexpr (ASAN) {
        int code = 0;
        try {
            code = interpreter.execute();
        } catch (std::runtime_error &e) {
            std::cerr << "Execute error: " << e.what();
            return 1;
        }
        return code;
    } else {
        return interpreter.execute();
    }
}