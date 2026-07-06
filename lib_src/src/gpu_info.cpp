#ifndef GPU_INFO
#define GPU_INFO

#include "../include/gpu_info.hpp"
#include <cuda.h>
#include <cuda_runtime_api.h>
#include <cuda_runtime.h>
#include <dlfcn.h>
//#include <nvml.h>
#include <string>

#include "daemon_monitor.hpp"
#include "common.hpp"
#include "system_features.hpp"
#include <vector>
#include <chrono>



using namespace std; 

#define DEVNAMELENGTH 128 

void * loadCudaLibrary() {

   	return dlopen ("libcuda.so", RTLD_NOW);

}

void * loadNVMLLibrary(){
	return dlopen("libnvidia-ml.so",RTLD_NOW);
}

void (*getProcAddress(void * lib, const char *name))(void){

    	return (void (*)(void)) dlsym(lib,(const char *)name);

}

int freeLibrary(void *lib){
   
        return dlclose(lib);

}

void anyCheck(bool is_ok, const char *description, const char *function, const char *file, int line) {
    	if (!is_ok) {
        	fprintf(stderr,"Error: %s in %s at %s:%d\n", description, function, file, line);
        	exit(EXIT_FAILURE);
    	}
}

int compatible_gpu(void* &cuLib){
    	if ((cuLib = loadCudaLibrary()) == NULL){

            	return CUDA_NO_COMPATIBLE;
    	}

    	return CUDA_COMPATIBLE;      

}


