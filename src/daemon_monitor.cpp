/**
* DaemonMonitor		Provides information about CPU, IO, Networks and GPUs
+ stats in a node.
* @version              v1.1
*/


#ifndef DAEMON_INFO
#define DAEMON_INFO

#include <string>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <sys/time.h>
#include <filesystem>
#include <chrono>
#include "system_features.hpp"
#include "daemon_monitor.hpp"
#include "net_info.hpp"
#include "devices_info.hpp"
#include "cpu_info.hpp"
#include "power_cpu_info.hpp"
#include "memory_info.hpp"
#include "Packed_sample.hpp"
#include "cliente_monitor.hpp"

#include <hiredis/hiredis.h>

using namespace std;

/**************************************************************************************************************/

Hw_conf hw_features;
int mode = -1;
Packed_sample *ps;
Packed_sample _last_ps;
int server_socket;
int threshold = 0;
int opt, port = 0;
char *server = NULL, *server2 = NULL, *server3 = NULL, *es_addr;
int error = 0;
int heartbit = 0;
int tmrd = 0;
int top_relation = 0, top_counter = 0;
int net_reducer;
int only_hotspots = 0;
int th_cpu = 0, th_mem = 0, th_io = 0, th_net = 0, th_ener = 0;
std::vector<int> thresholds(5);
std::chrono::milliseconds tmilisleep;

int n_devices_io = 0, n_cpu = 0,  n_cores = 0, n_interfaces = 0, n_samples = 0, n_core_temps = 0;
std::vector<int> comparePositions;
unsigned int tinterval = 1;
int lastcpu=0;
int lastmem=0;
int currcpu=0;
int currmem=0;


/* Alarms*/
int cpu_lo = 10;
int cpu_hi = 90;
int mem_lo = 15;
int mem_hi = 80;
int com_hi = 75;

/* Redis params */
int redis_port = 6379; //default
int redis_isunix = 0;
redisContext *c;
redisReply *reply;

/*
****************************************************************************************************************
* Computes the size of each package from the number of devices, interfaces, and samples that are sent in each packet.
****************************************************************************************************************
*/
void calculate_packed_size(){
    	int packed_size = 0;

    	packed_size = 13 + 2 * (hw_features.n_devices_io + hw_features.net_interfaces.size()+ n_core_temps) + (n_samples - 1 ) * (8 + 2 * (hw_features.n_devices_io + hw_features.net_interfaces.size() + n_core_temps));
}

void signalHandler(int _signal){
	cout << endl;
	cout << "Signal received: " << _signal << endl;
	delete ps;
	exit(_signal);
}

void sigAlarmHandler(int _signal){
 	signal(SIGALRM,sigAlarmHandler);
}

//Setup signal handler for termination 
void setup_signal_term(){
	signal(SIGINT, signalHandler);
        signal(SIGALRM,sigAlarmHandler);

}

/**
 * If sample values are in a determined range, it won't be sended (to reduce net usage)
 * @param ps
 * @return 0 if sample must be sended, 1 if not.
 */
int checkRangePS(Packed_sample* ps){
    int cont = 0;
    int i = 8;
    if (comparePositions.size() == 0){
        int pos = 12;
        comparePositions.push_back(pos);
        pos++;
        comparePositions.push_back(pos);
        pos++; // points to energy
        for (int a = 0; hw_features.n_devices_io > a; a++){
            pos+=2;
            comparePositions.push_back(pos);
        }
        for (int a = 0; hw_features.n_interfaces > a; a++){
            pos+=2;
            comparePositions.push_back(pos);
        }
        pos += 2*hw_features.n_core_temps;
        pos++;
        comparePositions.push_back(pos);
        pos++;
        comparePositions.push_back(pos);
    }

    for (auto i : comparePositions){//i; i < ps->packed_ptr; i++){
        if ((_last_ps.packed_buffer[i] + threshold) < ps->packed_buffer[i] || (_last_ps.packed_buffer[i] - threshold) > ps->packed_buffer[i])
            cont++;
    }

    if (cont > 0) {
        cont = 1;
        memcpy(&_last_ps.packed_buffer, ps->packed_buffer, sizeof(ps->packed_buffer));
    }
    return cont;
}

/**
 * If certain metrics exceed certain thresholds, an alarm is triggered. 
 * @param ps
 */
