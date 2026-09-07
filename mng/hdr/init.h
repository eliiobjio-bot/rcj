#pragma once
#include "../hdr/interface.h"
#include <iostream>
#include <string>
struct init_entry_t {
    pid_t  pid; /* the pid of program */
    int8_t status;
    // const char* cmd;  /* the command */
    // const char* abbr; /* the program name */
    std::string cmd;
    std::string abbr;
};

struct args_log {
    std::string level;
    std::string file;
    std::string ip;
    uint16_t    port;
    uint32_t    file_max_size;
};

struct args_global {
    bool is_master;
};
struct args_conf_file {
    std::string conf_path;
};
struct all_args_t {
    args_global    global;
    args_log       log;
    args_conf_file conf;
};
int   SetConfigFilePath(std::string path);
int   init_phy_container();
int   init_router();
int   start_app();
int   stop_app();
int   waitpid(pid_t id, int ms);
int   start_an_app(int idx);
int   stop_an_app(int idx);
int   get_an_app_status(int idx);
int   Handle_Enb_Config_Modify(char*, char*, char*, char*);
pid_t Get_beam_pid(int beam_idx);