#ifndef CPU_INFO
#define CPU_INFO

#include "cpu_info.hpp"

#include <string>

#include "daemon_monitor.hpp"
#include "common.hpp"
#include "system_features.hpp"
/*
****************************************************************************************************************
* Reads cpu status and stores info.
* 
****************************************************************************************************************
*/
int read_cpu_stats(std::vector<Cpu_dev> & s_cpu, int n_cpu, int n_cores) {

    std::ifstream myfile("/proc/stat");

    char *cpu = (char*)malloc(4);

    std::string line_2;

    int procsr = 0;

    unsigned long long sum_diff = 0;

    unsigned long long idle_diff = 0;

    double didle_diff = 0.0;
    double dbusy_diff = 0.0;

    double idle_ = 0.0, stinterval = 0.0;
    unsigned long long current_sum = 0, pre_sum = 0;

    if (myfile.good()) {
        s_cpu[0].stats[1] = s_cpu[0].stats[0];

        getline(myfile, line_2);
        std::vector<std::string> procs_run = split(line_2, ' ');

        if (procs_run[0].compare("cpu") == 0) {

            sscanf(line_2.c_str(), "%s %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
                   cpu,
                   &s_cpu[0].stats[0].user,
                   &s_cpu[0].stats[0].nice,
                   &s_cpu[0].stats[0].sys,
                   &s_cpu[0].stats[0].idle,
                   &s_cpu[0].stats[0].iowait,
                   &s_cpu[0].stats[0].hardirq,
                   &s_cpu[0].stats[0].softirq,
                   &s_cpu[0].stats[0].steal,
                   &s_cpu[0].stats[0].guest,
                   &s_cpu[0].stats[0].guest_nice);
        }

        if (procs_run[0].compare("procs_running") == 0) {
            procsr = atoi(procs_run[1].c_str());
        }

        //myfile.close();

        /* Divide between the number of cores */

        /*Sums do not include guest and guest nice because cpu user and cpu nice
        * already include those
        */
        current_sum = s_cpu[0].stats[0].user +
                      s_cpu[0].stats[0].nice +
                      s_cpu[0].stats[0].sys +
                      s_cpu[0].stats[0].idle +
                      s_cpu[0].stats[0].iowait +
                      s_cpu[0].stats[0].hardirq +
                      s_cpu[0].stats[0].softirq +
                      s_cpu[0].stats[0].steal;

        pre_sum = s_cpu[0].stats[1].user +
                  s_cpu[0].stats[1].nice +
                  s_cpu[0].stats[1].sys +
                  s_cpu[0].stats[1].idle +
                  s_cpu[0].stats[1].iowait +
                  s_cpu[0].stats[1].hardirq +
                  s_cpu[0].stats[1].softirq +
                  s_cpu[0].stats[1].steal;


        /*Time interval */
        sum_diff = (unsigned long) current_sum - pre_sum;

        idle_diff = s_cpu[0].stats[0].idle - s_cpu[0].stats[1].idle;

        didle_diff = sum_diff ? (double) idle_diff / ((double) sum_diff) : 0.0;

        didle_diff = didle_diff * 100;

        dbusy_diff = 100 - didle_diff;

        /* Number of CPUs*/
        log_concat(n_cpu, 2);

        /* Number of Cores */
        log_concat(n_cores, 2);


        /* CPU IDLE*/
        if (dbusy_diff < 0.1) {
            dbusy_diff = 0.1;
        }

        log_concat(dbusy_diff, 2);


        bool fin = false;
        int core_id = 0;
        while (!fin) {
            s_cpu[core_id].stats[1] = s_cpu[core_id].stats[0];

            getline(myfile, line_2);
            std::vector<std::string> procs_run = split(line_2, ' ');

            char field[10] = "";
            sprintf(field, "cpu%i", core_id);
            if (procs_run[0].compare(field) == 0) {//"cpu") == 0) {

                sscanf(line_2.c_str(), "%s %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
                       cpu,
                       &s_cpu[core_id + 1].stats[0].user,
                       &s_cpu[core_id + 1].stats[0].nice,
                       &s_cpu[core_id + 1].stats[0].sys,
                       &s_cpu[core_id + 1].stats[0].idle,
                       &s_cpu[core_id + 1].stats[0].iowait,
                       &s_cpu[core_id + 1].stats[0].hardirq,
                       &s_cpu[core_id + 1].stats[0].softirq,
                       &s_cpu[core_id + 1].stats[0].steal,
                       &s_cpu[core_id + 1].stats[0].guest,
                       &s_cpu[core_id + 1].stats[0].guest_nice);
            } else {
                fin = true;
            }

            if (procs_run[0].compare("procs_running") == 0) {
                procsr = atoi(procs_run[1].c_str());
            }

            //myfile.close();

            /* Divide between the number of cores */

            /*Sums do not include guest and guest nice because cpu user and cpu nice
            * already include those
            */
            if (procs_run[0].compare(field) == 0) {
                current_sum = s_cpu[core_id+1].stats[0].user +
                              s_cpu[core_id+1].stats[0].nice +
                              s_cpu[core_id+1].stats[0].sys +
                              s_cpu[core_id+1].stats[0].idle +
                              s_cpu[core_id+1].stats[0].iowait +
                              s_cpu[core_id+1].stats[0].hardirq +
                              s_cpu[core_id+1].stats[0].softirq +
                              s_cpu[core_id+1].stats[0].steal;

                pre_sum = s_cpu[core_id+1].stats[1].user +
                          s_cpu[core_id+1].stats[1].nice +
                          s_cpu[core_id+1].stats[1].sys +
                          s_cpu[core_id+1].stats[1].idle +
                          s_cpu[core_id+1].stats[1].iowait +
                          s_cpu[core_id+1].stats[1].hardirq +
                          s_cpu[core_id+1].stats[1].softirq +
                          s_cpu[core_id+1].stats[1].steal;


                /*Time interval */
                sum_diff = (unsigned long) current_sum - pre_sum;

                idle_diff = s_cpu[core_id+1].stats[0].idle - s_cpu[core_id+1].stats[1].idle;

                didle_diff = sum_diff ? (double) idle_diff / ((double) sum_diff) : 0.0;

                didle_diff = didle_diff * 100;

                dbusy_diff = 100 - didle_diff;

                /* CPU IDLE*/
                if (dbusy_diff < 0.1) {
                    dbusy_diff = 0.1;
                }

            }
        }
        myfile.close();
    } else {

        return EFILE;

    }

    free(cpu);

    return EOK;

}