void checkAlarm(Packed_sample* ps){

    int a1=0, a2=0, a3=0, a4=0, a5=0;

    if ((unsigned int)ps->packed_buffer[10] > mem_hi){ a1 = 1; }
    if ((unsigned int)ps->packed_buffer[10] < mem_lo){ a2 = 1; }
    if ((unsigned int)ps->packed_buffer[11] > cpu_hi){ a3 = 1; }
    if ((unsigned int)ps->packed_buffer[11] < cpu_lo){ a4 = 1; }

    int pos = 11;// cpu
    pos += hw_features.n_cpu;
    for (int a = 0; hw_features.n_devices_io > a; a++){
            pos+=2;
    }
    for (int a = 0; hw_features.n_interfaces > a; a++){
            pos+=2;
	    if((unsigned int)ps->packed_buffer[pos] > com_hi){ a5 = 1; }
    }

    /*char buf[64];
    snprintf(buf, 64,"%i:%i:%i:%i:%i",a1, a2, a3, a4, a5); //app deleted because it is not trivial to get it
    reply = (redisReply*)redisCommand(c, "SET %s %s", hw_features.hostname, buf);
    freeReplyObject(reply);*/
}


/**
 * This function executes a command in linux returning the output.
 * @param cmd
 * @return
 */
std::string execCommand(const char * cmd){
    std::string res = "";
    std::array<char,128> buffer;

    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe){
        throw std::runtime_error("popen() failed");
    }

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr){
        res += buffer.data();
    }

    return res;
}


/**
 * Update internal params from a file edited by user
 */
void updateConfParams() {

  std::cout << "\n********** Updating parameters *************" << std::endl;
  std::ifstream initfile(CONF_FILE);
  int error = 0;
  std::string line;
  int param_n = 0;
  int sizet = 0;
  while (std::getline(initfile, line)) {
    if (line.length() > 0 && line.at(0) != '#') {
      switch (param_n) {
        case 0: // interval time
          tinterval = strtol(line.c_str(), NULL, 10);
          tmilisleep = std::chrono::milliseconds(tinterval);
          printf("Interval time: %i\n", tinterval);
          param_n++;
          break;
        case 1: //port
          port = strtol(line.c_str(), NULL, 10);
          printf("Port: %i\n", port);
          param_n++;
          break;
        case 2: //num samples
          n_samples = strtol(line.c_str(), NULL, 10);
          printf("Num samples: %i\n", n_samples);
          param_n++;
          break;
        case 3: // server ip
          sizet = strlen(line.c_str());
          server = (char *) malloc(sizet);
          strcpy(server, line.c_str());
          printf("Server ip: %s\n", server);
          param_n++;
          break;
        case 4: // ES ip
          sizet = strlen(line.c_str());
          es_addr = (char *) malloc(sizet);
          strcpy(es_addr, line.c_str());
          printf("ElasticSearch IP: %s\n", es_addr);
          param_n++;
          break;
        case 5: // bitmap mode (0 off, 1 on)
          hw_features.modo_bitmap = strtol(line.c_str(), NULL, 10);
          cout << "Bitmap: " << hw_features.modo_bitmap << endl;
          param_n++;
          break;
        case 6: // threshold filter (0 no, 1 yes)
          net_reducer = strtol(line.c_str(), NULL, 10);
          printf("Threshold filter: %i\n", net_reducer);
          param_n++;
          break;
        case 7: //threshold filter value
          threshold = strtol(line.c_str(), NULL, 10);
          printf("Threshold filter value: %i\n", threshold);
          param_n++;
          break;
        case 8: // top interval
          top_relation = strtol(line.c_str(), NULL, 10);
          printf("Interval TOP: %i\n", top_relation);
          param_n++;
          break;
        case 9: // TMR (0 simple, 1 triple)
          tmrd = strtol(line.c_str(), NULL, 10);
          printf("TMR mode: %i\n", tmrd);
          param_n++;
          break;
        case 10: // backup IP 1
          sizet = strlen(line.c_str());
          server2 = (char *) malloc(sizet);
          strcpy(server2, line.c_str());
          if (strcmp(server2, "-1") == 0) server2 = NULL;
          printf("Server backup 1: %s\n", server2);
          param_n++;
          break;
        case 11: // backup IP 2
          sizet = strlen(line.c_str());
          server3 = (char *) malloc(sizet);
          strcpy(server3, line.c_str());
          if (strcmp(server3, "-1") == 0) server3 = NULL;
          printf("Server backup 2: %s\n", server3);
          param_n++;
          break;
        case 12:
          only_hotspots = strtol(line.c_str(), NULL, 10);
          if (only_hotspots != 0)
            printf("Notify only hotspots.\n");
          else
            printf("Notify all.\n");
          param_n++;
          break;
        case 13:
          th_mem = strtol(line.c_str(), NULL, 10);
          printf("Threshold MEM: %i\n", th_mem);
          param_n++;
          break;
        case 14:
          th_cpu = strtol(line.c_str(), NULL, 10);
          printf("Threshold CPU: %i\n", th_cpu);
          param_n++;
          break;
        case 15:
          th_io = strtol(line.c_str(), NULL, 10);
          printf("Threshold IO: %i\n", th_io);
          param_n++;
          break;
        case 16:
          th_net = strtol(line.c_str(), NULL, 10);
          printf("Threshold NET: %i\n", th_net);
          param_n++;
          break;
        case 17:
          th_ener = strtol(line.c_str(), NULL, 10);
          printf("Threshold Energy: %i\n", th_ener);
          param_n++;
          break;
        case 18:
          printf("More lines than expected in init.dae file.\n");
          break;
        default:
          error = 1;
          break;
      }
    }
  }

  if (error == 0) {
    //update thresholds
    thresholds[0] = th_mem;
    thresholds[1] = th_cpu;
    thresholds[2] = th_ener;
    thresholds[3] = th_io;
    thresholds[4] = th_net;

    error = init_socket(server, port);
    if (error == -1) {
      std::cerr << "Socket could not be initialized" << endl;
    }
    if (server2 != NULL) {
      error = init_socket_backup1(server2, port);
      if (error == -1) {
        std::cerr << "Backup 1 Socket could not be initialized" << endl;
      }
    }
    if (server3 != NULL) {
      error = init_socket_backup2(server3, port);
      if (error == -1) {
        std::cerr << "Backup 2 Socket could not be initialized" << endl;
      }
    }
  }
}


