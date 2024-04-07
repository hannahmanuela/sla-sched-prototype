#include <vector>
#include <string>
#include <cstring>

#include "proc_type.h"

#ifndef PROC_H
#define PROC_H

class Proc {

    public:
        Proc() {}

        Proc(int sla, vector<const char*> command, ProcType type) 
            : sla_(sla), time_spawned_(-1), command_(command), type_(type) {}
        
        ~Proc() {
            for (const char* cmd : command_) {
                delete[] cmd;
            }
        }

        void from_bytes(char* buffer) {
            std::memcpy(&sla_, buffer, sizeof(int));
            buffer += sizeof(int);
            std::memcpy(&time_spawned_, buffer, sizeof(int));
            buffer += sizeof(int);
            int command_size;
            std::memcpy(&command_size, buffer, sizeof(int));
            buffer += sizeof(int);
            for (int i = 0; i < command_size; ++i) {
                int cmd_len;
                std::memcpy(&cmd_len, buffer, sizeof(int));
                buffer += sizeof(int);
                char* cmd = new char[cmd_len + 1];
                std::memcpy(cmd, buffer, cmd_len);
                cmd[cmd_len] = '\0';
                buffer += cmd_len;
                command_.push_back(cmd);
            }
            std::memcpy(&type_, buffer, sizeof(ProcType));
        }
        void to_bytes(char* buffer) {
            std::memcpy(buffer, &sla_, sizeof(int));
            buffer += sizeof(int);
            std::memcpy(buffer, &time_spawned_, sizeof(int));
            buffer += sizeof(int);
            int command_size = command_.size();
            std::memcpy(buffer, &command_size, sizeof(int));
            buffer += sizeof(int);
            for (const char* cmd : command_) {
                int cmd_len = std::strlen(cmd);
                std::memcpy(buffer, &cmd_len, sizeof(int));
                buffer += sizeof(int);
                std::memcpy(buffer, cmd, cmd_len);
                buffer += cmd_len;
            }
            std::memcpy(buffer, &type_, sizeof(ProcType));
        }

        void print() {
            cout << "sla: " << sla_ << ", time_spawned: " << time_spawned_ << ", executable: " << command_[0] << endl;
        }

        
        vector<const char*> get_command() { return command_; }
        int get_sla() { return sla_; }
        // gets difference between argument (a time since the global start) and time_spawned
        int get_time_since_spawn(int curr_time) { return curr_time - time_spawned_; }
        ProcType get_type() { return type_; }

        void set_time_spawned(int ts) {time_spawned_ = ts;}

        
        void exec_proc() { 
            // need a null-terminated vector
            command_.push_back(NULL); 
            cout << "about to exec proc" << endl;
            execv(command_[0], const_cast<char* const*>(command_.data()));
            cout << "proc done?" << endl;
        }


    private:
        int sla_; // for now in ms
        int time_spawned_; // for now in ms, from global start time
        vector<const char*> command_;
        int cmd_size_;
        ProcType type_;

};

#endif  // PROC_H