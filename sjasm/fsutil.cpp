#include "global.h"
#include "fsutil.h"

fs::path getAbsPath(const fs::path &p) {
    return fs::absolute(p, global::CurrentDirectory);
}

