#ifndef SJASMPLUS_TESTS_Z80TESTUTIL_H
#define SJASMPLUS_TESTS_Z80TESTUTIL_H

#include <string>
#include <array>

using namespace std::string_literals;

#include <catch2/generators/catch_generators_range.hpp>
#include <catch2/generators/catch_generators_random.hpp>
#include <catch2/generators/catch_generators_adapters.hpp>


const std::array<std::string, 8> TableR = {
        "B", "C", "D", "E", "H", "L", "(HL)", "A"
};

const std::array<std::string, 4> TableRP = {
        "BC", "DE", "HL", "SP"
};

const std::array<std::string, 4> TableRP2 = {
        "BC", "DE", "HL", "AF"
};

const std::array<std::string, 8> TableCC = {
    "NZ", "Z", "NC", "C", "PO", "PE", "P", "M"
};

const std::array<std::string, 8> TableALU = {
    "ADD A,", "ADC A,", "SUB", "SBC A,", "AND", "XOR", "OR", "CP"
};

// Convert given relative displacement [-128...127] into textual representation relative to `$`
// Used by DJNZ, JR, JC cc
auto dispToText(int Disp) -> std::string {
    return Disp >= 0 ? "$ + 2 + "s + std::to_string(Disp) :
           "$ + 2 - "s + std::to_string(std::abs(Disp));
};

#endif //SJASMPLUS_TESTS_Z80TESTUTIL_H
