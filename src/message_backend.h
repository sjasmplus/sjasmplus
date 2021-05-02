#ifndef SJASMPLUS_MESSAGE_H
#define SJASMPLUS_MESSAGE_H

#include <iostream>

#include "message_if.h"

using namespace std::string_literals;
using std::cerr;
using std::endl;

namespace msg {

    class MPBackendIO {
    public:

        static void msg(const std::string &Message) {
            printL(Message);
        }

        static void msg(Type T, const Position &Pos, const std::string &Message) {
            printF(T, Pos, Message);
        }

    private:

        static std::string formatMsg(Type MType, const Position &P, const std::string &Message) {
            return P.Source.string() + ":"s + std::to_string(P.Line) + ":"s +
                   std::to_string(P.Column) + ": "s + [&]() {
                switch (MType) {
                    case Type::Note:
                        return "note: "s;
                    case Type::Warning:
                        return "warning: "s;
                    case Type::Error:
                    case Type::Fatal:
                    default:
                        return "error: "s;
                }
            }() + Message;

        }

        static void printF(Type MType, const Position &P, const std::string &Message) {
            printL(formatMsg(MType, P, Message));
        }

        static void printL(const std::string &Message) {
            cerr << Message << endl;
        }
    };

} // namespace msg

#endif //SJASMPLUS_MESSAGE_H
