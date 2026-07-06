#include <iostream>
#include <cstdio>
#include <cmath>
#include <string>
#include <vector>
#include <array>
#include "Packed_sample.hpp"
#include "common.hpp"
using namespace std;


Packed_sample::Packed_sample(){
    packed_ptr=0;
}

Packed_sample::Packed_sample(Hw_conf hwconf, unsigned int interval, int n_samples, int threshold) {
    this->ip_addr_s.assign(hwconf.ip_addr_s);
    this->n_samples = n_samples;
    this->modo_bitmap = hwconf.modo_bitmap;
    //cout << "Obtenido el modo bitmap: " << this->modo_bitmap  << endl;
    this->n_devices_io = hwconf.n_devices_io;
    this->memtotal = hwconf.mem_total;
    this->n_interfaces = hwconf.n_interfaces;
    this->threshold = threshold;
    this->n_cpu = hwconf.n_cpu;
    this->n_cores = hwconf.n_cores;
    this->interval = interval;
#if ENABLE_GPU
    this->n_gpus = hwconf.n_gpu;
#endif
    //this->n_siblings = hwconf.n_siblings;
    time(&(this->time_sample));
    samples_packed = 0;
    sample_pt = 0;
    packed_bytes = 0;
    datos_cuartil = 0;
    packed_ptr = 0;
    calculate_bit_maps_bytes();
    this->bit_map = std::unique_ptr<unsigned char[]>(new unsigned char[bytes_bitmap]);
    pack_features();
    calculate_sample_size();
    pos_bitmap = 0;
    pos_sample = 0;
    //calculate_bit_maps_bytes();

    //sample_size = 0;

    for (int i = 0; i < n_devices_io; i++) {
        dev_avg.push_back(0);
    }

    for (int i = 0; i < n_interfaces; i++) {
        net_avg.push_back(0);
    }

#if ENABLE_GPU
    for (int i = 0; i < n_gpus; i++) {
        gpu_avg.push_back(0);
    }

    for (int i = 0; i < n_gpus; i++) {
        Gpu_accu gpuaccu;

        gpuaccu.memUsage = 0;
        gpuaccu.gpuUsage = 0;
        gpuaccu.temperature = 0;
        gpuaccu.powerUsage = 0;

        gpu_accu.push_back(gpuaccu);
    }
#endif

    for (int i = 0; i < n_cpu; i++) {
        pw_cpu.push_back(0);
    }
}

Packed_sample::~Packed_sample() {
    /* Clear vector */
    dev_avg.clear();
    net_avg.clear();

#if ENABLE_GPU
    gpu_avg.clear();
    gpu_accu.clear();
#endif

    /*
    vector<unsigned char>().swap(dev_avg);
    vector<unsigned char>().swap(net_avg);
    vector<unsigned char>().swap(temp_avg);
    */
}

/**
 *	Pack the aggregated data in the packet
 */
void Packed_sample::Aggregation_sample(std::string ip, int mem, int cpu, int iot, int iow, int net) {
    this->ip_addr_s.assign(ip); //LDA IP
    /* Packing ip address. Position:0-3 */
    ip_v = parse_log(ip_addr_s);

    for (int i = 0; i < ip_v.size(); i++) { //prevents for ptr increasing a lot
        packed_buffer[i] = (unsigned char) stoi(ip_v[i]);
        packed_ptr++;
    }

    /* Packing memory. Position:4 */
    packed_buffer[packed_ptr] = (unsigned char) mem;
    packed_ptr++;

    /* Packing cpu. Position:5 */
    packed_buffer[packed_ptr] = (unsigned char) cpu;
    packed_ptr++;

    /* Packing io time. Position:6 */
    packed_buffer[packed_ptr] = (unsigned char) iot;
    packed_ptr++;

    /* Packing io writes. Position:7 */
    packed_buffer[packed_ptr] = (unsigned char) iow;
    packed_ptr++;

    /* Packing comm. Position:8 */
    packed_buffer[packed_ptr] = (unsigned char) net;
    packed_ptr++;

    /* set size*/
    this->sample_size = packed_ptr;

}

/**
 * Creates the generic packet that will be sent to the LDA/LDS
 * @param ip
 * @param counters
 * @param keys
 */
