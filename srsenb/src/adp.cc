#include "srsenb/hdr/adp.h"

#include "srsenb/hdr/udp.h"
#include <iostream>
#include <fstream>
#include <unordered_map>

namespace srsenb
{
  extern adp adp_;
  // extern adp adp_;

  adp::adp() : thread("ADP"), running(false)
  { /* Do nothing */
  }

  bool adp::init(all_args_t *args_)
  {
    std::cout << "------adp init succeed------" << std::endl;
    running = true;
    args = args_;
   
   //-------------------0929------------------
   #ifdef OPEN
    std::cout << " args->enb_files.mib_config = " << args->enb_files.mib_config << std::endl;
    if (!read_mib_config(args->enb_files.mib_config))
    {
      std::cerr << "[ADP][READ]read mib config error!" << std::endl;
    }
    #endif
    //----------------------------------------

    if (!udp_.init(args))
    {
      std::cout << "-----udp init Faiure-----" << std::endl;
    }

    start();
    return true;
  }

  void adp::stop()
  {
    if (running)
    {
      running = false;
      wait_thread_finish();
    }
  }

  void adp::run_thread()
  {
    while (running)
    {
      udp_.run_udp();
    }
  }

  bool adp::read_mib_config(const std::string &filename)
  {
    std::cout << "[ADP][READ][FILENAME]:" << filename << std::endl;
    std::ifstream file(filename);
    std::string line;
    // area_mode == 1 扩频模式

    int access_pid = 0;
    int access_port1 = 0;
    int access_port2 = 0;

    std::unordered_map<std::string, std::function<void(const std::string &)>> assigners = {
        {"access_pid", [&](const std::string &value)
         { access_pid = std::stoi(value); }},
        {"access_port1", [&](const std::string &value)
         { access_port1 = std::stoi(value); }},
        {"access_port2", [&](const std::string &value)
         { access_port2 = std::stoi(value); }}};

    if (file.is_open())
    {
      while (std::getline(file, line))
      {
        // Remove whitespace from the line
        line.erase(remove_if(line.begin(), line.end(), isspace), line.end());

        // Find the position of '=' and ';'
        size_t pos_equal = line.find("=");
        size_t pos_semicolon = line.find(";");

        if (pos_equal != std::string::npos)
        {
          // Extract the key and value
          std::string key = line.substr(0, pos_equal);
          std::string value = line.substr(pos_equal + 1, pos_semicolon - pos_equal - 1);

          // Assign the value if the key exists in the map
          if (assigners.find(key) != assigners.end())
          {
            assigners[key](value);
          }
        }
      }
      file.close();

      std::cout << "access_pid = " << access_pid << std::endl;
      std::cout << "access_port1 = " << access_port1 << std::endl;
      std::cout << "access_port2 = " << access_port2 << std::endl;
      //args->pid=access_pid;
      
      if (!udp_.trans_mib_config(access_pid, access_port1, access_port2))
      {
        std::cerr << "[ADP][READ] trans mib config error!" << std::endl;
      }

      return true;
    }
    else
    {
      std::cerr << "Unable to open file" << std::endl;
      return false;
    }
  }

} // namespace srsenb
