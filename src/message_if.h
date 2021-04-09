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

    template <typename T>
    class IMessagePrinter {

        static void note(const Position &Pos, const std::string &Message) {
            T::note(Pos, Message);
        }

        static void warn(const Position &Pos, const std::string &Message) {
            T::warn(Pos, Message);
        }

        static void err(const Position &Pos, const std::string &Message) {
            T::err(Pos, Message);
        }

        [[noreturn]] static void fatal(const Position &Pos, const std::string &Message) {
            T::fatal(Pos, Message);
        }

    };

} // namespace msg

#endif //SJASMPLUS_MESSAGE_IF_H
