#ifndef GPU_INFO_H
#define GPU_INFO_H

#include <string>
#include <vector>
#include "daemon_monitor.hpp"
#include <cuda.h>
#include <cuda_runtime_api.h>
#include <nvml.h>
//extern void *nvml_lib;
using namespace std;

#define CUDA_COMPATIBLE 1
#define CUDA_NO_COMPATIBLE -1

#define CUDA_CALL(function, ...)  {   cudaError_t status = function(__VA_ARGS__);   anyCheck(status == cudaSuccess, cudaGetErrorString(status), #function, __FILE__, __LINE__);} 

typedef CUresult CUDAAPI (*cuInit_pt)(unsigned int Flags);
typedef CUresult CUDAAPI (*cuDeviceGetCount_pt)(int *count);
typedef CUresult CUDAAPI (*cuDeviceComputeCapability_pt)(int *major, int *minor, CUdevice dev);
typedef cudaError_t CUDAAPI (*cudaSetDevice_pt)(int device);
typedef CUresult CUDAAPI (*cudaMemGetInfo_pt)(size_t * free, size_t * total);
typedef CUresult CUDAAPI (*cudaGetDeviceProperties_pt)(struct cudaDeviceProp * prop,int total);
typedef CUresult CUDAAPI (*cuDeviceGetName_pt)(char * name, int len, CUdevice dev);
typedef CUresult CUDAAPI (*cuDeviceGet_pt)(CUdevice * dev, int ordinal);
typedef CUresult CUDAAPI (*cuMemGetInfo_pt)(size_t * free, size_t * total);
typedef nvmlReturn_t CUDAAPI (*nvmlDeviceGetUtilizationRates_pt)(nvmlDevice_t device,nvmlUtilization_t * utilization);
typedef nvmlReturn_t CUDAAPI (*nvmlDeviceGetHandleByIndex_pt)(unsigned int index,nvmlDevice_t *device);
typedef nvmlReturn_t CUDAAPI (*nvmlInit_pt)(void);
typedef nvmlReturn_t CUDAAPI (*nvmlDeviceGetTemperature_pt)(nvmlDevice_t device, nvmlTemperatureSensors_t sensorType, unsigned int * temp);
typedef nvmlReturn_t CUDAAPI (*nvmlDeviceGetPowerUsage_pt)(nvmlDevice_t device, unsigned int * power);
typedef nvmlReturn_t CUDAAPI (*nvmlDeviceGetMemoryInfo_pt)(nvmlDevice_t device, nvmlMemory_t * memory);

/* Struct to include data of a GPU  */
typedef struct Gpu_dev{

	int cudacomp;
        string model_name;
        size_t memTotal;
        size_t memFree;
	size_t memUsage;
	unsigned int gpuUsage;
	unsigned int temperature;
	unsigned int powerUsage;
        int dev_id;
	unsigned char capability;

}Gpu_dev;

/* Struct used to accumulate data of GPUs */
typedef struct Gpu_accu{
	
	unsigned char memUsage;
	unsigned char gpuUsage;
	unsigned char temperature;
	unsigned char powerUsage;	

}Gpu_accu;

void get_n_gpu_devices();


void anyCheck(bool is_ok, const char *description, const char *function, const char *file, int line);

int compatible_gpu(void* &cuLib);

void (*getProcAddress(void * lib, const char *name))(void);

int freeLibrary(void *lib);


/**
        Reads gpu status and stores info.

        @param [in,out] gpus vector of Gpu_dev where the stats will be saved.
        @param [in] cuLib CUDA library
        @param [in] nvmlLib NVidia Management Library
        
*/
int read_gpu_stats(vector<Gpu_dev> & gpus,void* cuLib, void* nvmlLib);


/**
	
*/
int construct_capability(unsigned char & capability, int major, int minor);


/**
        Reads number of gpu devices NVIDIA and store information.

        @param [in, out] gpus vector of Gpu_dev in the node
        @param [in, out] dev_comp variable to know if there are GPU on the node
        @param [in, out] cuLib variable to save CUDA Library
	@param [in, out] nvmlLib variable to save NVidia Management Library
        
*/
int read_n_gpu(vector<Gpu_dev> & gpus, int & n_gpu,int & dev_comp, void** cuLib, void** nvmlLib);

#endif