void Packed_sample::Aggregation_sample_generic(std::string ip, std::vector<unsigned long> counters, std::vector<string> keys) {
    this->ip_addr_s.assign(ip); //LDA IP
    /* Packing ip address. Position:0-3 */
    ip_v = parse_log(ip_addr_s);

    for (int i = 0; i < ip_v.size(); i++) {
        packed_buffer[i] = (unsigned char) stoi(ip_v[i]);
        packed_ptr++;
        packed_bytes++;
    }

    /* packing type of packet generic (only for generic)*/
    packed_buffer[packed_ptr] = 'g';
    packed_ptr++;
    packed_bytes++;

    /* Packing counters. Position:5 */
    int poskey = 0;
    for (const auto& counter : counters) {
        const string& key = keys[poskey++];
        array<char, 64> keybuf{}; 
        snprintf(keybuf.data(), keybuf.size(), "%s", key.c_str());

        std::memcpy(&packed_buffer[packed_ptr], keybuf.data(), 64);
        packed_ptr += 64;
        packed_bytes += 64;

        // Empaquetar el contador (8 bytes)
        std::array<unsigned char, 8> buffer{};  // Buffer automático de 8 bytes
        serialize_long(buffer.data(), counter);

        std::memcpy(&packed_buffer[packed_ptr], buffer.data(), buffer.size());
        packed_ptr += buffer.size();
        packed_bytes += buffer.size();
    }

    /* set size*/
    this->sample_size = packed_ptr;
}


/*
 *	Pack the features in the packed_buffer
 */
void Packed_sample::pack_features() {

    /* Packing ip address. Position:0-3 */
    ip_v = parse_log(ip_addr_s);

	for (int i = 0; i < ip_v.size(); i++) {
        packed_buffer[i] = (unsigned char) stoi(ip_v[i]);
        packed_ptr++;
    }

    /* Packing memory total. Position:4 */
    packed_buffer[packed_ptr] = memtotal;
    packed_ptr++;

    /* Packing number of cpus. Position:5 */
    packed_buffer[packed_ptr] = (unsigned char) n_cpu;
    packed_ptr++;

    /* Packing number of cores. Position:6 */
    packed_buffer[packed_ptr] = (unsigned char) n_cores;
    packed_ptr++;

    /* Packing number of devices. Position:7 */
    packed_buffer[packed_ptr] = (unsigned char) n_devices_io;
    packed_ptr++;

    /* Packing number of interfaces. Position:8 */
    packed_buffer[packed_ptr] = (unsigned char) n_interfaces;
    packed_ptr++;

    #if ENABLE_GPU
    /* Packing number of gpus. Position:X */
    packed_buffer[packed_ptr] = (unsigned char) n_gpus;
    packed_ptr++;
    #endif

    /* Packing number of samples. Position:9 - Antes 11 no sé si aquí tenia algo mas ¿GPU?*/
    packed_buffer[packed_ptr] = (unsigned char) n_samples;
    packed_ptr++;

    /* Guardar modo bitmap */
    //packed_buffer[packed_ptr] = (unsigned char)modo_bitmap;
    //packed_ptr++;

    /* Packing number of bytes of bitmap. Position:12 */
    //packed_buffer[packed_ptr] = (unsigned char)bytes_bitmap;
    //packed_ptr++;

    /* Guardar espacio para los bytes del bitmap */
    //pos_bitmap = packed_ptr;

    //cout << "Posicion del puntero: "<< packed_ptr << endl;
    //cout << "Numero de bytes es: "<< bytes_bitmap << endl;
    //packed_ptr = packed_ptr + bytes_bitmap;
    //cout << "Posicion del puntero despues: "<< packed_ptr << endl;
    //pos_sample = packed_ptr;
    //cout << "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++posicion en la que empiezan los datos es: "<< packed_ptr << endl;
}


