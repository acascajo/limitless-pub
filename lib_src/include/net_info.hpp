
#ifndef NET_INFO_H
#define NET_INFO_H

#include <vector>
#include <string>

using namespace std;


typedef struct net_stats{
        unsigned long long rx_bytes;
        unsigned long long tx_bytes;
}Net_stats;

typedef struct net_dev{
        /* Name of node intefaces */
        std::string net_name;
        unsigned int speed;
        Net_stats stats[2];
}Net_dev;


/**
        Obtain number of interfaces and its speed

        @param [in, out] net_interfaces return name and speed for each interface
*/
int read_n_net_interface(std::vector<Net_dev> & net_interfaces, int & n_interfaces);


/**
        Get the stats for each interface include in net_interfaces

        @param [in, out] net_interfaces return stats
*/
void read_net_stats(std::vector<Net_dev> & net_interfaces);


/**
        Obtain name of the host and return ip address in a string

        @param [out] my_addr_s return ip address
*/
void get_addr(string & my_addr_s, string & hostname_s);

#endif
