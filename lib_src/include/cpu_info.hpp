#ifndef CPU_INFO_H
#define CPU_INFO_H

#include <string>
#include <vector>

#define CPU_STATS_SIZE   (sizeof(Cpu_stats))

using namespace std;


typedef struct cpu_stats{
        unsigned long long user; //normal processes executing in user mode
        unsigned long long nice; //niced processes (allow other processes to get a greater share of CPU) executing in user mode
        unsigned long long sys; //processes executing in kernel mode
        unsigned long long idle;
        unsigned long long iowait; //waiting for IO to complete
        unsigned long long hardirq; //hardware interrupts
        unsigned long long softirq; //software interrupts
        unsigned long long steal;
        unsigned long long guest;
        unsigned long long guest_nice;
}Cpu_stats;

typedef struct cpu_dev{
        string model_name;
        Cpu_stats stats[2];
}Cpu_dev;



/**
        Reads cpu status and stores info.

        @param [in,out] s_cpu vector of Cpu_dev where the stats will be saved (general).
        @param [in,out] s_cores vector of Core_dev where the stats will be saved (for each core).
        @param [in] n_cpu number of cpu in the node
        @param [in] n_cores number of cores in the node
        
*/
int read_cpu_stats(std::vector<Cpu_dev> & s_cpu, int n_cpu, int n_cores);


/**
        Reads number of processors and store information.

        @param [in,out] cpus vector of Cpu_dev in the node
        @param [in,out] n_cpu number of cpus in the node
        @param [in, out] n_cores number of cores in the node
        
*/
int read_n_processors(std::vector<Cpu_dev> & cpus, int & n_cpu, int & n_cores);

#endif
