#ifndef SJASMPLUS_LISTING_H
#define SJASMPLUS_LISTING_H

#include <cstdint>
#include <vector>
#include <string>
#include <stack>
#include <functional>

#include "util.h"


class ListingWriter : public TextOutput {
private:

    std::function<uint16_t()> getCPUAddress;
    std::function<int()> includeLevel;
    std::function<unsigned int()> maxLineNumber;
    std::function<int()> currentLine;


    bool IsActive = false;
    bool OmitLine = false;
    bool InMacro = false;
    std::stack<bool> MacroStack;
    std::vector<uint8_t> ByteBuffer;
    int UnitStartAddress;
    int LastEndAddress;
    int NumDigitsInLineNumber = 0;

    void listBytes4();

    void listBytes5();

    void listBytesLong(int pad, const std::string &Prefix);

    std::string printCurrentLocalLine();

public:
    ListingWriter() = default;

    void init0(
            std::function<uint16_t()> const &getCPUAddressFunc,
            std::function<int()> const &includeLevelFunc,
            std::function<unsigned int()> const &maxLineNumberFunc,
            std::function<int()> const &currentLineFunc
    ) {

        getCPUAddress = getCPUAddressFunc;
        includeLevel = includeLevelFunc;
        maxLineNumber = maxLineNumberFunc;
        currentLine = currentLineFunc;
    }

    void init(fs::path &FileName) override;

    void initPass();

    void listLine(const char *Line);

    void listLineSkip(const char *Line);

    void addByte(uint8_t Byte);

    void setUnitStartAddress() {
        UnitStartAddress = getCPUAddress();
    }

    void omitLine() {
        OmitLine = true;
    }

    void startMacro() {
        MacroStack.push(InMacro);
        InMacro = true;
    }

    void endMacro() {
        if (!MacroStack.empty()) {
            InMacro = MacroStack.top();
            MacroStack.pop();
        } else {
            InMacro = false;
        }
    }
};

#endif //SJASMPLUS_LISTING_H
