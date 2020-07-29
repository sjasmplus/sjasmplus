#ifndef SJASMPLUS_PARSER_STRUCT_H
#define SJASMPLUS_PARSER_STRUCT_H

#include "asm/struct.h"

SMEMB getStructMemberId(const char *&P);

void parseStructLabel(const char *&P, CStruct &St);

void parseStructMember(const char *&P, CStruct &St);

#endif //SJASMPLUS_PARSER_STRUCT_H
