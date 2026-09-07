/**
 * Copyright 2013-2021 Software Radio Systems Limited
 *
 * This file is part of srsRAN.
 *
 * srsRAN is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * srsRAN is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * A copy of the GNU Affero General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 *
 */

#include "srscnw/hdr/cnw.h"
#include "srscnw/hdr/adp.h"
#include "srsran/build_info.h"
#include "srsran/common/bcd_helpers.h"
#include "srsran/common/common_helper.h"
#include "srsran/common/config_file.h"
#include "srsran/common/crash_handler.h"
#include "srsran/srslog/srslog.h"
#include "srsran/srsran.h"
#include "srsran/support/emergency_handlers.h"
#include "srsran/support/signal_handler.h"
#include <boost/program_options.hpp>
#include <iostream>
#include <unordered_map>

#include "srsran/common/test_common.h"

using namespace std;
using namespace srsepc;
namespace bpo = boost::program_options;

typedef struct {
  std::string nas_mm_level;
  int         nas_mm_hex_limit;
  std::string nas_sm_level;
  int         nas_sm_hex_limit;
  std::string ims_level;
  int         ims_hex_limit;
  std::string other_level;
  int         other_hex_limit;

  std::string all_level;
  int         all_hex_limit;
  std::string filename;
} log_args_t;

typedef struct {
  log_args_t log_args;
  cnw_args_t cnw_args;
} all_args_t;

static srslog::sink*     log_sink = nullptr;
static std::atomic<bool> running  = {true};


bool read_5GC_conf_file(all_args_t* args)
{

  char cwd[256];
  if (getcwd(cwd, sizeof(cwd)) != nullptr)
  { // 检查返回值是否为nullptr
    std::cout << "Current working directory: " << cwd << std::endl;
  }
  else
  {
    std::cerr << "Failed to get current working directory" << std::endl;
  }

  std::ifstream m_conf_file;

  m_conf_file.open(args->cnw_args.config_file.c_str(), std::ifstream::in);
  if (!m_conf_file.is_open())
  {
    return false;
  }
  srsran::console("Opened conf file: %s\n", args->cnw_args.config_file.c_str());

  std::unordered_map<std::string, std::string> configValues;

  std::string line;
  while (std::getline(m_conf_file, line))
  {
    // Check if the line is a comment or blank
    if (line.empty() || line[0] == '#' || std::all_of(line.begin(), line.end(), ::isspace))
      continue;

    // Find the position of the first '=' character on the line
    size_t pos = line.find('=');
    if (pos != std::string::npos)
    {
      std::string key = line.substr(0, pos);
      std::string value = line.substr(pos + 1);
      // trim leading and trailing whitespaces from key and value
      key.erase(key.find_last_not_of(" \t\n\r") + 1);
      value.erase(0, value.find_first_not_of(" \t\n\r"));
      configValues[key] = value;
    }
  }

  m_conf_file.close();

  // Output the config key-value pairs
  for (const auto &pair : configValues)
  {
    std::cout << pair.first << " : " << pair.second << std::endl;
    if (pair.first == "ue_data_ip_addr")
    {
      args->cnw_args.set_ue_ip_adrr = pair.second;
    }
    if (pair.first == "nas_integrity_algo")
    {
      if (pair.second == "EIA1")
      {
        args->cnw_args.integrity_algo = srsran::INTEGRITY_ALGORITHM_ID_128_EIA1;
      }
      else if (pair.second == "EIA2")
      {
        args->cnw_args.integrity_algo = srsran::INTEGRITY_ALGORITHM_ID_128_EIA2;
      }
      else if (pair.second == "EIA3")
      {
        args->cnw_args.integrity_algo = srsran::INTEGRITY_ALGORITHM_ID_128_EIA3;
      }
      else
      {
        args->cnw_args.integrity_algo = srsran::INTEGRITY_ALGORITHM_ID_128_EIA1;
      }
    }

    if (pair.first == "all_level")
    {
      args->log_args.all_level = pair.second;
    }
    if (pair.first == "all_hex_limit")
    {
      args->log_args.all_level = pair.second;
    }
    if (pair.first == "log_filename")
    {
      args->log_args.filename = pair.second;
    }

  }

  return true;
}

