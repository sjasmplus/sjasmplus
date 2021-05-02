#ifndef SJASMPLUS_MESSAGE_IF_H
#define SJASMPLUS_MESSAGE_IF_H

#include <cstdlib>
#include <string>
#include "fs.h"

namespace msg {

    enum class Type {
        Note,
        Warning,
        Error,
        Fatal
    };

    struct Position {
        fs::path Source;
        std::size_t Line;
        std::size_t Column;
    };

    extern int WarningCount;
    extern int ErrorCount;

    template <typename MPBackend>
    struct IMessagePrinter {

        using B = MPBackend;

        static void msg(const std::string &Message) {
            B::msg(Message);
        }

        static void note(const Position &Pos, const std::string &Message) {
            B::msg(Type::Note, Pos, Message);
        }

        static void warn(const Position &Pos, const std::string &Message) {
            ++WarningCount;
            B::msg(Type::Warning, Pos, Message);
        }

        static void err(const Position &Pos, const std::string &Message) {
            ++ErrorCount;
            B::msg(Type::Error, Pos, Message);
        }

        [[noreturn]] static void fatal(const Position &Pos, const std::string &Message) {
            ++ErrorCount;
            B::msg(Type::Error, Pos, Message);
            std::exit(EXIT_FAILURE);
        }

    };

} // namespace msg

#endif //SJASMPLUS_MESSAGE_IF_H
