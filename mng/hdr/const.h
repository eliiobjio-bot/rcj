#pragma once
#include <cstdint>
#include <unistd.h>  
// const char *p_bridge_name = "bridge";
// 10.102.%d.%d
#define SUB_NET_FIELD "10.102"
//#define SUCCESS 0
#define OK 1
#define ERR (-1)

#define g_phy_ip_subfix 254
#define g_stack_service_port 6089
#define g_phy_service_port 6089

extern const char *p_docker_command;
extern const char *p_net_command;
extern const char *gp_stop_docker;
extern const char *gp_rm_docker;
extern const char *gp_start_docker;

#define WAITING(ID)   do {                   \
        int w = 3000;                        \
        wait.register_id(ID);                \
        while (w > 0 && wait.is_blocked(ID)) \
        {                                    \
            w--;                             \
            usleep(1000);                    \
        }                                    \
        if (!wait.is_blocked(ID))            \
        {                                    \
            return 1;                        \
        }                                    \
        else                                 \
        {                                    \
            return -1;                       \
        } } while (0)