/*
****************************************************************************************************************
* Pack sample into buffer to be sent.
* IN:
* @sample String to be packed in the buffer.
****************************************************************************************************************
*/
void Packed_sample::pack_sample_s(string sample){
    double dtmp = 0.0;
    std::vector<std::string> svec = split(sample, ' ');

    int sample_concat = 0;
    int stringptr = 0;

    /* Si se han empaquetado el numero de muestras que se quiere, se debe crear el mapa de bits y enviar los datos */
    if (samples_packed == n_samples) {
        /* Generar el bitmap y guardarlo en el packed buffer */
        to_bit_map();
        packed_ptr = 0;
        pack_features();
        sample_pt = 0;
        samples_packed = 0;
        time(&(this->time_sample));
    }
    if (packed_bytes == 0) {
        sample_pt = 0;
        sample_concat = 0;
    } else {
        sample_concat = 12; // No se si esto es util
    }
    stringptr = 2;

    /* Packing memory usage */
    packed_buffer[packed_ptr] = (unsigned char) stoi(svec[stringptr]);
    packed_ptr++;
    sample_concat++;
    packed_bytes++;
    stringptr += 3;

    /* Packing cpu idle */
    dtmp = stod(svec[stringptr]);
    packed_buffer[packed_ptr] = (unsigned char) dtmp;
    packed_ptr++;
    sample_concat++;
    packed_bytes++;
    stringptr++;

#if ENABLE_POWERCOLLECTOR
    /* Packing  energy usage for each cpu */
    for (int i = 0; i < n_cpu; i++) {
        dtmp = stod(svec[stringptr]);
        packed_buffer[packed_ptr] = (unsigned char) dtmp;
        packed_ptr++;
        packed_bytes++;
        sample_concat++;
        stringptr++;
    }
#endif

#if ENABLE_IOCOLLECTOR
    /* Packing  w(%) and tIO(%) for each device */
    for (int i = 0; i < n_devices_io; i++) {
        packed_buffer[packed_ptr] = (unsigned char) stoi(svec[stringptr]);
        packed_ptr++;
        packed_bytes++;
        sample_concat++;
        stringptr++;

        packed_buffer[packed_ptr] = (unsigned char) stoi(svec[stringptr]);
        packed_ptr++;
        packed_bytes++;
        sample_concat++;
        stringptr++;
    }
#endif

#if ENABLE_NETWORKCOLLECTOR
    /* Packing speed of the network and network usage (%) for each interface*/
    for (int i = 0; i < n_interfaces; i++) {
        packed_buffer[packed_ptr] = (unsigned char) stoi(svec[stringptr]);
        packed_ptr++;
        packed_bytes++;
        sample_concat++;
        stringptr++;

        packed_buffer[packed_ptr] = (unsigned char) stoi(svec[stringptr]);
        packed_ptr++;
        packed_bytes++;
        sample_concat++;
        stringptr++;
    }
#endif

#if ENABLE_GPU
    /* Packing gpu features: memory usage(%), gpu usage(%), temp(degrees Celsius) and powerUsage(Watts) for each gpu */
    for (int i = 0; i < n_gpus; i++) {
        packed_buffer[packed_ptr] = (unsigned char) stoi(svec[stringptr]);
        packed_ptr++;
        packed_bytes++;
        sample_concat++;
        stringptr++;

        packed_buffer[packed_ptr] = (unsigned char) stoi(svec[stringptr]);
        packed_ptr++;
        packed_bytes++;
        sample_concat++;
        stringptr++;

        packed_buffer[packed_ptr] = (unsigned char) stoi(svec[stringptr]);
        packed_ptr++;
        packed_bytes++;
        sample_concat++;
        stringptr++;

        packed_buffer[packed_ptr] = (unsigned char) stoi(svec[stringptr]);
        packed_ptr++;
        packed_bytes++;
        sample_concat++;
        stringptr++;
    }

#endif


    samples_packed++;
}