int main(int argc, char *argv[]) {

  using namespace std::chrono;
  using clk = high_resolution_clock;
  int i = -1;
  FILE *fp;
  clk::time_point t1;
  clk::time_point t2;
  char *slurm_cmd = (char *) malloc(128);
  std::string job_name = "";

  clk::duration difft;
  clk::duration difference;

  stringstream sargv;
  string sarg;

  if (argc != 1) {
    cerr << "Usage: ./DaeMon [params configured in init.dae configuration file]" << endl;
    exit(1);
  }

  std::ifstream initfile(INIT_FILE);
  std::string line;
  int param_n = 0;
  int sizet = 0;
  while (std::getline(initfile, line)) {
    if (line.length() > 0 && line.at(0) != '#') {
      switch (param_n) {
        case 0: // interval time
          tinterval = strtol(line.c_str(), NULL, 10);
          printf("Interval time: %i\n", tinterval);
          param_n++;
          break;
        case 1: //port
          port = strtol(line.c_str(), NULL, 10);
          printf("Port: %i\n", port);
          param_n++;
          break;
        case 2: //num samples
          n_samples = strtol(line.c_str(), NULL, 10);
          printf("Num samples: %i\n", n_samples);
          param_n++;
          break;
        case 3: // server ip
          sizet = strlen(line.c_str());
          server = (char *) malloc(sizet);
          strcpy(server, line.c_str());
          printf("Server ip: %s\n", server);
          param_n++;
          break;
        case 4: // DB ip
          sizet = strlen(line.c_str());
          es_addr = (char *) malloc(sizet);
          strcpy(es_addr, line.c_str());
          printf("Redis IP: %s\n", es_addr);
          param_n++;
          break;
        case 5: // bitmap mode (0 off, 1 on)
          hw_features.modo_bitmap = strtol(line.c_str(), NULL, 10);
          cout << "Bitmap: " << hw_features.modo_bitmap << endl;
          param_n++;
          break;
        case 6: // threshold filter (0 no, 1 yes)
          net_reducer = strtol(line.c_str(), NULL, 10);
          printf("Threshold filter: %i\n", net_reducer);
          param_n++;
          break;
        case 7: //threshold filter value
          threshold = strtol(line.c_str(), NULL, 10);
          printf("Threshold filter value: %i\n", threshold);
          param_n++;
          break;
        case 8: // top interval
          top_relation = strtol(line.c_str(), NULL, 10);
          printf("Interval TOP: %i\n", top_relation);
          param_n++;
          break;
        case 9: // TMR (0 simple, 1 triple)
          tmrd = strtol(line.c_str(), NULL, 10);
          printf("TMR mode: %i\n", tmrd);
          param_n++;
          break;
        case 10: // backup IP 1
          sizet = strlen(line.c_str());
          server2 = (char *) malloc(sizet);
          strcpy(server2, line.c_str());
          if (strcmp(server2, "-1") == 0) server2 = NULL;
          printf("Server backup 1: %s\n", server2);
          param_n++;
          break;
        case 11: // backup IP 2
          sizet = strlen(line.c_str());
          server3 = (char *) malloc(sizet);
          strcpy(server3, line.c_str());
          if (strcmp(server3, "-1") == 0) server3 = NULL;
          printf("Server backup 2: %s\n", server3);
          param_n++;
          break;
        case 12:
          only_hotspots = strtol(line.c_str(), NULL, 10);
          if (only_hotspots != 0)
            printf("Notify only hotspots.\n");
          else
            printf("Notify all.\n");
          param_n++;
          break;
        case 13:
          th_mem = strtol(line.c_str(), NULL, 10);
          printf("Threshold MEM: %i\n", th_mem);
          param_n++;
          break;
        case 14:
          th_cpu = strtol(line.c_str(), NULL, 10);
          printf("Threshold CPU: %i\n", th_cpu);
          param_n++;
          break;
        case 15:
          th_io = strtol(line.c_str(), NULL, 10);
          printf("Threshold IO: %i\n", th_io);
          param_n++;
          break;
        case 16:
          th_net = strtol(line.c_str(), NULL, 10);
          printf("Threshold NET: %i\n", th_net);
          param_n++;
          break;
        case 17:
          th_ener = strtol(line.c_str(), NULL, 10);
          printf("Threshold Energy: %i\n", th_ener);
          param_n++;
          break;
        case 18:
          printf("More lines than expected in init.dae file.\n");
          break;
        default:
          return -1;
      }
    }
  }

  tmilisleep = std::chrono::milliseconds(tinterval);
  thresholds[0] = th_mem;
  thresholds[1] = th_cpu;
  thresholds[2] = th_ener;
  thresholds[3] = th_io;
  thresholds[4] = th_net;



  // **************  REDIS INITIALIZATION **********
  c = redisConnect(es_addr, 6379); //localhost for debug
  if (c == NULL || c->err) {
    if (c) {
      printf("Connection error: %s\n", c->errstr);
      redisFree(c);
    } else {
      printf("Connection error: can't allocate redis context\n");
    }
  }

  printf("Redis connected \n");


  //Setup for signal termination
  setup_signal_term();

  // Read number of processor
  error = read_n_processors(hw_features.cpus, hw_features.n_cpu, hw_features.n_cores);//, hw_features.n_siblings);
  if (error != EOK) {
    cerr << "Error: " << error << endl;
    exit(0);
  }

  // Get Ip Address
  get_addr(hw_features.ip_addr_s, hw_features.hostname);
  cout << "hostname es: " << hw_features.hostname << endl;

  // Get memory total
  get_mem_total(hw_features.mem_total);

  header_append("IP_Addr Mem(GB) MemUsage(%) NCPU NCores CPUBusy(%)");

  // Get power path
  //error = get_power_path(hw_features.pwcpu_features, hw_features.path_dir, hw_features.n_cpu);

  // Read number of devices
  error = read_n_devices(hw_features.io_dev, hw_features.n_devices_io);
  if (error != EOK) {
    cerr << "Error: " << error << endl;
    exit(0);
  }
  header_append(" ");

  error = read_n_net_interface(hw_features.net_interfaces, hw_features.n_interfaces);
  if (error != EOK) {
    cerr << "Error: " << error << endl;
    exit(0);
  }

  log_concat_interfaces(hw_features);

  ps = new Packed_sample(hw_features, tinterval, n_samples, threshold);

  // Print on the screen with the output format of the data.
  cout << get_header_line() << endl;

  calculate_packed_size();

  //INIT Communications: Check that everything is correct
  if (server == NULL) {
    std::cerr << "ERROR: NO address for master was given." << std::endl;
    exit(-1);
  }
  if ((port < 1024) || (port > 65535)) {
    fprintf(stderr, "Error: Port must be in the range 1024 <= port <= 65535");
    exit(-1);
  }


  error = init_socket(server, port);
  if (error == -1) {
    std::cerr << "Socket could not be initialized" << endl;
    return error;
  }
  if (server2 != NULL) {
    error = init_socket_backup1(server2, port);
    if (error == -1) {
      std::cerr << "Backup 1 Socket could not be initialized" << endl;
      return error;
    }
  }
  if (server3 != NULL) {
    error = init_socket_backup2(server3, port);
    if (error == -1) {
      std::cerr << "Backup 2 Socket could not be initialized" << endl;
      return error;
    }
  }

  //Send initial packet with the configuration
  //send_conf_packet(&hw_features);

  //Time where the conf file was updated
  std::filesystem::file_time_type last_conf_update;
  bool init = true;

  while (1) {

    t1 = clk::now();

    // ************************* CHECK IF THERE IS A CONFIGURATION UPDATE*************
    if(std::filesystem::exists(CONF_FILE)){
      if(init){
        init = false;
        auto ftime = std::filesystem::last_write_time(CONF_FILE);
        last_conf_update = ftime;
        updateConfParams();
      }
      else{
          auto ftime = std::filesystem::last_write_time(CONF_FILE);
          if(ftime > last_conf_update){
            last_conf_update = ftime;
            updateConfParams();
          }
      }
    }
    // ******************************************************************************

    // Clear log_line
    log_clear();

    // Concat ip address
    log_append(hw_features.ip_addr_s);

    // *************************  MEMORY USAGE ******************
    error = read_memory_stats();
    if (error != EOK) {
      cerr << "Error: " << error << endl;
      exit(0);
    }

    // *************************** CPU USAGE ************************
    //hw_features.cores.clear();
    read_cpu_stats(hw_features.cpus /*, hw_features.cores*/, hw_features.n_cpu, hw_features.n_cores);
   
    // *************************** POWER USAGE **************************
    //get_power(hw_features.pwcpu_features, hw_features.path_dir, hw_features.n_cpu);



    // ************************** DEVICES USAGE ************************
    read_devices_stats(hw_features.io_dev, tinterval);
   
    // **************************** NET USAGE ******************************
    read_net_stats(hw_features.net_interfaces);
   
    // ************************ PACKET TRANSFER ***********************
    if (i != -1) {
      ps->pack_sample_s(get_log_line(), 0, 0);//, hw_features.cores);
      //ps->pack_sample_prometheus(get_log_line(), job_name);
      //ps->pack_sample_generic("capo14;18446744073709551615;campo2;543;campo3;18446744073709551615;campo4;1111;campo5;123;campo6;543;");
      ps->packed_ptr++;

      cout << "Sending: " << get_log_line() << endl;
      i++;

      //Send to redis
      printf("SET monitor:%s %s", hw_features.hostname.c_str(), get_log_line().c_str());
      reply = (redisReply*)redisCommand(c,"SET monitor:%s %s", hw_features.hostname.c_str(), get_log_line().c_str());
      if (reply == NULL) fprintf(stderr, "Failed to execute Redis command\n");
      freeReplyObject(reply);

      //checkAlarm(ps);

      if (i == n_samples) {
        i = 0;
        // create packet and send
        if (net_reducer == 1) {
          if (_last_ps.ip_addr_s == "" || heartbit == 10) {
            heartbit = 0;
            _last_ps.ip_addr_s = ps->ip_addr_s;
            memcpy(&_last_ps.packed_buffer, ps->packed_buffer, sizeof(ps->packed_buffer));
            send_monitor_packet(*ps, tmrd);
          } else if (checkRangePS(ps) != 0)
            send_monitor_packet(*ps, tmrd);
          else
            heartbit++;
        } else
          send_monitor_generic(*ps, tmrd);
      }
    }

    t2 = clk::now();

    difft = (t2 - t1);

    if (difft < tmilisleep) {
      std::chrono::milliseconds randomize(0);//rand() % 2000);
      clk::duration difference = (tmilisleep - difft - randomize);

      struct timeval tval;
      tval.tv_sec = std::chrono::duration_cast<std::chrono::microseconds>(difference).count() / 1000000;
      tval.tv_usec = std::chrono::duration_cast<std::chrono::microseconds>(difference).count() % 1000000;

      struct itimerval tival;
      tival.it_value = tval;
      tival.it_interval.tv_sec = 0;
      tival.it_interval.tv_usec = 0;

      setitimer(ITIMER_REAL, &tival, NULL);
      pause();


    } else {

#ifdef DEBUG_TIME
      cout << " Time -> overflow" << endl;
#endif

    }
    if (i < 0) { i++; }
  }

  // Disconnects and frees the context
  redisFree(c);

  return 0;
}



#endif
