#ifndef SJASMPLUS_MESSAGE_IF_H
#define SJASMPLUS_MESSAGE_IF_H

#include <string>
#include "fs.h"

namespace msg {

    struct Position {
        fs::path Source;
        std::size_t Line;
        std::size_t Column;
    };

    extern int WarningCount;
    extern int ErrorCount;

    template <typename P>
    struct IMessagePrinter {

        static void note(const Position &Pos, const std::string &Message) {
            P::note(Pos, Message);
        }

        static void warn(const Position &Pos, const std::string &Message) {
            P::warn(Pos, Message);
            ++WarningCount;
        }

        static void err(const Position &Pos, const std::string &Message) {
            P::err(Pos, Message);
            ++ErrorCount;
        }

        [[noreturn]] static void fatal(const Position &Pos, const std::string &Message) {
            ++ErrorCount;
            P::fatal(Pos, Message);
        }

    };

} // namespace msg

#endif //SJASMPLUS_MESSAGE_IF_H
