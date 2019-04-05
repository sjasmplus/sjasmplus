#ifndef SJASMPLUS_PARSER_DEFINES_H
#define SJASMPLUS_PARSER_DEFINES_H

#include <string>
#include <map>
#include <vector>
#include <boost/optional.hpp>

// Return true if redefined and existing define
bool setDefine(const std::string &Name, const std::string &Value);

bool unsetDefine(const std::string &Name);

boost::optional<std::string> getDefine(const std::string &Name);

bool setDefArray(const std::string &Name, const std::vector<std::string> &Arr);

boost::optional<const std::vector<std::string> &> getDefArray(const std::string &Name);

bool unsetDefArray(const std::string &Name);

// Clear both DEFINE and DEFARRAY tables
void clearDefines();

// Checks if either DEFINE or DEFARRAY for given name exists
bool ifDefName(const std::string &Name);

#endif //SJASMPLUS_PARSER_DEFINES_H
