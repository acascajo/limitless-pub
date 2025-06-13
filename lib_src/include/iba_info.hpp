
#ifndef IBA_INFO_H
#define IBA_INFO_H

#include <vector>
#include <string>

using namespace std;


typedef struct iba_stats{
        unsigned long long xmitData;
        unsigned long long rcvData;
        unsigned long long xmitPkts;
        unsigned long long rcvPkts;
        unsigned long long xmitWait;
}IBA_stats;

typedef struct iba_dev{
        std::string dev_name;
	std::string guid;
        IBA_stats stats[1];
}IBA_dev;


/**
        Get the stats 
*/
void read_iba_stats(IBA_dev & iba_data);


#endif
