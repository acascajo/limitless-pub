#ifndef PACKED_SAMPLE_H
#define PACKED_SAMPLE_H


#include <cmath>
#include <vector>
#include <string>
#include "system_features.hpp"
#include <ctime>
#include <memory>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

#define MEMORY_OUT 128
#define CPU_OUT 64 //Primer bit empezando por la izquierda
#define DEVICE_OUT 32 //Segundo bit
#define NET_OUT 16
#define TEMP_OUT 8
#define POS7 128
#define POS6 64

#define mem_cpu 2

/* Positions of the values in the packed_buffer */
/* Mem usage position */
#define MEM_POSITION 11
/* CPU Idle position */
#define CPU_POSITION 12
/* w(%) position */
#define DEVICE_POSITION 13
/* Speed of the interface position */
#define NET_POSITION 13 + (2 * n_devices_io)
/* Temperature actual position */
#define  TEMP_POSITION 15 + (2 * n_interfaces)

#define SAMPLES_BUFFER_SIZE 1024
using namespace std;

class Packed_sample {

public:

    unsigned char packed_buffer[SAMPLES_BUFFER_SIZE];
    unsigned char n_samples;

    unsigned int sample_size;
    unsigned int interval;
    int memtotal;
    int samples_packed;
    int modo_bitmap;  //Modo bitmap 0: solo se envia muestra, 1: solo se envia bitmap, 2: bitmap y muestra
    int packed_ptr;
    int packed_bytes;
    int sample_pt;
    int n_devices_io;
    int n_interfaces;
    int threshold;
    int n_cpu;
    int n_cores;
    int tot_acc;
    int load_miss;
    int store_miss;
    int pos_bitmap; // Position in the buffer sample where start bitmap
    int pos_sample; // Position in the buffer sample where start sample data
    time_t time_sample;
    std::string ip_addr_s;
    std::vector<std::string> ip_v;
    int n_siblings;

    /* bit map variables */
    unique_ptr<unsigned char[]> bit_map;//FIXME: should pass this object always as ref so this pointer can work correctly
    int datos_cuartil;
    int bytes_bitmap;
    int bit_map_ptr;
    int byte_bit_map;

    unsigned char memory_avg;
    unsigned char cpu_avg;

    vector<unsigned char> pw_cpu;
    vector<unsigned char> dev_avg;
    vector<unsigned char> net_avg;

    Packed_sample();

    Packed_sample(Hw_conf hwconf, unsigned int interval, int n_samples, int threshold);

    ~Packed_sample();

    void set_n_samples(int n_samples);

    void set_n_devices_io(int n_devices_io);

    void set_n_interfaces(int n_interfaces);

    /*
    *	Pack the features in the packed_buffer
    */
    void pack_features();


    /**
        Pack sample into buffer to be sent.

        @param [in] sample String to be packed in the buffer.
    */
    void pack_monitoring(int mem_usa, int cpu_usa, int *devices_usa[2], int *net_usa, int *temp);


    /**
        Pack sample into buffer to be sent.

        @param [in] sample String to be packed in the buffer.
    */
    void pack_sample_s(std::string sample);

    void pack_sample_s(std::string sample, double cache_miss, double cpu_stalled);//, std::vector<Core_dev> cores);

    void pack_sample_prometheus(string sample, std::string jobname);

    void pack_sample_generic(string sample);

    void Aggregation_sample(std::string ip, int mem, int cpu, int iot, int iow, int net);

    void Aggregation_sample_generic(std::string ip, std::vector<unsigned long> counters, std::vector<string> keys);


    /**
        Splits the log

        @param [in] str String representing the log content.
        @return Vector of strings with the log splitted

    */
    vector<string> split_log(std::string str);


    /**
        Parse sample to include on the buffer

        @param [in] sample String including sample to be parsed.
        @return Vector of strings including the parsed contents.

    */
    vector<string> parse_log(std::string sample);

    /**
        Calculate the size of a sample.

    */
    void calculate_sample_size();


    /**
        Calculate size of bit map on bytes

        @return Size of bit map on bytes

    */
    int calculate_bit_maps_bytes();


    /**
        Calculate size of bit map on bytes

        @return Size of bit map on bytes

    */
    void to_bit_map();


    /**
        Encodes the value passed by parameters in the bit map

        @param[in] cuartil number to be coded

    */
    void codificar_cuartil(int cuartil);

};


void serialize_int(unsigned char *buffer, unsigned int value);
int deserialize_int(unsigned char *buffer);
void serialize_long(unsigned char *buffer, unsigned long value);

/** IMPORTED FROM NETWORKDATA*/

/* This function keeps sending bytes from a buffer until the specified amount	*
 * is actually sent. It is designed to overcome the problem of send function	*
 * being able to return having sent only a portion of the specified bytes.	*
 * n: specifies the amount of bytes to send from the first positions 		*
 * of the buffer.								*
 * On success n is returned.							*
 * On error -1 is returned.							*/
int sendn(int socket_descriptor, void *buf, int n, struct sockaddr_in *out_addr);

/* This function keeps receiving bytes from a buffer until the specified amount	*
 * is actually received. It is designed to overcome the problem of recv function*
 * being able to return having received only a portion of the specified bytes.	*
 * n: specifies the amount of bytes to receive in the buffer.			*
 * On success n is returned.							*
 * On premature close/shutdown on peer 0 is returned.				*
 * On error -1 is returned.							*/
int recvn(int socket_descriptor, void *buf, int n);

int convert_time_to_char(unsigned char * buffer, time_t time);

int convert_char_to_int(unsigned char * buffer);

int convert_char_to_short_int(unsigned char * buffer, unsigned short int *number);

int convert_short_int_to_char(char * buffer, unsigned short int number);

int calculate_conf_packet(Hw_conf hw_conf);

int convert_short_int_to_char(char * buffer, unsigned short int number);

int create_conf_packet(Hw_conf hw_conf, char * conf_packed);

int create_sample_packet(Packed_sample &info, unsigned char * buffer);

int create_generic_packet(Packed_sample &info, unsigned char * buffer);


#endif