int read_gpu_stats(vector<Gpu_dev> & gpus, void* cuLib, void* nvmlLib){

	using namespace std::chrono;
	using clk = high_resolution_clock;

	clk::time_point t_1;
	clk::time_point	t_2;


	clk::duration difft;

	t_1 = clk::now();
	
	nvmlDeviceGetUtilizationRates_pt mynvmlDeviceGetUtilizationRates = NULL;	
	nvmlDeviceGetTemperature_pt mynnvmlDeviceGetTemperature = NULL;
	nvmlDeviceGetPowerUsage_pt mynvmlDeviceGetPowerUsage = NULL;
	nvmlDeviceGetTemperature_pt mynvmlDeviceGetTemperature = NULL;
	nvmlDeviceGetMemoryInfo_pt mynvmlDeviceGetMemoryInfo = NULL;
//	cudaSetDevice_pt my_cudaSetDevice = NULL;
    	
	if ((mynvmlDeviceGetUtilizationRates = (nvmlDeviceGetUtilizationRates_pt) getProcAddress(nvmlLib, "nvmlDeviceGetUtilizationRates")) == NULL){
		cout << "fallo get utilization" << endl;
            	return 1; // sth is wrong with the library
        }
	
	#ifdef DEBUG_GPU
		cout << "my nvmldevice cargado" << endl;
	#endif 

//	cuMemGetInfo_pt mycuMemGetInfo = NULL;	
	nvmlDeviceGetHandleByIndex_pt mynvmlDeviceGetHandleByIndex = NULL;
	
	if ((mynvmlDeviceGetHandleByIndex = (nvmlDeviceGetHandleByIndex_pt) getProcAddress(nvmlLib, "nvmlDeviceGetHandleByIndex")) == NULL){
            	return 1; // sth is wrong with the library
        }

//	if ((mycuMemGetInfo = (cuMemGetInfo_pt) getProcAddress(cuLib, "cuMemGetInfo")) == NULL){
  //          	return 1; // sth is wrong with the library
    //    }

	if ((mynvmlDeviceGetTemperature = (nvmlDeviceGetTemperature_pt) getProcAddress(nvmlLib, "nvmlDeviceGetTemperature")) == NULL){
            	return 1; // sth is wrong with the library
        }

    	if ((mynvmlDeviceGetPowerUsage = (nvmlDeviceGetPowerUsage_pt) getProcAddress(nvmlLib, "nvmlDeviceGetPowerUsage")) == NULL){
            	return 1; // sth is wrong with the library
        }
	

	if ((mynvmlDeviceGetMemoryInfo = (nvmlDeviceGetMemoryInfo_pt) getProcAddress(nvmlLib, "nvmlDeviceGetMemoryInfo")) == NULL){
            	return 1; // sth is wrong with the library
        }

	t_2 = clk::now();

	difft = (t_2 - t_1);

	CUresult curesult;

    	t_1 = clk::now();

	/* GPU devices CUDA compatible */                     
    	for(int i = 0; i < gpus.size(); i++){
	
		size_t gpu_mem_used = 0;
    		double gpu_percentage = 0.0;

		unsigned int gpu_usage = 0;

		nvmlUtilization_t _utilization;
		
		#ifdef DEBUG_GPU
			cout << "antes de coger los rates"<< endl;
		#endif
		
		nvmlReturn_t error;
		nvmlDevice_t gpu_id = 0;

		unsigned int index = 0;
		nvmlDevice_t device;


		error = mynvmlDeviceGetHandleByIndex(i, &device);
       		
		if(error != NVML_SUCCESS){
			cout << "eror get handle" << endl;
	
			log_concat(gpus[i].memUsage,2);
	
        		log_concat(gpus[i].gpuUsage,2);

			log_concat(gpus[i].temperature,2);
			log_concat(gpus[i].powerUsage,2);
		
			return EGPU;
		}if(error == NVML_ERROR_UNINITIALIZED){
			cout << "error uninitialized" << endl;
		}else if(error ==NVML_ERROR_INVALID_ARGUMENT){
			cout << "invalid argument" << endl;
		}else if(error == NVML_ERROR_NOT_FOUND){
			cout << "not found" << endl;
		}else if(error == NVML_ERROR_INSUFFICIENT_POWER){
			cout << "insuficient power" << endl;
		}

		/* Getting utilization rates */
		error = mynvmlDeviceGetUtilizationRates(device, &_utilization);
	
		if(error !=NVML_SUCCESS){
			cout << "error getutilization" << endl;
		/*If an error occurred while getting usage, all is 0*/
			gpus[i].gpuUsage = 0;
			gpus[i].memUsage = 0;
		}else{
			gpus[i].gpuUsage = _utilization.gpu;
			gpus[i].memUsage = _utilization.memory;				
		}
	
		
		
	
		gpu_mem_used = 0;

		nvmlMemory_t memoryinfo;

		error = mynvmlDeviceGetMemoryInfo(device, &memoryinfo);

		if(error !=NVML_SUCCESS){
			cerr << "[ERROR][GPU] Could not get memory of device " << i << endl;
        	gpus[i].memTotal = 0.0;
        	gpus[i].memFree = 0.0;
		}else{
			gpus[i].memTotal = memoryinfo.total / ((float)(1024*1024)); //Get Megabytes
        	gpus[i].memFree =  memoryinfo.free / ((float)(1024*1024)); //Get Megabytes
		}


		gpu_mem_used = gpus[i].memTotal - gpus[i].memFree;
	
		#ifdef DEBUG_GPU
			cout << "uso de la memoria es: "<< gpu_mem_used  << endl;
		#endif
	
		gpus[i].cudacomp = CUDA_COMPATIBLE;
        gpu_percentage = gpus[i].memTotal  ? (float) gpu_mem_used /(float) gpus[i].memTotal : 0.0;
        gpu_percentage = gpu_percentage * 100;

		error = mynvmlDeviceGetTemperature(device, NVML_TEMPERATURE_GPU, &gpus[i].temperature);  
		
		if(error !=NVML_SUCCESS){
			cerr << "[ERROR][GPU] Could not get temperature of device " << i << endl;
			gpus[i].temperature = 0;
		}
		mynvmlDeviceGetPowerUsage(device, &gpus[i].powerUsage);       
		if(error !=NVML_SUCCESS){
			cerr << "[ERROR][GPU] Could not get power of device " << i << endl;
			gpus[i].powerUsage = 0;
		}else{
			gpus[i].powerUsage = gpus[i].powerUsage / 1000.0;
		}

		
	
		#ifdef DEBUG_GPU
			cout << "temperatura de la gpu: " << gpus[i].temperature << endl;
		#endif	

	 	/* Concatenamos la memoria total y el pocentaje de la memoria usada */

		#ifdef DEBUG_GPU
			cout <<"se concatena el uso de la memoria: "<< gpus[i].memUsage << endl;
		#endif 

		log_concat(gpus[i].memUsage,2);
	
        log_concat(gpus[i].gpuUsage,2);

		log_concat(gpus[i].temperature,2);
	
		log_concat(gpus[i].powerUsage,2);
    }

	t_2 = clk::now();

	difft = (t_2 - t_1);

	return 0;
}

/**  
	Puts the major number on the top of a char and the minor number on the bottom
	
	@param[in,out] capability
	@param[in] major Major number to be compute and save in the char
	@param[in] minor Minor number to be compute and save in the char
	@return int that especify the error

*/
int construct_capability(unsigned char & capability, int major, int minor){
	capability = 0;

	capability = capability | (major << 4);
	capability = capability | (minor);

	return EOK;
}


/**
        Reads number of gpu devices NVIDIA and store information.

        @param [in,out] cpus vector of Cpu_dev in the node
        @param [in,out] n_cpu number of cpus in the node
        @param [in, out] n_cores number of cores in the node
        
*/