/**********************************************************************
 *  Program arguments processing
 ***********************************************************************/
string config_file;

void parse_args(all_args_t* args, int argc, char* argv[])
{
  string   mme_name;
  string   mme_code;
  string   mme_group;
  string   amf_rgion_id;
  string   amf_set_id;
  string   amf_pointer;  
  string   tac;
  string   mcc;
  string   mnc;
  string   mme_bind_addr;
  string   mme_apn;
  string   encryption_algo;
  string   integrity_algo;
  uint16_t paging_timer     = 0;
  uint32_t max_paging_queue = 0;
  string   spgw_bind_addr;
  string   sgi_if_addr;
  string   sgi_if_name;
  string   dns_addr;
  string   full_net_name;
  string   short_net_name;
  bool     request_imeisv;
  string   hss_db_file;
  string   hss_auth_algo;
  string   log_filename;

  // Command line only options
  bpo::options_description general("General options");
  // clang-format off
  general.add_options()
      ("help,h", "Produce help message")
      ("version,v", "Print version information and exit")
      ;

  // Command line or config file options
  bpo::options_description common("Configuration options");
  common.add_options()

    //AMF
    ("amf.amf_rgion_id",        bpo::value<string>(&amf_rgion_id)->default_value("0xfe"),            "AMF Region id")
    ("amf.amf_set_id",            bpo::value<string>(&amf_set_id)->default_value("0x0001"),        "AMF id")
    ("amf.amf_pointer",       bpo::value<string>(&amf_pointer)->default_value("0x01"),           "AMF Pointer")
    ("amf.tac",             bpo::value<string>(&tac)->default_value("0x0007"),                  "Tracking Area Code")
    ("amf.mcc",             bpo::value<string>(&mcc)->default_value("460"),                  "Mobile Country Code")
    ("amf.mnc",             bpo::value<string>(&mnc)->default_value("00"),                   "Mobile Network Code")
    ("amf.nas_encryption_algo", bpo::value<string>(&encryption_algo)->default_value("EEA0"),         "Set preferred encryption algorithm for NAS layer")
    ("amf.nas_integrity_algo", bpo::value<string>(&integrity_algo)->default_value("EIA2"),     "Set preferred integrity protection algorithm for NAS ")

    //IP ADRR
    ("ip_addr.cnw_ip_adrr", bpo::value<string>(&args->cnw_args.cnw_ip_adrr)->default_value("127.0.0.1"),        "Bind port for MAC network trace")
    ("ip_addr.cnw_port", bpo::value<uint16_t>(&args->cnw_args.cnw_port)->default_value(5004),     "Client IP address for MAC network trace")
    ("ip_addr.enb1_ip_adrr", bpo::value<string>(&args->cnw_args.enb1_ip_adrr)->default_value("127.0.0.1"),        "Bind port for MAC network trace")
    ("ip_addr.enb1_port", bpo::value<uint16_t>(&args->cnw_args.enb1_port)->default_value(5001),     "Client IP address for MAC network trace")
    ("ip_addr.enb2_ip_adrr", bpo::value<string>(&args->cnw_args.enb2_ip_adrr)->default_value("127.0.0.1"),        "Bind port for MAC network trace")
    ("ip_addr.enb2_port", bpo::value<uint16_t>(&args->cnw_args.enb2_port)->default_value(6001),     "Client IP address for MAC network trace")
    ("ip_addr.ate1_ip_adrr", bpo::value<string>(&args->cnw_args.ate1_ip_adrr)->default_value("192.168.10.253"),        "Bind port for MAC network trace")
    ("ip_addr.ate1_port", bpo::value<uint16_t>(&args->cnw_args.ate1_port)->default_value(9038),     "Client IP address for MAC network trace")
    ("ip_addr.ate2_ip_adrr", bpo::value<string>(&args->cnw_args.ate2_ip_adrr)->default_value("192.168.10.253"),        "Bind port for MAC network trace")
    ("ip_addr.ate2_port", bpo::value<uint16_t>(&args->cnw_args.ate2_port)->default_value(9058),     "Client IP address for MAC network trace")


    //UPF
    ("upf.ue_ip_addr", bpo::value<string>(&args->cnw_args.set_ue_ip_adrr)->default_value("1.127.11.10"), "IP address of SP-GW for the S1-U connection")
    ("upf.sgi_if_addr",    bpo::value<string>(&args->cnw_args.sgi_if_addr)->default_value("1.127.11.1"),   "IP address of TUN interface for the SGi connection")
    ("upf.sgi_if_name",    bpo::value<string>(&args->cnw_args.sgi_if_name)->default_value("xw_sgi"), "Name of TUN interface for the SGi connection")
    ("upf.max_paging_queue", bpo::value<uint32_t>(&args->cnw_args.max_paging_queue)->default_value(100), "Max number of packets in paging queue")


    // PCAP
    ("pcap.nas_net_enable", bpo::value<bool>(&args->cnw_args.pcap_net.enable)->default_value(false),         "Enable MAC network captures")
    ("pcap.bind_ip", bpo::value<string>(&args->cnw_args.pcap_net.bind_ip)->default_value("0.0.0.0"),         "Bind IP address for MAC network trace")
    ("pcap.bind_port", bpo::value<uint16_t>(&args->cnw_args.pcap_net.bind_port)->default_value(5687),        "Bind port for MAC network trace")
    ("pcap.client_ip", bpo::value<string>(&args->cnw_args.pcap_net.client_ip)->default_value("127.0.0.1"),     "Client IP address for MAC network trace")
    ("pcap.client_port", bpo::value<uint16_t>(&args->cnw_args.pcap_net.client_port)->default_value(5847),    "Enable MAC network captures")

    //Log
    ("log.all_level", bpo::value<string>(&args->log_args.all_level)->default_value("info"),     "Client IP address for MAC network trace")
    ("log.all_hex_limit", bpo::value<int>(&args->log_args.all_hex_limit)->default_value(32),    "Enable MAC network captures")
    ("log.filename", bpo::value<string>(&args->log_args.filename)->default_value("stdout"),    "Enable MAC network captures")

  ;

  // Positional options - config file location
  bpo::options_description position("Positional options");
  position.add_options()
    ("config_file", bpo::value<string>(&args->cnw_args.config_file), "MME configuration file")
    ("user_db_file", bpo::value<string>(&args->cnw_args.user_db_file), "user_db file")
    ("log_filename", bpo::value<string>(&args->log_args.filename), "log_filename")
  ;

  // clang-format on

  bpo::positional_options_description p;
  p.add("config_file", -1);

  // these options are allowed on the command line
  bpo::options_description cmdline_options;
  cmdline_options.add(common).add(position).add(general);

  // parse the command line and store result in vm
  bpo::variables_map vm;
  try {
    bpo::store(bpo::command_line_parser(argc, argv).options(cmdline_options).positional(p).run(), vm);
    bpo::notify(vm);
  } catch (bpo::error& e) {
    cerr << e.what() << endl;
    exit(1);
  }

  // help option was given - print usage and exit
  if (vm.count("help")) {
    cout << "Usage: " << argv[0] << " [OPTIONS] config_file" << endl << endl;
    cout << common << endl << general << endl;
    exit(0);
  }

   // print version number and exit
  if (vm.count("version"))
  {
    cout << "Version " << srsran_get_version_major() << "." << srsran_get_version_minor() << "."
         << srsran_get_version_patch() << endl;
    exit(0);
  }

  // if no config file given, check users home path
  if (!vm.count("config_file"))
  {
    if (!config_exists(args->cnw_args.config_file, "cnw.conf"))
    {
      cout << "Failed to read cnw configuration file " << args->cnw_args.config_file << " - exiting" << endl;
      exit(1);
    }
  }

  cout << "Reading configuration file " << args->cnw_args.config_file << "..." << endl;
  ifstream conf(args->cnw_args.config_file.c_str(), ios::in);
  if (conf.fail())
  {
    cout << "Failed to read configuration file " << args->cnw_args.config_file << " - exiting" << endl;
    exit(1);
  }

  // parse config file and handle errors gracefully
  try
  {
    bpo::store(bpo::parse_config_file(conf, common), vm);
    bpo::notify(vm);
  }
  catch (const boost::program_options::error &e)
  {
    cerr << e.what() << endl;
    exit(1);
  }

  // read_5GC_conf_file(args);

  if (encryption_algo == "EEA0") {
    args->cnw_args.encryption_algo = srsran::CIPHERING_ALGORITHM_ID_EEA0;
  } else if (encryption_algo == "EEA1") {
    args->cnw_args.encryption_algo = srsran::CIPHERING_ALGORITHM_ID_128_EEA1;
  } else if (encryption_algo == "EEA2") {
    args->cnw_args.encryption_algo = srsran::CIPHERING_ALGORITHM_ID_128_EEA2;
  } else if (encryption_algo == "EEA3") {
    args->cnw_args.encryption_algo = srsran::CIPHERING_ALGORITHM_ID_128_EEA3;
  } else {
    args->cnw_args.encryption_algo = srsran::CIPHERING_ALGORITHM_ID_EEA0;
    cout << "Error parsing amf.encryption_algo:" << encryption_algo << " - must be EEA0, EEA1, EEA2 or EEA3." << endl;
    cout << "Using default amf.encryption_algo: EEA0" << endl;
  }

  if (integrity_algo == "EIA0") {
    args->cnw_args.integrity_algo = srsran::INTEGRITY_ALGORITHM_ID_EIA0;
    cout << "Warning parsing amf.integrity_algo:" << encryption_algo
         << " - EIA0 will not supported by UEs use EIA1 or EIA2" << endl;
  } else if (integrity_algo == "EIA1") {
    args->cnw_args.integrity_algo = srsran::INTEGRITY_ALGORITHM_ID_128_EIA1;
  } else if (integrity_algo == "EIA2") {
    args->cnw_args.integrity_algo = srsran::INTEGRITY_ALGORITHM_ID_128_EIA2;
  } else if (integrity_algo == "EIA3") {
    args->cnw_args.integrity_algo = srsran::INTEGRITY_ALGORITHM_ID_128_EIA3;
  } else {
    args->cnw_args.integrity_algo = srsran::INTEGRITY_ALGORITHM_ID_128_EIA1;
    cout << "Error parsing amf.integrity_algo:" << encryption_algo << " - must be EIA0, EIA1, EIA2 or EIA3." << endl;
    cout << "Using default amf.integrity_algo: EIA1" << endl;
  }
  cout << "encryption_algo: " << encryption_algo << "  integrity_algo: " << integrity_algo << endl;
  cout << "encryption_algo: " << args->cnw_args.encryption_algo << "  integrity_algo: " << args->cnw_args.integrity_algo << endl;

  // Apply all_level to any unset layers

  args->log_args.nas_mm_level = args->log_args.all_level;
  args->log_args.nas_sm_level = args->log_args.all_level;
  args->log_args.ims_level = args->log_args.all_level;
  args->log_args.other_level = args->log_args.all_level;
  
  // Apply all_hex_limit to any unset layers

  args->log_args.nas_mm_hex_limit = args->log_args.all_hex_limit;
  args->log_args.nas_sm_hex_limit = args->log_args.all_hex_limit;
  args->log_args.ims_hex_limit = args->log_args.all_hex_limit;
  args->log_args.other_hex_limit = args->log_args.all_hex_limit;

  return;
}

