#include "../hdr/init.h"
#include "../hdr/const.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string.h>
#include <sys/wait.h>
#include <vector>
using namespace std;
static init_entry_t init_apps[] = {
    // IOT
    { -1, 0, "", "enb1" },
    { -1, 0, "", "enb2" },
    // AN
    { -1, 0, "", "enb_AN_1" },
    { -1, 0, "", "enb_AN_2" },
    // CNW
    { -1, 0, "", "CNW" },

};

//--config_file=path/enb.conf
//#define VERSION_RELEASE
//#define CONF_TEST
int SetConfigFilePath(string path)
{
#ifdef VERSION_RELEASE
//#ifdef BUILD_WITH_CRAN_RELEASE
  string path_access = path + "/sdr_access";
  string path_iot = path + "/sdr_iot";
   std::cout<<" release  ============= path access  "<<path_access<<std::endl;
   std::cout<<" release ============= path iot  "<<path_iot<<std::endl;
#else
  string path_access = path;
  string path_iot = path;
   std::cout<<" debug  ============= path access  "<<path_access<<std::endl;
   std::cout<<" debug ============= path iot  "<<path_iot<<std::endl;
#endif
  init_apps[0].cmd = path_iot +"/srsenb/src/srsenb --config_file=" + path + "/IOT/beam1/enb.conf" +
                    " --cnw_config_file="+ path + "/IOT/beam1/cnw.conf" +
                    //" --udm.db_file="+ path + "/IOT/beam1/user_db.csv" +
                    " --rf.device_args=\"ksw_channel_choose=0\" ";
  init_apps[1].cmd = path_iot +"/srsenb/src/srsenb --config_file=" + path + "/IOT/beam2/enb.conf" +
                    " --cnw_config_file="+ path + "/IOT/beam2/cnw.conf" +
                    //" --udm.db_file="+ path + "/IOT/beam2/user_db.csv" +
                    " --rf.device_args=\"ksw_channel_choose=1\" ";
#ifndef CONF_TEST
  init_apps[2].cmd = path_access +"/srsenb/src/srsenb --config_file=" + path + "/access/beam1/enb1.conf" +
                    " --rf.device_args=\"ksw_channel_choose=0\" "; //+ "--enb.ttcn_time=120";
#else
  init_apps[2].cmd = "./srsenb/src/srsenb --config_file=/opt/AccessIot/access/beam1/enb1.conf --rf.device_args=\"ksw_channel_choose=0\" "; //+ "--enb.ttcn_time=120";
#endif

  init_apps[3].cmd = path_access +"/srsenb/src/srsenb --config_file=" + path + "/access/beam2/enb1.conf" +
                    " --rf.device_args=\"ksw_channel_choose=1\" ";//+ "--enb.ttcn_time=120";
#ifndef CONF_TEST                   
  init_apps[4].cmd =  path_access + "/srscnw/srs5gc --config_file=" + path + "/access/cnw/cnw.conf" +
                    " --user_db_file=" + path + "/access/cnw/user_db.csv" + " --log_filename=stdout";
#else
    init_apps[4].cmd =  "./srscnw/srs5gc --config_file=/opt/AccessIot/access/cnw/cnw.conf --user_db_file=/opt/AccessIot/access/cnw/user_db.csv --log_filename=stdout";
#endif
  std::cout<<std::endl;
  std::cout<<" ============= init apps iot  "<<init_apps[0].cmd<<std::endl<<std::endl;
  std::cout<<" ============= init apps access  "<<init_apps[2].cmd<<std::endl<<std::endl;
  std::cout<<" ============= init apps 5gc  "<<init_apps[4].cmd<<std::endl<<std::endl;
  
  // init_apps[0].cmd = "./srsenb/src/srsenb --config_file=" + path + "/IOT/beam1/enb.conf" +
  //                    " --rf.device_name=zmq    "
  //                    "--rf.device_args=\"fail_on_disconnect=true,tx_port=tcp://*:2000,rx_port=tcp://"
  //                    "localhost:2001,id=enb,base_srate=23.04e6\"";
  // init_apps[1].cmd = "./srsenb/src/srsenb --config_file=" + path + "/IOT/beam2/enb.conf" +
  //                    " --rf.device_name=zmq    "
  //                    "--rf.device_args=\"fail_on_disconnect=true,tx_port=tcp://*:3000,rx_port=tcp://"
  //                    "localhost:3001,id=enb,base_srate=23.04e6\"";
  // init_apps[2].cmd = "./srsenb/src/srsenb --config_file=" + path + "/access/beam1/enb1.conf" +
  //                    " --rf.device_name=zmq    "
  //                    "--rf.device_args=\"fail_on_disconnect=true,tx_port=tcp://*:4000,rx_port=tcp://"
  //                    "localhost:4001,id=enb,base_srate=23.04e6\"";
  // init_apps[3].cmd = "./srsenb/src/srsenb --config_file=" + path + "/access/beam2/enb1.conf" +
  //                    " --rf.device_name=zmq    "
  //                    "--rf.device_args=\"fail_on_disconnect=true,tx_port=tcp://*:5000,rx_port=tcp://"
  //                    "localhost:5001,id=enb,base_srate=23.04e6\"";
  return 0;
}

