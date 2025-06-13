#ifndef CLIENTE_MONITOR_IBA_HPP
#define CLIENTE_MONITOR_IBA_HPP

#include "cliente_monitor.hpp"

//Infiniband link
#include <infiniband/verbs.h>
#include <infiniband/arch.h>
#include <iba_info.hpp>



/**
 * Collects IBA performance data and sends it to InfluxDB and writes it into a logfile
 */
void DoIBAstuff(std::string hostname, int& xmitdata, int& xmitwait, std::string& hex_guid);
void MakeIBADecision(int ccti_increase, int xmitdata, int xmitwait);

/**
* Functions to send the data to the light server
*/
int sendn_ib(int socket_descriptor, void *buf, int n, struct sockaddr_in *out_addr);
void initializeSocket();


#endif //CLIENTE_MONITOR_IBA_HPP
