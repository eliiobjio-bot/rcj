#pragma once
#include <iostream>
#include <assert.h>
#include <arpa/inet.h>
#include <map>
#include <list>
#include <string>
#include <memory>
#include "srsran/net/cy_udpsocket.h"


std::string exec(const char *cmd);

pid_t spawn_process(const char *cmd, const char *abbr);
int stop_process(pid_t pid);

class waiting
{
protected:
    std::map<uint32_t, bool> id_map;

public:
    waiting() = default;
    void register_id(uint32_t id)
    {
        if (id_map.find(id)== id_map.end())
        {
            id_map[id] = true;
        }
    };
    void unregister(uint32_t id)
    {
        id_map.erase(id);
    }
    bool is_blocked(uint32_t id)
    {
        if(id_map.find(id) != id_map.end())
        {
            return id_map[id];
        }
        return false;
    }

};

class ifbase
{
protected:
    static waiting wait;
public:
    ifbase(){};
    void unregister(uint32_t id){wait.unregister(id);};
};

class ifadmin : public ifbase
{
protected:
  
    std::shared_ptr<UDPSocket<63000>> udp_socket;
    uint16_t padding = 0;
    std::string ip_addr;
    uint16_t port;
public:
    ifadmin(){
        udp_socket = std::make_shared<UDPSocket<63000>>(true);
        padding = ntohs(0xEDED);
    }
    void init(const std::string &&ip, uint16_t p)
    {
        ip_addr = std::move(ip);
        port = p;
        udp_socket->Connect(ip_addr, port);
    };

    virtual int send_ack(const uint8_t *pdata, uint32_t len)
    {
        udp_socket->SendTo((char *)pdata, ip_addr, port);
        return 0;
    }
    virtual int send_nack(const uint8_t *pdata, uint32_t len)
    {
        udp_socket->SendTo((char *)pdata, ip_addr, port);
        return 0;
    }
    ~ifadmin()
    {
        if (udp_socket)
            udp_socket->Close();
    }
    virtual int process_data(const uint8_t *pdata, uint32_t len) = 0;
};

