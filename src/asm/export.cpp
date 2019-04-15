#include <string>

#include "export.h"

using namespace std::string_literals;

void ExportWriter::init(fs::path &FileName) {
    open(FileName);
}

void ExportWriter::write(const std::string &Name, aint Value) {
    if (!OFS.is_open()) {
        TextOutput::open(FileName);
    }
    TextOutput::write(Name + ": EQU 0x"s + toHex32(Value) + "\n"s);
}