/*
****************************************************************************************************************
* Reads number of processors and store information.
****************************************************************************************************************
*/
int read_n_processors(std::vector<Cpu_dev> & cpus, int & n_cpu, int & n_cores) {
    /* Read from file cpuinfo as many times as word processor appears.*/
    std::string line_n_cpu;

    int n_processors = 0;

    std::stringstream ss, ssc;

    ifstream n_cpu_file("/proc/cpuinfo");
    if (n_cpu_file.good()) {

        Cpu_dev cpudev;

        while (getline(n_cpu_file, line_n_cpu)) {
            std::vector<std::string> procs_run = split(line_n_cpu, ' ');

            if (procs_run.empty()) {

                continue;

            } else {

                if (procs_run[0].compare("processor") == 0) {

                    n_processors++;

                } /*else if ((procs_run[0].compare("siblings") == 0)) {
                    if (n_siblings < 0) {
                        ss << procs_run[2];
                        ss >> n_siblings;
                        cpus.push_back(cpudev);
                    }
                } else*/ if ((procs_run[0].compare("cpu") == 0) && (procs_run[1].compare("cores") == 0)) {

                    ss.clear();
                    ss << procs_run[3];
                    ss >> n_cores;

                    cpus.push_back(cpudev);

                } else if ((procs_run[0].compare("physical") == 0) && (procs_run[1].compare("id") == 0)) {
                    ssc.clear();

                    ssc << procs_run[3];
                    ssc >> n_cpu;
                    n_cpu += 1;

                } else if ((procs_run[0].compare("model")) == 0 && (procs_run[1].compare("name") == 0)) {
                    cpudev.model_name.clear();
                    for (int i = 3; i < procs_run.size(); i++) {
//            				cout << procs_run[i] << endl;
                        cpudev.model_name.append(procs_run[i]);
                        cpudev.model_name.append(" ");
                    }
                }
            }
        }
    } else {
        return EFILE;
    }

    n_cpu_file.close();
    return EOK;
}


#endif
