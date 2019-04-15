#ifndef SJASMPLUS_ASM_DEFINE_H
#define SJASMPLUS_ASM_DEFINE_H

#include <string>
#include <vector>
#include <map>
#include <boost/optional.hpp>

using boost::optional;

class CDefines {
public:
    // Return true if redefined an existing define
    bool set(const std::string &Name, const std::string &Value);

    bool unset(const std::string &Name);

    optional<std::string> get(const std::string &Name);

    bool setArray(const std::string &Name, const std::vector<std::string> &Arr);

    optional<const std::vector<std::string> &> getArray(const std::string &Name);

    bool unsetArray(const std::string &Name);

    // Clear both DEFINE and DEFARRAY tables
    void clear();

    // Checks if either DEFINE or DEFARRAY for given name exists
    bool defined(const std::string &Name);

private:
    std::map<std::string, std::string> DefineTable;
    std::map<std::string, std::vector<std::string>> DefArrayTable;

};

#endif //SJASMPLUS_ASM_DEFINE_H
