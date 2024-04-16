#include <vector>
#include <string>

#include "proc.h"
#include "proc_hist.h"

#ifndef MESSAGE_H
#define MESSAGE_H

enum MessageType { PROC, EXIT, HEARTBEAT, HEARTBEAT_RESPONSE };

class Message {
    public:
        MessageType type;
        Message(MessageType t) : type(t) {}
        Message() {}

        virtual void to_bytes(char* buffer) = 0;
};

class ProcMessage : public Message {

    public:
        Proc* msg_proc;
        
        ProcMessage() : Message(PROC) {}
        ProcMessage(Proc* proc) : Message(PROC), msg_proc(proc) {}

        void from_bytes(char* buffer) {
            std::memcpy(&type, buffer, sizeof(MessageType));
            buffer += sizeof(MessageType);
            msg_proc = new Proc();
            msg_proc->from_bytes(buffer);
        };
        void to_bytes(char* buffer) {
            std::memcpy(buffer, &type, sizeof(MessageType));
            buffer += sizeof(MessageType);
            msg_proc->to_bytes(buffer);
        };
};

class ExitMessage : public Message {
    public:        
        ExitMessage() : Message(EXIT) {}

        void to_bytes(char* buffer) {
            std::memcpy(buffer, &type, sizeof(MessageType));
        };
};

class HeartbeatMessage : public Message {
    public:        
        HeartbeatMessage() : Message(HEARTBEAT) {}

        void to_bytes(char* buffer) {
            std::memcpy(buffer, &type, sizeof(MessageType));
        };
};

class HeartbeatResponseMessage : public Message {

    public:
        ProcHist hist;
        HeartbeatResponseMessage() : Message(HEARTBEAT_RESPONSE) {}
        HeartbeatResponseMessage(ProcHist h) : Message(HEARTBEAT_RESPONSE), hist(h) {}

        void from_bytes(char* buffer) {
            std::memcpy(&type, buffer, sizeof(MessageType));
            buffer += sizeof(MessageType);
            hist.from_bytes(buffer);
        };
        void to_bytes(char* buffer) {
            std::memcpy(buffer, &type, sizeof(MessageType));
            buffer += sizeof(MessageType);
            hist.to_bytes(buffer);
        };
};


#endif  // MESSAGE_H