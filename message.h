#include <vector>
#include <string>

#include "proc.h"

#ifndef MESSAGE_H
#define MESSAGE_H

class Message {

    public:
        int sender;
        bool is_exit;
        Proc* msg_proc;

        Message() : sender(0), is_exit(false), msg_proc(nullptr) {}
        Message(int sender, bool is_exit, Proc* proc) {
            this->is_exit = is_exit;
            this->sender = sender;
            msg_proc = proc;
        }

        void from_bytes(char* buffer) {
            std::memcpy(&sender, buffer, sizeof(int));
            buffer += sizeof(int);
            std::memcpy(&is_exit, buffer, sizeof(bool));
            buffer += sizeof(bool);
            msg_proc = new Proc();
            msg_proc->from_bytes(buffer);
        };
        void to_bytes(char* buffer) {
            std::memcpy(buffer, &sender, sizeof(int));
            buffer += sizeof(int);
            std::memcpy(buffer, &is_exit, sizeof(bool));
            buffer += sizeof(bool);
            msg_proc->to_bytes(buffer);
        };

};

#endif  // MESSAGE_H