/*
****************************************************************************************************************
* Pack sample into buffer to be sent.
* IN:
* @sample String to be packed in the buffer.
****************************************************************************************************************
*/
void Packed_sample::pack_sample_s(string sample, double cache_miss, double cpu_stalled){//, std::vector<Core_dev> cores) {
    double dtmp = 0.0;
    std::vector<std::string> svec = split(sample, ' ');

    int sample_concat = 0;
    int stringptr = 0;

    /* Si se han empaquetado el numero de muestras que se quiere, se debe crear el mapa de bits y enviar los datos */
    if (samples_packed == n_samples) {
        /* Generar el bitmap y guardarlo en el packed buffer */
        to_bit_map();
        packed_ptr = 0;
        pack_features();
        sample_pt = 0;
        samples_packed = 0;
        time(&(this->time_sample));
    }
    if (packed_bytes == 0) {
        sample_pt = 0;
        sample_concat = 0;
    } else {
        sample_concat = 12;
    }
    stringptr = 2;

    /* Packing memory usage */
    packed_buffer[packed_ptr] = (unsigned char) stoi(svec[stringptr]);
    packed_ptr++;
    sample_concat++;
    packed_bytes++;
    stringptr += 3;

    /* Packing cpu idle */
    dtmp = stod(svec[stringptr]);
    packed_buffer[packed_ptr] = (unsigned char) dtmp;
    packed_ptr++;
    sample_concat++;
    packed_bytes++;
    stringptr++;

    /* Packing  energy usage for each cpu */
    /*for (int i = 0; i < n_cpu; i++) {
        dtmp = stod(svec[stringptr]);
        packed_buffer[packed_ptr] = (unsigned char) dtmp;
        packed_ptr++;
        packed_bytes++;
        sample_concat++;
        stringptr++;
    }*/

    /* Packing  w(%) and tIO(%) for each device */
    for (int i = 0; i < n_devices_io; i++) {
        packed_buffer[packed_ptr] = (unsigned char) stoi(svec[stringptr]);
        packed_ptr++;
        packed_bytes++;
        sample_concat++;
        stringptr++;

        packed_buffer[packed_ptr] = (unsigned char) stoi(svec[stringptr]);
        packed_ptr++;
        packed_bytes++;
        sample_concat++;
        stringptr++;
    }

    /* Packing speed of the network and network usage (%) for each interface*/
    for (int i = 0; i < n_interfaces; i++) {
        packed_buffer[packed_ptr] = (unsigned char) stoi(svec[stringptr]);
        packed_ptr++;
        packed_bytes++;
        sample_concat++;
        stringptr++;

        packed_buffer[packed_ptr] = (unsigned char) stoi(svec[stringptr]);
        packed_ptr++;
        packed_bytes++;
        sample_concat++;
        stringptr++;
    }

    /*cout << "Packed_buffer" << endl;
    for(int i = 0; i < packeqd_ptr; i++){
    	printf("%d ",packed_buffer[i]);
    }
	cout << endl;*/

    /*PACK CACHE INFORMATION*/
    int r = (int)cache_miss;
    packed_buffer[packed_ptr] = (unsigned char) r;
    packed_ptr++;
    packed_bytes++;
    sample_concat++;
    stringptr++;

    /*PACK cpu stalled  INFORMATION*/
    int s = (int)cpu_stalled;
    packed_buffer[packed_ptr] = (unsigned char) s;
    packed_ptr++;
    packed_bytes++;
    sample_concat++;
    stringptr++;


    /*PACK core information*/
    /*packed_buffer[packed_ptr] = (unsigned char) n_siblings;
    packed_ptr++;
    packed_bytes++;
    sample_concat++;
    stringptr++;
    for(int core = 0; core < cores.size(); ++core) {
        int l = (int)cores[core].load;
        packed_buffer[packed_ptr] = (unsigned char)l;
        packed_ptr++;
        packed_bytes++;
        sample_concat++;
        stringptr++;
    }
    n_siblings = (int)cores.size();
	*/

    samples_packed++;
}


/***************** Packed_sample with slurm job **************/ 
void Packed_sample::pack_sample_prometheus(string sample, std::string jobname) {
	double dtmp = 0.0;
    std::vector<std::string> svec = split(sample, ' ');

    int sample_concat = 0;
    int stringptr = 0;
    packed_ptr = 0;
    pack_features();
    stringptr = 2;

    /* Packing memory usage */
    packed_buffer[packed_ptr] = (unsigned char) stoi(svec[stringptr]);
    packed_ptr++;
    sample_concat++;
    packed_bytes++;
    stringptr += 3;

    /* Packing cpu idle */
    dtmp = stod(svec[stringptr]);
    packed_buffer[packed_ptr] = (unsigned char) dtmp;
    packed_ptr++;
    sample_concat++;
    packed_bytes++;
    stringptr++;

    /* Packing  energy usage for each cpu */
   	/* for (int i = 0; i < n_cpu; i++) {
        dtmp = stod(svec[stringptr]);
        packed_buffer[packed_ptr] = (unsigned char) dtmp;
        packed_ptr++;
        packed_bytes++;
        sample_concat++;
        stringptr++;
    }*/

    /* Packing  w(%) and tIO(%) for each device */
    for (int i = 0; i < n_devices_io; i++) {
        packed_buffer[packed_ptr] = (unsigned char) stoi(svec[stringptr]);
        packed_ptr++;
        packed_bytes++;
        sample_concat++;
        stringptr++;

        packed_buffer[packed_ptr] = (unsigned char) stoi(svec[stringptr]);
        packed_ptr++;
        packed_bytes++;
        sample_concat++;
        stringptr++;
    }

    /* Packing speed of the network and network usage (%) for each interface*/
    for (int i = 0; i < n_interfaces; i++) {
        packed_buffer[packed_ptr] = (unsigned char) stoi(svec[stringptr]);
        packed_ptr++;
        packed_bytes++;
        sample_concat++;
        stringptr++;

        packed_buffer[packed_ptr] = (unsigned char) stoi(svec[stringptr]);
        packed_ptr++;
        packed_bytes++;
        sample_concat++;
        stringptr++;
    }


    /*cout << "Packed_buffer" << endl;
    for(int i = 0; i < packed_ptr; i++){
        printf("%d ",packed_buffer[i]);
    }
	cout << endl;*/

    /*PACK CACHE INFORMATION - DUMMY*/
    int r = 0;
    packed_buffer[packed_ptr] = (unsigned char)r;
    packed_ptr++;
    packed_bytes++;
    sample_concat++;
    stringptr++;

    /*PACK cpu stalled  INFORMATION*/
    int s = 0;
    packed_buffer[packed_ptr] = (unsigned char)s;
    packed_ptr++;
    packed_bytes++;
    sample_concat++;
    stringptr++;

	/* JOBNAME for PROMETHEUS AGGREGATIONS*/
	for (char c : jobname){
		packed_buffer[packed_ptr] = (unsigned char)c;
		packed_ptr++;
		packed_bytes++;
		sample_concat++;
	}

	samples_packed++;
}


