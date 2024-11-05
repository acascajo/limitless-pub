#ifndef SERVER_DATA_H
#define SERVER_DATA_H


#include <string>
#include "system_features.hpp"


/**
    Obtain the hardware configuration from a clientIP that  is already registered.
    @param[in] clientIP     String containing the IP of the client.
    @param[out] hw_conf     Pointer to a Hw_conf structure that will contain the result of the search.
    @returns 0 in case everything went ok, -1 in case of error or key was not found

*/


int obtain_hw_conf(const std::string & clientIP, Hw_conf * hw_conf);

/**
    Insert the harware configuration into the table.
    @param[in] hw_conf     Hardware configuration to be inserted.
    @returns 0 in case everything went ok, -1 in case of error

*/
int insert_hw_conf( Hw_conf hw_conf);
int recovery_insert_hw_conf( Hw_conf hw_conf);





#endif