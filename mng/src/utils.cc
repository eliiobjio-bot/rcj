#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <array>
#include <cstring>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <unistd.h>
#include <chrono>
#include <random>
#include "../hdr/const.h"
#include "../hdr/interface.h"


std::string exec(const char *cmd)
{
    std::array<char, 512> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe)
    {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
    {
        result += buffer.data();
    }
    return result;
}
const char *turn_ip_addr_to_str(uint32_t in)
{
    struct in_addr in_add;
    in_add.s_addr = htonl(in);

    return inet_ntoa(in_add);
}
uint32_t turn_ip_addr_to_uint32(const char *str_ip)
{
    return ntohl(inet_addr(str_ip));
}
void replace_dot_char(char *str)
{
    int len = strlen(str);
    for (int i = 0; i < len; i++)
    {
        if (str[i] == '.')
        {
            str[i] = '_';
        }
    }
    return;
}
void replace_dash_char(char *str)
{
    int len = strlen(str);
    for (int i = 0; i < len; i++)
    {
        if (str[i] == '_')
        {
            str[i] = '.';
        }
    }
    return;
}


int stop_process(pid_t pid)
{
    char buf[256] = {0};
    snprintf(buf, 256, "ps -p %u", pid);
    if (system(buf) == 0)  // ?
    {
        kill(pid, SIGINT);
        usleep(1000);
        kill(pid, SIGTERM);
        usleep(1000);
        kill(pid, SIGKILL);
        
        return 0;
    }
    else
    {
        return -1;
    }
}

pid_t spawn_process(const char *cmd, const char *abbr)
{
    pid_t pid;
    int i;
    char *cmdpath, *s;
    char *args[255];
    char pidfile[256];
    const char *envp[] = {
        "TERM=vt100",
        "PATH=/bin:/sbin:/usr/local/bin",
        NULL};
    if (!cmd || !abbr)
    {
        return -2;
    }
    pid = fork();
    if (pid < 0)
    {
        return -1;
    }
    else if (pid == 0)
    { // child

        for (i = 0; i < 32; i++)
            signal(i, SIG_DFL);

        char *dupcmd = strdup(cmd);
        if (dupcmd == NULL)
        {
            return -1;
        }

        for (s = (char *)dupcmd, i = 0; (s = strsep((char **)&dupcmd, " \t")) != NULL;)
        {
            if (*s != '\0')
            {
                args[i] = s;
                s++;
                i++;
            }
        }
        args[i] = NULL;
        cmdpath = args[0];
        prctl(PR_SET_NAME, "mng_daemon");  
        if(-1 ==execve(cmdpath, args, const_cast<char* const*>(envp)))
        {
            perror("execve ");  
        }
        
        std::cout << "exec cmdpath:" << cmdpath << std::endl;
        std::cout << "exec:" << args[0] << std::endl;
        //exit(0);
    }
    else
    {
        std::cout << "sub id:" << pid << std::endl;
        return pid;
    }
    /* get pid from pid file？ */
    return -1;
}

int gen_random(int low, int high)
{
    std::random_device rd;  
    std::mt19937 gen(rd());  
    std::uniform_int_distribution<> distrib(low, high);  
  
    
    return distrib(gen);  
  
}
// void fill_pdxp_header(pdxp_pkt_header &pkt_header, uint32_t &bid)
// {
//     auto now = std::chrono::system_clock::now();
    
//     auto duration = now.time_since_epoch();  
//     auto ticks = std::chrono::duration_cast<std::chrono::microseconds>(duration);  
//     auto count = ticks.count();

//     uint32_t date =(uint32_t) (count/(24*3600*1000));
//     uint32_t time =(uint32_t) (count%(24*3600*1000));
    

//     pkt_header.ver = 0x10;
//     pkt_header.mid = htons(0x80);
//     pkt_header.sid = htonl(COMMAND_SOURCE_CONTROL);
//     pkt_header.did = 0;  // 没多大意义
//     pkt_header.bid =  htonl(time+ gen_random(1,1000));  // 数据标志 may be bug?
//     pkt_header.seq = 0;
//     pkt_header.flag = 0; // need filled
//     //rsv
//     bid = time;
//     pkt_header.send_date = htonl(date);
//     pkt_header.send_time = htonl(time);
// }