/***************** Packed_sample:  **************/
void serialize_int(unsigned char *buffer, unsigned int value)
{
  buffer[0] = value >> 24;
  buffer[1] = value >> 16;
  buffer[2] = value >> 8;
  buffer[3] = value;
  //return buffer + 4;
}

void serialize_long(unsigned char *buffer, unsigned long value)
{
  buffer[0] = value >> 56;
  buffer[1] = value >> 48;
  buffer[2] = value >> 40;
  buffer[3] = value >> 32;
  buffer[4] = value >> 24;
  buffer[5] = value >> 16;
  buffer[6] = value >> 8;
  buffer[7] = value;
}

int deserialize_int(unsigned char *buffer)
{
  return (uint8_t(buffer[0]) << 24) | (uint8_t(buffer[1]) << 16) | (uint8_t(buffer[2]) << 8) | uint8_t(buffer[3]);
}


/*
 * int n: number of values
 * Values are double
 * Delimiter = ';'
 */
void Packed_sample::pack_sample_generic(string sample) {
  	packed_ptr = 0;
  	for(int i = 0; i < 12; i++) {
    	packed_buffer[packed_ptr] = (unsigned char)'g';
    	packed_ptr++;
    	packed_bytes++;
  	}

  	packed_ptr = 12; //start writing in 12
  	std::vector<std::string> svec = split(sample, ';');

  	//PACKET TYPE GENERIC: string (64) and value (double)
  	packed_buffer[packed_ptr] = (unsigned char) 'g';
  	packed_ptr++;
  	packed_bytes++;

	for (int i = 0; i < svec.size(); i+=2){
        std::array<char, 64> keybuf{};  // Buffer automático de 64 bytes
        std::snprintf(keybuf.data(), keybuf.size(), "%s", svec[i].c_str());

        std::memcpy(&packed_buffer[packed_ptr], keybuf.data(), 64);
        packed_ptr += 64;
        packed_bytes += 64;

        // Empaquetar el valor como long (8 bytes)
        std::array<unsigned char, 8> buffer{};  // Buffer automático de 8 bytes
        unsigned long value = std::stoul(svec[i + 1]);  // Convertir la cadena a unsigned long
        serialize_long(buffer.data(), value);

        std::memcpy(&packed_buffer[packed_ptr], buffer.data(), buffer.size());
        packed_ptr += buffer.size();
        packed_bytes += buffer.size();
  	}

  	//print
  	/*cout << packed_buffer[0] << " ";
  	for (int i = 1; i < packed_ptr; i++)
    	cout << (int)packed_buffer[i] << " ";
  	cout << endl;*/

  	samples_packed++;
  	sample_size = packed_ptr; //size of packet
}


/*
****************************************************************************************************************
* Splits the log using dots, spaces and tab characters.
* IN:
* @string	String representing the log content.
* RETURNS:
* Vector of strings with the splitted contents.
****************************************************************************************************************
*/
vector <string> Packed_sample::split_log(string str){
    if(str.length()==0){
        vector<string> internal;
    }

    vector<string> internal;
    std::stringstream ss(str); // Turn the string into a stream.
    string tok;
    while(getline(ss, tok, '.')) {
        std::stringstream ss2(tok);
        string tok2;

        while(getline(ss2,tok2,' ')){
            std::stringstream ss3(tok2);
            string tok3;

            while(getline(ss3,tok3,'\t')){
                if(tok != ""){
                    internal.push_back(tok3);
                }
            }
        }
    }

    return internal;
}

