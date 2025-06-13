#include <iostream>
#include <fstream>
#include <unistd.h>
#include <cstring>
#include "iba_info.hpp"
#include <string>
#include <sys/types.h>          /* See NOTES */
#include "common.hpp"

using namespace std;


/*
****************************************************************************************************************
* Reads network status and stores info.
****************************************************************************************************************
*/
void read_iba_stats(IBA_dev & iba_data) {

	string path_xmitData("/sys/class/infiniband/mlx4_0/ports/1/counters/port_xmit_data");
	string path_rcvData("/sys/class/infiniband/mlx4_0/ports/1/counters/port_rcv_data");
	string path_xmitPkts("/sys/class/infiniband/mlx4_0/ports/1/counters/port_xmit_packets");
	string path_rcvPkts("/sys/class/infiniband/mlx4_0/ports/1/counters/port_rcv_packets");
	string path_xmitWait("/sys/class/infiniband/mlx4_0/ports/1/counters/port_xmit_wait");

	std::stringstream ss, rr, tt, uu, vv;
	ss.precision(4);
	rr.precision(4);
	tt.precision(4);
	uu.precision(4);
	vv.precision(4);

	string xData, rData, xPacket, rPacket, xWait;
	
	unsigned long long xmitData = 0;
        unsigned long long rcvData = 0;
        unsigned long long xmitPkts = 0;
        unsigned long long rcvPkts = 0;
        unsigned long long xmitWait = 0;
	std::string dev_name = "";
        std::string guid = "";

		ss.clear();
		rr.clear();
		tt.clear();
		uu.clear();
		vv.clear();

		std::ifstream xmitData_file(path_xmitData);
		getline(xmitData_file, xData);
		rr << xData;
		rr >> xmitData;
		iba_data.stats[0].xmitData = xmitData;

		std::ifstream rcvData_file(path_rcvData);
                getline(rcvData_file, rData);
		ss << rData;
		ss >> rcvData;
		iba_data.stats[0].rcvData = rcvData;

		std::ifstream xmitPkts_file(path_xmitPkts);
                getline(xmitPkts_file, xPacket);
		tt << xPacket;
		tt >> xmitPkts;
		iba_data.stats[0].xmitPkts = xmitPkts;

		std::ifstream rcvPkts_file(path_rcvPkts);
                getline(rcvPkts_file, rPacket);
		uu << xPacket;
		uu >> xmitPkts;
		iba_data.stats[0].rcvPkts = rcvPkts;

		std::ifstream xmitWait_file(path_xmitWait);
                getline(xmitWait_file, xWait);
		vv << xWait;
		vv >> xmitWait;
		iba_data.stats[0].xmitWait = xmitWait;


		xmitData_file.close();
		rcvData_file.close();
		xmitPkts_file.close();
		rcvPkts_file.close();
		xmitWait_file.close();
}


