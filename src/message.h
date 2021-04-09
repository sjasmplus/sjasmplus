#ifndef SJASMPLUS_MESSAGE_H
#define SJASMPLUS_MESSAGE_H

#include <iostream>
#include <cstdlib>

#include "message_if.h"

using namespace std::string_literals;
using std::cerr;
using std::endl;

namespace msg {

    enum class Type {
        Note,
        Warning,
        Error
    };

    class MessagePrinter {
    public:

        static void note(const Position &Pos, const std::string &Message) {
            cerr << formatMsg(Type::Note, Pos, Message) << endl;
        }

        static void warn(const Position &Pos, const std::string &Message) {
            cerr << formatMsg(Type::Warning, Pos, Message) << endl;
        }

        static void err(const Position &Pos, const std::string &Message) {
            cerr << formatMsg(Type::Error, Pos, Message) << endl;
        }

        [[noreturn]] static void fatal(const Position &Pos, const std::string &Message) {
            err(Pos, Message);
            std::exit(EXIT_FAILURE);
        }

    private:
        static std::string formatMsg(Type MType, const Position &P, const std::string &Message) {
            return P.Source.string() + ":"s + std::to_string(P.Line) + ":"s +
                   std::to_string(P.Column) + ": "s + [&]() {
                switch (MType) {
                    case Type::Note:
                        return "note: "s;
                    case Type::Warning:
                        return "warning "s;
                    case Type::Error:
                    default:
                        return "error: "s;
                }
            }() + Message;

        }
    };

} // namespace msg

#endif //SJASMPLUS_MESSAGE_H
