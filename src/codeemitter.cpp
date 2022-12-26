#include <cassert>

#include "codeemitter.h"

optional<std::string> CodeEmitter::emitByte(uint8_t Byte) {
    const std::string ErrMsg{"CPU address space overflow"s};
    if (CPUAddrOverflow) {
        return ErrMsg;
    } else if (EmitAddrOverflow) {
        return ErrMsg + " (DISP shift = "s + std::to_string(EmitAddress - CPUAddress) + ")"s;
    }
    if (MemManager.isActive()) {
        MemManager.writeByte(getEmitAddress(), Byte);
    }
    if (RawOStream != nullptr) {
        RawOStream->write((char *)&Byte, 1);
    }
    incAddress();
    return std::nullopt;
}

// Increase address and return true on overflow
bool CodeEmitter::incAddress() {
    CPUAddress++;
    if (CPUAddress == 0)
        CPUAddrOverflow = true;
    if (Disp) {
        EmitAddress++;
        if (EmitAddress == 0)
            EmitAddrOverflow = true;
    }
    if (CPUAddress == 0 || (Disp && EmitAddress == 0))
        return true;
    return false;
}

// DISP directive
void CodeEmitter::doDisp(uint16_t DispAddress) {
    EmitAddress = CPUAddress;
    EmitAddrOverflow = CPUAddrOverflow;
    CPUAddress = DispAddress;
    Disp = true;
}

// ENT directive (undoes DISP)
void CodeEmitter::doEnt() {
    CPUAddress = EmitAddress;
    CPUAddrOverflow = EmitAddrOverflow;
    Disp = false;
}

void CodeEmitter::setRawOutputOptions(bool EnableOrOverride, const fs::path &FileName,
                                      const fs::path &_ForcedOutputDirectory, std::stringstream *StringStream) {
    if (EnableOrOverride) {
        RawOutputOverride = !FileName.empty();
        setRawOutput(FileName);
    }
    ForcedOutputDirectory = _ForcedOutputDirectory;

    if (StringStream != nullptr) {
        RawOutputOverride = true;
        setRawOutput(StringStream);
    }
}

void CodeEmitter::setRawOutput(const fs::path &FileName, OutputMode Mode) {
    if (RawOFS.is_open()) {
        RawOFS.close();
        RawOStream = nullptr;
        enforceFileSize();
    }
    auto OpenMode = std::ios_base::binary | std::ios_base::in | std::ios_base::out;
    switch (Mode) {
        case OutputMode::Truncate:
            OpenMode |= std::ios_base::trunc;
            break;
        case OutputMode::Append:
            OpenMode |= std::ios_base::ate;
            break;
        default:
            break;
    }
    RawOutputEnable = true;
    RawOutputFileName = ForcedOutputDirectory.empty() ? FileName : resolveOutputPath(FileName);
    RawOFS.open(RawOutputFileName, OpenMode);
    RawOStream = &RawOFS;
}

void CodeEmitter::setRawOutput(std::stringstream *SStream) {
    RawOStream = SStream;
    RawOutputEnable = true;
}

optional<std::string> CodeEmitter::seekRawOutput(std::streamoff Offset, std::ios_base::seekdir Method) {
    if (RawOStream != nullptr) {

        std::streampos NewPos;
        if (Method == std::ios_base::cur) {
            NewPos = RawOStream->tellp() + Offset;
        } else {
            NewPos = Offset;
        }

        RawOStream->seekp(Offset, Method);

        if (RawOStream->tellp() != NewPos) {
            return "Could not seek to position "s + std::to_string(Offset) +
                   " of file "s + RawOutputFileName.string();
        }
    }
    return std::nullopt;
}

void CodeEmitter::enforceFileSize() {
    // File must be closed at this point
    assert(!RawOFS.is_open());
    if (ForcedRawOutputSize > 0) {
        auto Size = fs::file_size(RawOutputFileName);
        if (ForcedRawOutputSize < Size) {
            Warning("File "s + RawOutputFileName.string() +
                    " truncated by SIZE directive by "s +
                    std::to_string(Size - ForcedRawOutputSize) + " bytes"s);
        }
        fs::resize_file(RawOutputFileName, ForcedRawOutputSize);
    }
}

fs::path CodeEmitter::resolveOutputPath(const fs::path &p) {
    if (!ForcedOutputDirectory.empty()) {
        return fs::absolute(ForcedOutputDirectory / p);
    } else {
        return getAbsPath(p);
    }
}

optional<std::string> CodeEmitter::align(uint16_t Alignment, optional<uint8_t> FillByte) {
    if (Alignment > 0x8000 || Alignment < 2)
        return "Invalid alignment value: "s + std::to_string(Alignment);
    while (getCPUAddress() % Alignment != 0) {
        if (FillByte) {
            auto Err = emitByte(*FillByte);
            if (Err) return Err;
        } else {
            incAddress();
        }
    }
    return std::nullopt;
}