/*
****************************************************************************************************************
* Parse sample to include on the buffer
* IN:
* @sample	String including sample to be parsed.
* RETURNS:
* Vector of strings including the parsed contents.
****************************************************************************************************************
*/
vector <string> Packed_sample::parse_log(string sample){
    vector<string> log_vector;
    stringstream sdouble;
    string sinsert;

    /* Getting the ip of the sample */
    vector<string> sample_split = split(sample, ' ');
    string ip_ = sample_split[0];
    log_vector = split_log(ip_);

    for(int i = 1; i < sample_split.size(); i++ ){
        sdouble.str("");
        sdouble << stoi(sample_split[i]);
        log_vector.push_back(sdouble.str());
    }

    return log_vector;
}


void Packed_sample::calculate_sample_size(){
	/*
		n_cpu = Number of cpus
		mem_cpu = 
		n_interfaces = number of interfaces
		n_devices_io = number of I/O devices
		cache
	    stalled
	    [core_load*n_core]
	    [slurm_jobname]
	*/
	
	this->sample_size = 2 + n_cpu + 2 * n_devices_io + 2 * n_interfaces + 1 + 1 + 1 + 10;
	// + n_siblings; //cache and stalled //slurm_jobname (10 chars)
}

int Packed_sample::calculate_bit_maps_bytes(){
	/**
		Datos que se guardan en el bitmap son:
		- uso memoria
		- uso cpu
		- por cada cpu temperatura
		- por cada io device %
		- por cada interfaz %
		- por cada coretemp %

		n_cpu = Number of cpus
		mem_cpu =
		n_interfaces = number of interfaces
		n_devices_io = number of I/O devices
		n_core_temps = number of coretemps
	 */
	int total_bits = 2 + 2 + 2*(n_cpu) + 2*(n_devices_io) + 2*(n_interfaces) ; //Total bits used: mem, cpu, temp for each cpu

    //bytes_bitmap = n_cpu + (mem_cpu + n_interfaces + n_devices_io + n_core_temps) * 2 + (n_gpus * 4);
    //bytes_bitmap = ceil((double)bytes_bitmap / 8);
    bytes_bitmap = ceil((double)total_bits / 8);

    return 0;        
}


void Packed_sample::to_bit_map(){
    /* Inicializar las variables */
    //bit_map[bytes_bitmap];

    memory_avg = 0;
    cpu_avg = 0;
    byte_bit_map = 0;
    bit_map_ptr = 0;

    int packed_buffer_ptr = 0;

    for(int i = 0; i < n_devices_io; i++){
        dev_avg[i] = 0;
    }

	for(int i = 0; i < n_interfaces; i++){
        net_avg[i] = 0;
    }

    packed_buffer_ptr = pos_sample;

    /* Media de los valores del packed buffer */
    /* Primero se suman los valores acumulados en el packed buffer */
    for(int i = 0; i < n_samples;i++){
        //packed_buffer_ptr = 11;
        /* Sumar los valores de la memoria */
        memory_avg += packed_buffer[packed_buffer_ptr];
        packed_buffer_ptr++;

        /* Sumar los valores de cpu */
        cpu_avg += packed_buffer[packed_buffer_ptr];
        packed_buffer_ptr++;

		/* Sumar los valores del uso de energia en las cpu */
        for(int j = 0;j < n_cpu; j++){
	    	pw_cpu[j] += packed_buffer[packed_buffer_ptr];
           	packed_buffer_ptr++;
        }

        /* Sumar los valores de los dispositivos */
        for(int j = 0; j < n_devices_io; j++){
            packed_buffer_ptr++;
            dev_avg[j] += packed_buffer[packed_buffer_ptr];
            packed_buffer_ptr++;
        }

        /* Sumar los valores de las interfaces */
        for(int j = 0; j < n_interfaces; j++){
            packed_buffer_ptr++;
            net_avg[j] += packed_buffer[packed_buffer_ptr];
            packed_buffer_ptr++;
        }
    }

    /* Ahora se realiza la media por valor */
    /* Media de la memoria */
    memory_avg = memory_avg / n_samples;
    codificar_cuartil(memory_avg);

    /* Media de la cpu */
    cpu_avg = cpu_avg / n_samples;
    codificar_cuartil(cpu_avg);

    /* Media de energia por cpu */
    for(int i = 0; i < n_devices_io;i++){
        pw_cpu[i] = pw_cpu[i] / n_samples;
        codificar_cuartil(pw_cpu[i]);
    }

    /* Media por dispositivo */
    for(int i = 0; i < n_devices_io;i++){
        dev_avg[i] = dev_avg[i] / n_samples;
        codificar_cuartil(dev_avg[i]);
    }

    /* Media por interfaz */
    for(int i = 0; i < n_interfaces; i++){
        net_avg[i] = net_avg[i] / n_samples;
        codificar_cuartil(net_avg[i]);
    }

    /***
        Las posiciones del packed_buffer son:
        0-3: direccion ip
        4: memoria en GBytes
        5: numero de cpus
        6: numero de cores
        7: numero de dispositivos
        8: numero de interfaces
        9: numero de muestras que se envian empaquetadas
        10: uso de la memoria en el tiempo de muestra
        11: cpu idle en el tiempo de muestra
        El resto de posiciones depende del numero de dispositivos, interfaces y coretemps
    1 bytes por numero de CPUs
        2 bytes por numero de dispositivos
        2 bytes por numero de interfaces
        2 bytes por numero de coretemps
    ***/
}

