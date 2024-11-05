
#ifndef DEVICES_INFO_H
#define DEVICES_INFO_H


#include <string>
#include <vector>

#define DEVICES_STATS_SIZE   (sizeof(Devices_stats))

using namespace std;

typedef struct devices_stats{
        /* Number of read operations issued to the device */
        unsigned long rd_ios;
        /* Number of read request merged */
        unsigned long rd_merges;
        /* Number of sectors read */
        unsigned long rd_sec;
        /* Time of read requests in queue */
        unsigned long int rd_ticks;
        /* Number of write operations issued to the device */
        unsigned long wr_ios;
        /* Number of write request merged */
        unsigned long wr_merges;
        /* Number of sectors written */
        unsigned long wr_sec;
        /* Time of write requests in queue */
        unsigned int wr_ticks;
        /* Number of I/Os in progress */
        unsigned int ios_pgr;
        /* Number of ticks total (for this device) for I/O */
        unsigned int tot_ticks;
        /* Number of ticks requests spent in queu e*/
        unsigned int rq_ticks;
}Devices_stats;

typedef struct io_dev{
        std::string dev_name;
        Devices_stats stats[2];
}IO_dev;




/**
        Read the number of devices

        @param [in, out] s_dv features of each device
        @param [in, out] n_devices number of devices in the node
*/
int read_n_devices(std::vector<IO_dev> & s_dv, int & n_devices);



/**
        Function that its called each loop to read devices stats

        @param [in, out] s_dv features of each device
        @param [in] tinterval time of the interval, used to calculate stats 
*/
void read_devices_stats(std::vector<IO_dev> & s_dv, unsigned int tinterval);


/**
        Update values of a device
*/
void update_values(Devices_stats * s_dv[2], unsigned long rd_ios,
        unsigned long rd_merges,
        unsigned long rd_sec,
        unsigned long rd_ticks,
        unsigned long wr_ios,
        unsigned long wr_merges,
        unsigned long wr_sec,
        unsigned int wr_ticks,
        unsigned int ios_pgr,
        unsigned int tot_ticks,
        unsigned int rq_ticks);

#endif