int read_n_gpu(vector<Gpu_dev> & gpus, int & n_gpu, int & dev_comp, void** cuLib, void** nvmlLib){
    	struct cudaDeviceProp deviceProp;
    
    	cuInit_pt my_cuInit = NULL;
    	cuDeviceGetCount_pt my_cuDeviceGetCount = NULL;
    	cuDeviceComputeCapability_pt my_cuDeviceComputeCapability = NULL;
    	cuDeviceGet_pt my_cuDeviceGet = NULL;
    	cuDeviceGetName_pt my_cuDeviceGetName = NULL;
    	nvmlInit_pt my_nvmlInit = NULL;
	
	nvmlDeviceGetMemoryInfo_pt mynvmlDeviceGetMemoryInfo = NULL;

	CUdevice dev_;
	n_gpu = 0;
    	dev_comp = compatible_gpu(*cuLib);
	
	if(cuLib == NULL){
		cout << "biblioteca no se ha cargado bien"<< endl;
	}
	
	*nvmlLib = loadNVMLLibrary();
	
	if(nvmlLib == NULL){
		cout << "biblioteca nvml no se ha cargado bien" << endl;
	}
	
    	if(dev_comp == CUDA_NO_COMPATIBLE){ // Cuda is not present in the system
		
		cout << "error cuda no compatible"<< endl;
		n_gpu = 0;
        	
		return EGPU; 
    	}else{ // Cuda present in the system

		#ifdef DEBUG_GPU
			cout <<"dentor del else" << endl;
		#endif

        	if ((my_cuInit = (cuInit_pt) getProcAddress(*cuLib, "cuInit")) == NULL){
            		return 1; // sth is wrong with the library
        	}
		
		#ifdef DEBUG_GPU
			cout <<"cuInit success" << endl;
		#endif
	
		if ((my_nvmlInit = (nvmlInit_pt) getProcAddress(*nvmlLib, "nvmlInit")) == NULL){
            		return 1; // sth is wrong with the library
        	}

		#ifdef DEBUG_GPU
			cout <<"before device get count" << endl;
		#endif

        	if ((my_cuDeviceGetCount = (cuDeviceGetCount_pt) getProcAddress(*cuLib, "cuDeviceGetCount")) == NULL){
            		return 1; // sth is wrong with the library
        	}        
        
		#ifdef DEBUG_GPU
			cout << "device get cout success" << endl;    
        	#endif
	
		if ((my_cuDeviceComputeCapability = (cuDeviceComputeCapability_pt) getProcAddress(*cuLib, "cuDeviceComputeCapability")) == NULL){
			cout << " fallo en device compute capability" << endl;
        	}

		#ifdef DEBUG_GPU
			cout << "device compute capability success" << endl;    
        	#endif
	
		if ((my_cuDeviceGet = (cuDeviceGet_pt) getProcAddress(*cuLib, "cuDeviceGet")) == NULL){
			cout << "fallo cuda device get " << endl;
        	}

		#ifdef DEBUG_GPU
			cout << "cudeviceget success" << endl;    
        	#endif
	
		if ((my_cuDeviceGetName = (cuDeviceGetName_pt) getProcAddress(*cuLib, "cuDeviceGetName")) == NULL){
			cout << "fallo cuda device get " << endl;
        	}

        	int count = 0, i;
	
        	if(CUDA_SUCCESS != my_cuInit(0)){
            		cout <<"fallo en CUDA init" << endl;
			//return 1; // failed to initialize
		}
	
		#ifdef DEBUG_GPU
			cout << "init success" << endl;
		#endif

		if(NVML_SUCCESS!=my_nvmlInit()){
			cout << "fallo en el init de nvml" << endl;
		}

        	if (CUDA_SUCCESS != my_cuDeviceGetCount(&count)){
    	
			#ifdef DEBUG_GPU
				cout << "fallo en el device get count" << endl;
			#endif
    
		}

		if ((mynvmlDeviceGetMemoryInfo = (nvmlDeviceGetMemoryInfo_pt) getProcAddress(*nvmlLib, "nvmlDeviceGetMemoryInfo")) == NULL){

            		return 1; // sth is wrong with the library
        	}

		nvmlDeviceGetHandleByIndex_pt mynvmlDeviceGetHandleByIndex = NULL;
	
		if ((mynvmlDeviceGetHandleByIndex = (nvmlDeviceGetHandleByIndex_pt) getProcAddress(*nvmlLib, "nvmlDeviceGetHandleByIndex")) == NULL){
            		return 1; // sth is wrong with the library
        	}

		#ifdef DEBUG_GPU
			cout << "devicegetcount success" << endl;
		#endif


		int major = 0, minor = 0;
		int error = 0;
        
		for (i = 0; i < count; i++){
			major = 0;
			minor = 0;
		
			Gpu_dev gpudev;
	    		char dev_name[DEVNAMELENGTH];

			my_cuDeviceComputeCapability(&major, &minor, i);
		
			construct_capability(gpudev.capability, major, minor);	    	
	
			my_cuDeviceGet(&dev_, i);            	
	   
            		my_cuDeviceGetName(dev_name, DEVNAMELENGTH, dev_); 

			nvmlDevice_t device;
		
			error = mynvmlDeviceGetHandleByIndex(i, &device);

			nvmlMemory_t memoryinfo;
			error = mynvmlDeviceGetMemoryInfo(device, &memoryinfo);

        		gpudev.memTotal = memoryinfo.total / (1024*1024*1000);

            		gpudev.dev_id = i;
            		gpudev.model_name = dev_name;
     			gpudev.cudacomp = 1;       
            		gpus.push_back(gpudev);
			n_gpu++;

        	}

	

    	}
    
	
	return EOK;

}


#endif