void Packed_sample::codificar_cuartil(int cuartil){
    unsigned char * tmp_p = bit_map.get();
    if(cuartil < 25){
        tmp_p[byte_bit_map] = tmp_p[byte_bit_map] | (0 << 6 - bit_map_ptr);
    }else if(cuartil >= 25 && cuartil < 50){
        tmp_p[byte_bit_map] = tmp_p[byte_bit_map] | (1 << 6 - bit_map_ptr);
    }else if(cuartil >=50 && cuartil < 75){
        tmp_p[byte_bit_map] = tmp_p[byte_bit_map] | (2 << 6 - bit_map_ptr);
    }else{
        tmp_p[byte_bit_map] = tmp_p[byte_bit_map] | (3 << 6 - bit_map_ptr);
    }

    bit_map_ptr += 2;
    if(bit_map_ptr > 6){
        byte_bit_map++;
        bit_map_ptr = 0;
    }
}


/* IMPORTED FROM NETWORK DATA */
int convert_char_to_short_int(unsigned char * buffer, unsigned short int *number){
    *number = ((int)buffer[0]<<8) | (int) buffer[1];
    return 0;
}

int convert_int_to_char(unsigned char * buffer, unsigned int number){
    int size = sizeof(unsigned int);
    for(int i= 1; i <= size; i++ ){
        buffer[i - 1] = (unsigned char)(number >> (8 * (size - i))) & (0xff);
    }
    return size;
}

int convert_char_to_int(unsigned char * buffer){
    int size = sizeof(unsigned int);
    unsigned int number = 0;
    for(int i= 1; i <= size; i++){
        unsigned int tmp = (unsigned int) buffer[i-1];
        number = (tmp << ((size-i)*8)) | number ;
    }
    return number;
}

/**
 * It converts a short value to unsigned char
 * */
int convert_short_int_to_char(char * buffer, unsigned short int number){
    buffer[0] = (unsigned char)(number>>8);
    buffer[1]  = number & 0xff;
    return 0;
}

int convert_time_to_char(unsigned char * buffer, time_t time){
    int size = sizeof(time_t);
    for(int i= 1; i <= size; i++ ){
        buffer[i - 1] = (unsigned char)(time >> (8 * (size - i))) & (0xff);
    }
    return size;
}


int calculate_conf_packet(Hw_conf hw_conf){
  //return	CONF_PACKET_SIZE + (hw_conf.n_gpu * 3);
  return 0;
}


/**
	Create the configuration packet
	@param[] hw_conf:
	@param[] conf_packed:
*/
int create_conf_packet(Hw_conf hw_conf, char * conf_packed){
    unsigned char mode = 0; //This variable indicate if mode is configuration (mode = 0), or information packet (mode != 0)

    calculate_conf_packet(hw_conf);

    int cpp = 0; //conf packet pointer
    conf_packed[0] = mode;
    conf_packed[1] = (unsigned char) hw_conf.n_cpu;
    conf_packed[2] = (unsigned char) hw_conf.n_cores;
    convert_short_int_to_char(&conf_packed[3],(unsigned short int) hw_conf.mem_total);
    conf_packed[5] = (unsigned char) hw_conf.n_devices_io;
    conf_packed[6] = (unsigned char) hw_conf.n_interfaces;

    /* Number of GPUs */
    cpp = 6;
    cpp ++;
    conf_packed[cpp] = hw_conf.hostname.length();
    cpp++;

    memcpy(&conf_packed[cpp],hw_conf.hostname.c_str(),hw_conf.hostname.length());
    cpp += hw_conf.hostname.length();

    /* Obtener modo bitmap */
    //cpp++;
    //conf_packed[cpp] = (unsigned char)hw_conf.modo_bitmap;
    //	printf("------------------------Modo del bitmap es: %d --------------\n",conf_packed[cpp]);
    //cout << "---------------------Modo del bitmap es: " << conf_packed[cpp] << endl;
    //cpp++;

    return cpp;
}

