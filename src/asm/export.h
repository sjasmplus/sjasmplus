#ifndef SJASMPLUS_ASM_EXPORT_H
#define SJASMPLUS_ASM_EXPORT_H

#include "util.h"

class ExportWriter : public TextOutput {
private:
    fs::path FileName;
public:
    explicit ExportWriter(fs::path &_FileName) : FileName{_FileName} {}
    void init(fs::path &FileName) override;
    void write(const std::string &Name, aint Value);
};

#endif //SJASMPLUS_ASM_EXPORT_H
