
//#include "cl_input.h"
































/*int parse_arguments(){
	 if(argc == 1){
                        cerr << "Usage: ./DaeMon -h" << endl;
                        exit(0);
                }
                while((opt = getopt(argc, argv, "hds:p:i:t:a:m")) != -1)
                {

                        switch(opt)
                        {
                                case 'h':
                                        cout << "./DaeMon -i <time interval (ms)> -s <samples to send packed (1 - 255)> -t <threshold 0-100> -p <number port of server (1024 <= port <= 65535)> -a <address of server in dot format(x.x.x.x)>" << endl;

                                        exit(0);
                                        break;

                                case 'i':

                                        tinterval = strtol(optarg, NULL, 10);
                                        break;

                                case 'p':
                                        port = strtol(optarg, NULL, 10);
                                        break;

                                case 't':
                                        threshold = strtol(optarg, NULL, 10);
                                        break;

                                case 's':
                                        n_samples= strtol(optarg, NULL, 10);
                                        if(n_samples > MAX_SAMPLES){
                                                cerr << "Maximum number of samples allowed 127" << endl;
                                                exit(0);
                                        }
                                        break;

                                case 'a':
                                        server = optarg;
                                        printf("Server %s\n", server);
                                        break;
                                case 'm':
                                        mode = 1;
                                        print_manual();
                                        break;

                                case '?':
                                        if((optopt == 'p')){
                                                fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                                        }else if(optopt == 'a'){
                                                cerr << "ERROR: Option -" << optopt << "requires an argument." << std::endl;
                                        }else if (isprint (optopt)){
                                                fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                                        }else{
                                                fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
                                        }
                                        default:
                                        return -1;
                        }
		}



}*/
