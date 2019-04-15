#ifndef SJASMPLUS_PARSER_STRUCT_H
#define SJASMPLUS_PARSER_STRUCT_H

#include "asm/struct.h"

SMEMB getStructMemberId(const char *&p);

void parseStructLabel(CStruct &St);

void parseStructMember(CStruct &St);

#endif //SJASMPLUS_PARSER_STRUCT_H
