
#include <boost/program_options.hpp>
#include <boost/program_options/parsers.hpp>
#include <fstream>
#include <iostream>
#include <poll.h>
#include <signal.h>
#include <string>

#include "../hdr/const.h"
#include "../hdr/control.h"
#include "../hdr/init.h"
#include "../hdr/mng_scpi_server.h"
#include "srsran/common/string_helpers.h"
#include "srsran/common/thread_pool.h"
#include "srsran/net/cy_udpserver.h"
#include "srsran/server/cy_server.h"
#include "srsran/srslog/srslog.h"
#include "srsran/srsran.h"

static bool running = false;
// static void signal_handler()
// {
//     running = false;
// }
namespace bpo = boost::program_options;
using namespace std;
using mng_signal_hanlder = void (*)(int);
string     config_file;
static int parse_args(all_args_t* args, int argc, char* argv[])
{
  bool use_standard_lte_rates = false;

  // Command line only options
  bpo::options_description general("General options");

  general.add_options()("help,h", "Produce help message")("version,v", "Print version information and exit");

  bpo::options_description common("Configuration options");

  common.add_options()("log.level", bpo::value<std::string>(&args->log.level)->default_value("debug"), "Set log level")(
      "log.file", bpo::value<std::string>(&args->log.file)->default_value("/tmp/mng.log"), "Log file")(
      "log.ip", bpo::value<std::string>(&args->log.ip)->default_value("127.0.0.1"), "Log remote ip")(
      "log.port", bpo::value<uint16_t>(&args->log.port)->default_value(8000), "Log remote port")(
      "log.file_max_size", bpo::value<uint32_t>(&args->log.file_max_size)->default_value(10000), "Log file max size")(
      "mng_files.enb_conf_path",
      bpo::value<string>(&args->conf.conf_path)->default_value("/home/ei41/work/src/xw_code/xwcom/srsRAN-IoT.v1/build"),
      "config file path");

  bpo::positional_options_description p;
  bpo::options_description            position("Positional options");
  position.add_options()(
      "config_file", bpo::value<string>(&config_file)->default_value("mng.conf"), "mng configuration file");

  p.add("config_file", -1);

  // these options are allowed on the command line
  bpo::options_description cmdline_options;

  cmdline_options.add(common).add(position);
  bpo::variables_map vm;
  try {
    bpo::store(bpo::command_line_parser(argc, argv).options(cmdline_options).positional(p).run(), vm);
    bpo::notify(vm);
  } catch (bpo::error& e) {
    std::cerr << e.what() << endl;
    return -1;
  }
  ifstream conf(config_file.c_str(), ios::in);
  if (conf.fail()) {
    cout << "Failed to read enb configuration file " << config_file << " - exiting" << std::endl;
    exit(1);
  }
  try {
    bpo::store(bpo::parse_config_file(conf, common), vm);
    bpo::notify(vm);
  } catch (const boost::program_options::error& e) {
    cerr << e.what() << endl;
    exit(1);
  }
  // help option was given - print usage and exit
  if (vm.count("help")) {
    cout << "Usage: " << argv[0] << " [OPTIONS] config_file" << endl;
    exit(-1);
  }
  return 0;
}

// main must run in sudo mode
static void signal_handler(int signal)
{
  switch (signal) {
    case SIGALRM:
      fprintf(stderr, "Couldn't stop gracefully. Forcing exit.\n");
      raise(SIGKILL);
      break;
    default:
      stop_app();
      break;
  }
  running = false;
}
void mng_register_signal_handler(mng_signal_hanlder handler)
{
  signal(SIGINT, handler);
  signal(SIGTERM, handler);
  signal(SIGHUP, handler);
  signal(SIGALRM, handler);
  return;
}
static void execute_cmd(const string& cmd_line)
{
  vector<string> cmd;
  srsran::string_parse_list(cmd_line, ' ', cmd);
  if (cmd[0] == "q") {
    raise(SIGTERM);
  } else if (cmd[0] == "flush") {
    if (cmd.size() != 1) {
      cout << "Usage: " << cmd[0] << endl;
      return;
    }
    srslog::flush();
    cout << "Flushed log file buffers" << endl;
  } else {
    cout << "          q: quit srsenb" << endl;
  }
}
static void* input_loop()
{
  struct pollfd pfd = {STDIN_FILENO, POLLIN, 0};
  string        input_line;
  while (running) {
    int ret = poll(&pfd, 1, 1000); // query stdin with a timeout of 1000ms
    if (ret == 1) {
      // there is user input to read
      getline(cin, input_line);
      if (cin.eof() || cin.bad()) {
        cout << "Closing stdin thread." << endl;
        break;
      } else if (not input_line.empty()) {
        list<string> cmd_list;
        srsran::string_parse_list(input_line, ';', cmd_list);

        for (const string& cmd : cmd_list) {
          execute_cmd(cmd);
        }
      }
    }
  }
  return nullptr;
}

int main(int argc, char* argv[])
{
  all_args_t args = {};
  int        eno  = parse_args(&args, argc, argv);
  if (eno != 0) {
    return eno;
  }

  // int node = 1; // should read from config;
  mng_register_signal_handler(signal_handler);
  auto log_sink = (args.log.file == "stdout") ? srslog::create_stdout_sink()
                                              : srslog::create_file_sink(args.log.file, args.log.file_max_size * 1024u);
  if (!log_sink) {
    return -1;
  }
  srslog::set_default_sink(*log_sink);

  // start_app();
    if (CYSCPI_CHANNEL::mp_scpi == nullptr) {
    CYSCPI_CHANNEL::mp_scpi = std::make_shared<CYSATScpiParse>(args);
  }
  std::shared_ptr<SCPI_SERVER> p_scpi_server = std::make_shared<SCPI_SERVER>(srslog::get_default_sink());
  p_scpi_server->init();
  
  std::thread input(&input_loop);
  running = true;
  while (running) {
    sleep(1);
  }
  input.join();
  // sleep(1);
  return 0;
}