int create_sample_packet(Packed_sample &info, unsigned char * buffer){
    int contador = 0;
    buffer[contador] = (unsigned char)info.n_samples;
    contador++;

    //Include bitmap mode
    //contador+= convert_int_to_char(&buffer[contador],info.modo_bitmap);
    buffer[contador] = (unsigned char)info.modo_bitmap;
    contador++;

    if(info.modo_bitmap == 0){ //No se envia el bitmap pero si el sample
        //Include interval
        contador+= convert_int_to_char(&buffer[contador],info.interval);
        //Include time
        contador+= convert_time_to_char(&buffer[contador],info.time_sample);

        /* Ahora los datos no empiezan en esa posición tiene que ser calculada */
        //Empezamos en el 12 que es donde empiezan los datos de las muestras
        memcpy((unsigned char *)&buffer[contador], (unsigned char *)&info.packed_buffer[10], (info.n_samples * info.sample_size));

    }else if(info.modo_bitmap == 1){ //No se envia el sample pero si el bitmap
        //Include bytes of bitmap
        //contador+= convert_int_to_char(&buffer[contador],info.);
        buffer[contador] = (unsigned char)info.bytes_bitmap;
        contador++;

        //Include bitmap
        memcpy((unsigned char *)&buffer[contador], (unsigned char *)&info.bit_map[0], info.bytes_bitmap);
        contador+= (unsigned char)info.bytes_bitmap;

    }else if(info.modo_bitmap == 2){ //Se envia el bitmap y el sample
        //Include bytes of bitmap
        //contador+= convert_int_to_char(&buffer[contador],info.);
        buffer[contador] = (unsigned char)info.bytes_bitmap;
        contador++;

        //Include bitmap
        memcpy((unsigned char *)&buffer[contador], (unsigned char *)&info.bit_map[0], info.bytes_bitmap);
        contador+= (unsigned char)info.bytes_bitmap;

        //Include interval
        contador+= convert_int_to_char(&buffer[contador],info.interval);
        //Include time
        contador+= convert_time_to_char(&buffer[contador],info.time_sample);

        /* Se copia el sample en el buffer */
        //Empezamos en el 12 que es donde empiezan los datos de las muestras

        memcpy((unsigned char *)&buffer[contador], (unsigned char *)&info.packed_buffer[12], (info.n_samples * info.sample_size));
    }

    return contador+info.sample_size;//0;
}

int create_generic_packet(Packed_sample &info, unsigned char * buffer){
    int contador = 0;
    memcpy((unsigned char *)&buffer[contador], (unsigned char *)&info.packed_buffer[0], (info.n_samples * info.sample_size));
    return contador+info.sample_size;//0;
}


/**
 * This function keeps sending bytes from a buffer until the specified amount
 * is actually sent. It is designed to overcome the problem of send function
 * being able to return having sent only a portion of the specified bytes.
 * @param socket_descriptor
 * @param buf
 * @param n specifies the amount of bytes to send from the first positions of the buffer
 * @param out_addr
 * @return On success n is returned. On error -1 is returned.
 */
int sendn(int socket_descriptor, void *buf, int n, struct sockaddr_in *out_addr) {
    char *buf_ptr;
    int bsent; //actual bytes sent in last send call (may be lower than value specified to send)
    int bleft = n; //bytes left to be sent

    buf_ptr =(char *) buf;
    bsent = sendto(socket_descriptor, buf_ptr, n, 0, (struct sockaddr *) out_addr, sizeof(struct sockaddr_in));
    return n; //success
}

/**
 * This function keeps receiving bytes from a buffer until the specified amount
 * is actually received. It is designed to overcome the problem of recv function
 * being able to return having received only a portion of the specified bytes.
 * @param socket_descriptor
 * @param buf
 * @param n specifies the amount of bytes to receive in the buffer
 * @return On success n is returned. On premature close/shutdown on peer 0 is returned. On error -1 is returned.
 */
int recvn(int socket_descriptor, void *buf, int n) {
    char *buf_ptr;
    int bread; //actual bytes received in las recv call (may be lower than value specified to recv)
    int bleft; //bytes left to be received

    buf_ptr = (char *) buf;
    bleft = n;

    do {
        bread = recv(socket_descriptor, buf_ptr, bleft, 0);
        bleft -= bread;
        buf_ptr += bread; //pointer arithmetic to manage offset
    } while ( (bread > 0) && ( bleft != 0) );

    if ( bread <= 0) { //recv went wrong
        if ( bread == 0){ //peer socket closed/shutdown
          return (0);
        }
        //other error during last recv
        return (-1);
    }
    //else bleft == 0 // all bytes sent
    return n;
}