int start_app()
{
    for (auto i = 0u; i < sizeof(init_apps) / sizeof(init_entry_t); i++)
    {
        pid_t pid = spawn_process(init_apps[i].cmd.c_str(), init_apps[i].abbr.c_str());
        if (pid > 0)
        {
            init_apps[i].pid = pid;
            init_apps[i].status = 1;
        }
    }
    return 0;
}
static int wait_app(int ms)
{
    int      time = 0;
    uint16_t count = 0;
    while (count < sizeof(init_apps) / sizeof(init_entry_t) && time < ms)
    {
        for (auto i = 0u; i < sizeof(init_apps) / sizeof(init_entry_t); i++)
        {
            pid_t pid = init_apps[i].pid;
            if (pid > 0 && init_apps[i].status > 0)
            {
                int   status;
                pid_t result = waitpid(pid, &status, WNOHANG);
                if (WIFEXITED(status))
                {
                    init_apps[i].status = 0;
                    count++;
                    std::cout << "stopped the app =" << pid << std::endl;
                }
            }
        }
        usleep(1000);
        time++;
    }
    if (time > ms)
        return -1;
    return 0;
}
int stop_app()
{
    for (auto i = 0u; i < sizeof(init_apps) / sizeof(init_entry_t); i++)
    {
        pid_t pid = init_apps[i].pid;
        if (pid > 0)
        {
            stop_process(pid);
        }
    }
    wait_app(5000);
    return 0;
}
int waitpid(pid_t id, int ms)
{
    int      time = 0;
    uint16_t count = 0;
    while (count < 1 && time < ms)
    {
        pid_t pid = id;
        if (pid > 0)
        {
            int   status;
            pid_t result = waitpid(pid, &status, WNOHANG);
            if (WIFEXITED(status))
            {
                count++;
                std::cout << "stopped the app =" << pid << std::endl;
            }
        }
        usleep(1000);
        time++;
    }
    if (time > ms)
    {
        return -1;
    }

    return 0;
}
static int wait_an_app(int idx, int ms)
{
    int      time = 0;
    uint16_t count = 0;
    while (count < 1 && time < ms)
    {
        pid_t pid = init_apps[idx].pid;
        if (pid > 0 && init_apps[idx].status > 0)
        {
            int   status;
            pid_t result = waitpid(pid, &status, WNOHANG);
            if (WIFEXITED(status))
            {
                count++;
                std::cout << "stopped the app =" << pid << std::endl;
            }
        }
        usleep(1000);
        time++;
    }
    if (time > ms)
    {
        return -1;
    }

    return 0;
}
pid_t Get_beam_pid(int beam_idx)
{
    return init_apps[beam_idx].pid;
}
int start_an_app(int idx)
{
    uint32_t num = sizeof(init_apps) / sizeof(init_entry_t);
    // assert(idx < num);
    if (init_apps[idx].status <= 0)
    {
        std::cout << "start an app" << init_apps[idx].cmd.data() << std::endl;
        pid_t pid = spawn_process(init_apps[idx].cmd.c_str(), init_apps[idx].abbr.c_str());
        std::cout << "end an app" << pid << std::endl;
        if (pid > 0)
        {
            init_apps[idx].pid = pid;
            init_apps[idx].status = 1;
        }
    }
    return 0;
}
int stop_an_app(int idx)
{
    uint32_t num = sizeof(init_apps) / sizeof(init_entry_t);
    // assert(idx < num);

    pid_t pid = init_apps[idx].pid;
    if (pid > 0 && init_apps[idx].status > 0)
    {
        stop_process(pid);
        wait_an_app(idx, 5000);
        init_apps[idx].status = 0;
        init_apps[idx].pid = -1;
    }
    return 0;
}
int get_an_app_status(int idx)
{
    uint32_t num = sizeof(init_apps) / sizeof(init_entry_t);
    // assert(idx < num);

    return init_apps[idx].status;
}
static int Process_Config_file(fstream&    fs,
                               const char* filepath,
                               const char* WordSegment,
                               const char* SegSubParaName,
                               const char* ParaValue)
{
    std::string           line{};
    std::string           tmp_Ws{ WordSegment };
    int                   findWordflag = 0;
    std::string           searchStr = std::string(SegSubParaName);
    std::string           replaceStr = std::string(SegSubParaName) + " = " + std::string(ParaValue);
    std::vector< string > vs{};
    while (getline(fs, line))
    {
        if (line == tmp_Ws)
        {
            findWordflag = 1;
            break;
        }
        vs.push_back(line);
    }
    if (findWordflag == 1)
    {
        while (std::getline(fs, line))
        {
            if (line.find(searchStr) != std::string::npos)
            {
                line = replaceStr;
            }
            vs.push_back(line);
        }
        fs.close();
    }
    else
    {
        std::cout << "find word Segment fail" << std::endl;
        return -1;
    }
    fs.open(filepath, std::ios::out | ios::trunc);
    for (const auto& c : vs)
    {
        fs << c << endl;
    }
    fs.close();
    return 0;
}
int Handle_Enb_Config_Modify(char* filepath, char* WordSegment, char* SegSubParaName, char* ParaValue)
{
    int     ret = 0;
    fstream fs(filepath, std::ios::in | std::ios::out);
    if (fs.is_open())
    {
        ret = Process_Config_file(fs, filepath, WordSegment, SegSubParaName, ParaValue);
    }
    else
    {
        std::cout << "file open fail:" << filepath << std::endl;
        return -1;
    }
    return ret;
}