std::string get_build_mode()
{
  return std::string(srsran_get_build_mode());
}

std::string get_build_info()
{
  if (std::string(srsran_get_build_info()).find("  ") != std::string::npos) {
    return std::string(srsran_get_version());
  }
  return std::string(srsran_get_build_info());
}

std::string get_build_string()
{
  std::stringstream ss;
  ss << "Built in " << get_build_mode() << " mode using " << get_build_info() << "." << std::endl;
  return ss.str();
}

static void emergency_cleanup_handler(void* data)
{
  srslog::flush();
  if (log_sink) {
    log_sink->flush();
  }
}

static void signal_handler()
{
  running = false;
}


int main(int argc, char* argv[])
{
    printf("cnw main is running!\n");

    srsran_register_signal_handler(signal_handler);
    add_emergency_cleanup_handler(emergency_cleanup_handler, nullptr);

    // print build info
    cout << endl << get_build_string() << endl;
    cout << endl << "---  Software Radio Systems CNW  ---" << endl << endl;

    all_args_t args;
    parse_args(&args, argc, argv);

    // Setup logging.
    log_sink = (args.log_args.filename == "stdout") ? srslog::create_stdout_sink()
                                                    : srslog::create_file_sink(args.log_args.filename);
    if (!log_sink) {
      return SRSRAN_ERROR;
    }
    srslog::log_channel* chan = srslog::create_log_channel("main_channel", *log_sink);
    if (!chan) {
      return SRSRAN_ERROR;
    }
    srslog::set_default_sink(*log_sink);

    // Start the log backend.
    srslog::init();

    if (args.log_args.filename != "stdout") {
      auto& cnw_logger = srslog::fetch_basic_logger("CNW", false);
      cnw_logger.info("\n\n%s\n---  Software Radio Systems CNW log ---\n\n", get_build_string().c_str());
    }

    srsran::log_args(argc, argv, "CNW");


    srslog::basic_logger& nas_mm_logger = srslog::fetch_basic_logger("NAS_MM", false);
    nas_mm_logger.set_level(srslog::str_to_basic_level(args.log_args.nas_mm_level));
    nas_mm_logger.set_hex_dump_max_size(args.log_args.nas_mm_hex_limit);

    srslog::basic_logger& nas_sm_logger = srslog::fetch_basic_logger("NAS_SM", false);
    nas_sm_logger.set_level(srslog::str_to_basic_level(args.log_args.nas_sm_level));
    nas_sm_logger.set_hex_dump_max_size(args.log_args.nas_sm_hex_limit);

    srslog::basic_logger& ims_logger = srslog::fetch_basic_logger("IMS", false);
    ims_logger.set_level(srslog::str_to_basic_level(args.log_args.ims_level));
    ims_logger.set_hex_dump_max_size(args.log_args.ims_hex_limit);

    // adp m_adp;
    // m_adp.init(args.cnw_args);

    cnw* cnw = cnw::get_instance();

    // if (cnw->init(args.cnw_args, &m_adp)) {
    //   cout << "Error initializing CNW" << endl;
    //   exit(1);
    // }

    if (cnw->init(args.cnw_args)) {
      cout << "Error initializing CNW" << endl;
      exit(1);
    }

    cnw->start();

    while (running) {
      sleep(1);
    }
    
    // m_adp.stop();
    cnw->stop();
    cnw->m_nas_mm->cleanup();
    cnw->m_nas_sm->cleanup();
    cnw->m_pcs_ims->cleanup();
    cnw->m_xw_icmp->xw_tun_->cleanup();
    cnw->m_xw_icmp->cleanup();
    cnw->cleanup();

    cout << std::endl << "---  exiting  ---" << endl;
  return 0;
}
