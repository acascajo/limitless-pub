import sys, getopt
import socket,struct 
import datetime
from collections import namedtuple
import time

SIZE_IP = 4
SIZE_CONF = 5

Hwcollection  = {};
getall = False



def ip2uint(ip):
    """
    Convert an IP string to long
    """
    packedIP = socket.inet_aton(ip)
    return struct.unpack("!I", packedIP)[0]

def int2ip(ip):
    """
    Convert an IP string to long
    """
    return socket.inet_ntoa(ip)


def obtainHWinfo(ipaddress,inputfile):
   global Hwcollection
   Hwconf = namedtuple("Hwconf", "node_name total_memory num_cpus num_cores num_io_devices num_network_interfaces num_gpus gpu_compat memory_gpu capa_gpu")
   with open(inputfile, 'rb') as ifile:
	ipfound = ifile.read(SIZE_IP)
	print "Detected: "+str(ipfound) 
	while ipfound:
		ip_decimal = struct.unpack("!I", ipfound)[0]
		print "Detected: "+str(ip_decimal)

		numcpus = struct.unpack("!B",ifile.read(1))[0]	
		numcores = struct.unpack("!B", ifile.read(1))[0]
		mem_total = struct.unpack("!B",ifile.read(1))[0]
		num_io = struct.unpack("!B",ifile.read(1))[0]
		num_net = struct.unpack("!B",ifile.read(1))[0]
		num_gpu =  struct.unpack("!B",ifile.read(1))[0]
		gpu_comp = []
		gpu_mem = []
		gpu_capa = []
		for gpu in range(0,num_gpu):
			gpu_comp.append(struct.unpack("!B",ifile.read(1))[0])
			gpu_mem.append(struct.unpack("!B",ifile.read(1))[0])
                        gpu_capa.append(struct.unpack("!B",ifile.read(1))[0])
		num_bytes =  struct.unpack("!B",ifile.read(1))[0]
		name_node = ifile.read(num_bytes)
		Hwcollection[ip_decimal] = Hwconf(name_node, mem_total, numcpus, numcores,num_io, num_net,num_gpu,gpu_comp,gpu_mem, gpu_capa)

		ipfound = ifile.read(SIZE_IP)
		print str(Hwcollection) 

def searchIP(ipaddress, inputfile):
   global Hwcollection 
   of = None
   """Open file in binary mode"""
   with open(inputfile, 'rb') as ifile:
   	ipfound = ifile.read(SIZE_IP)
        while ipfound:
		sig=0
		try:
			ip_decimal = struct.unpack("!I", ipfound)[0]
			n_samples = struct.unpack("!B",ifile.read(1))[0]
			print "NUmber of samples "+str(n_samples)	
			dummy_byte =struct.unpack("!B",ifile.read(1))[0]
			time_interval = struct.unpack("!I", ifile.read(4))[0]
			print "time_interval "+str(time_interval)
			timedec = struct.unpack("!Q", ifile.read(8))[0]
			print "timedec "+str(timedec)
			if getall:
				of = open(int2ip(ipfound)+'data.txt', 'a+')
			else:
				of = open(ipaddress+'data.txt', 'a+')
			for i in range (0, n_samples):
			
				try:
					strlog = Hwcollection[ip_decimal].node_name
				except:
					sig=1
					print "Ip error: {}".format(ip_decimal)
				if sig==0:
					strlog = Hwcollection[ip_decimal].node_name
					strlog = strlog+','+int2ip(ipfound)
					time_tot = datetime.timedelta(seconds=(time_interval * i))+datetime.datetime.fromtimestamp(timedec)
					'''time = datetime.timedelta(seconds=((time_interval/1000) * i)) + datetime.timedelta(seconds=(timedec/1000))
					print "Time: "+str(time)'''
					strlog = strlog + ','+ str(time_tot.strftime('%Y-%m-%d %H:%M:%S'))
					'''str(time.strftime('%Y-%m-%d %H:%M:%S'))'''
					'''time_tot.strftime('%Y-%m-%d %H:%M:%S'))'''
					mem = struct.unpack("!B",ifile.read(1))[0]
					strlog = strlog + ","+ str(mem)
					cpu = struct.unpack("!B",ifile.read(1))[0]
					strlog = strlog +","+str(cpu)
					for j in range (0, Hwcollection[ip_decimal].num_cpus):
                	        	        strlog = strlog +","+str( struct.unpack("!B",ifile.read(1))[0])
					for j in range (0, Hwcollection[ip_decimal].num_io_devices):
						strlog = strlog +","+str( struct.unpack("!B",ifile.read(1))[0])
						strlog = strlog +","+str( struct.unpack("!B",ifile.read(1))[0])
					for j in range (0, Hwcollection[ip_decimal].num_network_interfaces):
                	        	        strlog = strlog +","+str( struct.unpack("!B",ifile.read(1))[0])
						strlog = strlog +","+str( struct.unpack("!B",ifile.read(1))[0])
					for j in range (0, Hwcollection[ip_decimal].num_cpus):
                	        	        strlog = strlog +","+str( struct.unpack("!B",ifile.read(1))[0])
						strlog = strlog +","+str( struct.unpack("!B",ifile.read(1))[0])
					for j in range (0, Hwcollection[ip_decimal].num_gpus):
                	        	        strlog = strlog +","+str( struct.unpack("!B",ifile.read(1))[0])
						strlog = strlog +","+str( struct.unpack("!B",ifile.read(1))[0])
                	        	        strlog = strlog +","+str( struct.unpack("!B",ifile.read(1))[0])
						strlog = strlog +","+str( struct.unpack("!B",ifile.read(1))[0])
					strlog = strlog + '\n'
					struct.unpack("!B",ifile.read(1))[0]
					struct.unpack("!B",ifile.read(1))[0]
					print strlog
					if getall or ip_decimal == ip2uint(ipaddress):
						of.write(strlog)

		except:
			sig=1
				
		ifile.read(1)
		of.close()
		ipfound = ifile.read(SIZE_IP)								


def create_graph(ip, hwconf):
	import pandas as pd
	import numpy as np
	import datetime as dt
	import matplotlib.pyplot as plt
	from matplotlib.dates import HourLocator,SecondLocator, DateFormatter
	import matplotlib.dates as mdates
	from matplotlib.pyplot import cm
	
	filename = int2ip(struct.pack(">I", ip))+"data.txt"
	print filename
	num_columns = 5+hwconf.num_io_devices+hwconf.num_network_interfaces+(3*hwconf.num_cpus)+(4*(hwconf.num_gpus))
	names_columns = ['Name node','IP', 'Time', 'Mem (%)' , 'CPU(%)']
	for i in range(0, hwconf.num_cpus):
                names_columns.append("CPU Energy"+str(i)+" J")
	for i in range(0, hwconf.num_io_devices):
		names_columns.append("IO "+str(i)+" w(%)")
		names_columns.append("IO "+str(i)+" t(%)")
	for i in range(0, hwconf.num_network_interfaces):
                names_columns.append("Net "+str(i)+" Gb")
                names_columns.append("Net "+str(i)+" (%)")
	for i in range(0, hwconf.num_cpus):
                names_columns.append("Tmp "+str(i)+" C")
                names_columns.append("Tmp "+str(i)+" (%)")
	for i in range(0, hwconf.num_gpus):
                names_columns.append("GPU "+str(i)+" mem (%)")
                names_columns.append("GPU "+str(i)+" usage(%)")
                names_columns.append("GPU "+str(i)+" tmp(C)")
                names_columns.append("GPU "+str(i)+" power (W)")
	print str(len(names_columns))


	try:		
		data=pd.read_csv(filename, sep=',',header=None, names=names_columns)
	except IOError:
		return -1 
	print str(data)
	data.Time = data.Time.apply( lambda x: mdates.date2num(dt.datetime.strptime(x, '%Y-%m-%d %H:%M:%S')))
	
	fig, ax = plt.subplots()
	#n equals number of colors to be used
	n = 10
	#Obtain different colors for loops
	color=iter(cm.rainbow(np.linspace(0,1,n)))

        #GRAPH PER SECTION

        #ENERGY
	#ax.xaxis.set_major_locator(HourLocator())
	ax.xaxis.set_major_formatter(DateFormatter('%Y-%m-%d %H:%M:%S'))
	ax.fmt_xdata = DateFormatter('%Y-%m-%d %H:%M:%S')
	fig.autofmt_xdate()
	ener = []
	for i in range(0, hwconf.num_cpus):
		c = next(color)
		enertmp, = plt.plot_date(data['Time'], data["CPU Energy"+str(i)+" J"],c=c, fmt='-', label = 'CPU ' +str(i) +'(J)')
		ener.append(enertmp)
	#cpu, = plt.plot_date(data['Time'], data['CPU(%)'],fmt='b-', label = 'CPU(%)')
	#mem, = plt.plot_date(data['Time'], data['Mem (%)'],fmt='r-', label = 'Mem(%)')
	#for i in range(0, hwconf.num_io_devices):
		#sdaw, = plt.plot_date(data['Time'], data["IO 0 w(%)"],fmt='y-', label = 'sda w(%)')
		#sdat, = plt.plot_date(data['Time'], data["IO "+str(i)+" t(%)"],fmt='yx', label = 'sda t(%)')
	#sdbw, = plt.plot_date(data['Time'], data["IO 1 w(%)"],fmt='g-', label = 'sdb w(%)')
        #sdbt, = plt.plot_date(data['Time'], data["IO 1 t(%)"],fmt='gx', label = 'sdb t(%)' )
	plt.legend(ener)
	plt.title("Node "+ data['Name node'])
	
	plt.ylabel("J")
	
	#ax.autoscale_view()	
	plt.savefig(int2ip(struct.pack(">I", ip))+'ene.pdf')
	plt.close()	
#plt.show()
        
        #CPU
	fig, ax = plt.subplots()
	#n equals number of colors to be used
	n = 10
	#Obtain different colors for loops
	color=iter(cm.rainbow(np.linspace(0,1,n)))
        #ax.xaxis.set_major_locator(HourLocator())
        ax.xaxis.set_major_formatter(DateFormatter('%Y-%m-%d %H:%M:%S'))
        ax.fmt_xdata = DateFormatter('%Y-%m-%d %H:%M:%S')
        fig.autofmt_xdate()
        for i in range(0, hwconf.num_cpus):
                c = next(color)
                cpu, = plt.plot_date(data['Time'], data['CPU(%)'],fmt='b-', label = 'CPU(%)')
        plt.title("Node "+ data['Name node'])
        plt.ylabel("%")
        #ax.autoscale_view()
        plt.savefig(int2ip(struct.pack(">I", ip))+'cpu.pdf')
        plt.close()
#plt.show()

        #MEM
        fig, ax = plt.subplots()
        #n equals number of colors to be used
        n = 10
        #Obtain different colors for loops
        color=iter(cm.rainbow(np.linspace(0,1,n)))
        #ax.xaxis.set_major_locator(HourLocator())
        ax.xaxis.set_major_formatter(DateFormatter('%Y-%m-%d %H:%M:%S'))
        ax.fmt_xdata = DateFormatter('%Y-%m-%d %H:%M:%S')
        fig.autofmt_xdate()
	mem, = plt.plot_date(data['Time'], data['Mem (%)'],fmt='r-', label = 'Mem(%)')
        plt.title("Node "+ data['Name node'])
        plt.ylabel("%")
        #ax.autoscale_view()
        plt.savefig(int2ip(struct.pack(">I", ip))+'mem.pdf')
        plt.close()
#plt.show()

	#NET
        fig, ax = plt.subplots()
        #n equals number of colors to be used
        n = 10
        #Obtain different colors for loops
        color=iter(cm.rainbow(np.linspace(0,1,n)))
        #ax.xaxis.set_major_locator(HourLocator())
        ax.xaxis.set_major_formatter(DateFormatter('%Y-%m-%d %H:%M:%S'))
        ax.fmt_xdata = DateFormatter('%Y-%m-%d %H:%M:%S')
        fig.autofmt_xdate()
	net, = plt.plot_date(data['Time'], data["Net "+str(i)+" (%)"],fmt='r-', label = 'Mem(%)')
        plt.title("Node "+ data['Name node'])
        plt.ylabel("%")
        #ax.autoscale_view()
        plt.savefig(int2ip(struct.pack(">I", ip))+'net.pdf')
        plt.close()
#plt.show()

        #IO
        fig, ax = plt.subplots()
        #n equals number of colors to be used
        n = 10
        #Obtain different colors for loops
        color=iter(cm.rainbow(np.linspace(0,1,n)))
        #ax.xaxis.set_major_locator(HourLocator())
        ax.xaxis.set_major_formatter(DateFormatter('%Y-%m-%d %H:%M:%S'))
        ax.fmt_xdata = DateFormatter('%Y-%m-%d %H:%M:%S')
        fig.autofmt_xdate()
        for i in range(0, hwconf.num_io_devices):
                #sdaw, = plt.plot_date(data['Time'], data["IO 0 w(%)"],fmt='y-', label = 'sda w(%)')
                sdat, = plt.plot_date(data['Time'], data["IO "+str(i)+" t(%)"],fmt='yx', label = 'sda t(%)')
        #sdbw, = plt.plot_date(data['Time'], data["IO 1 w(%)"],fmt='g-', label = 'sdb w(%)')
        #sdbt, = plt.plot_date(data['Time'], data["IO 1 t(%)"],fmt='gx', label = 'sdb t(%)' )
        
        plt.title("Node "+ data['Name node'])
        plt.ylabel("IO (%)")
        #ax.autoscale_view()
        plt.savefig(int2ip(struct.pack(">I", ip))+'io.pdf')
        plt.close()
#plt.show()
        fig, ax = plt.subplots()
        #n equals number of colors to be used
        n = 10
        #Obtain different colors for loops
        color=iter(cm.rainbow(np.linspace(0,1,n)))
        #ax.xaxis.set_major_locator(HourLocator())
        ax.xaxis.set_major_formatter(DateFormatter('%Y-%m-%d %H:%M:%S'))
        ax.fmt_xdata = DateFormatter('%Y-%m-%d %H:%M:%S')
        fig.autofmt_xdate()
        for i in range(0, hwconf.num_io_devices):
                sdaw, = plt.plot_date(data['Time'], data["IO 0 w(%)"],fmt='y-', label = 'sda w(%)')
                #sdat, = plt.plot_date(data['Time'], data["IO "+str(i)+" t(%)"],fmt='yx', label = 'sda t(%)')
        #sdbw, = plt.plot_date(data['Time'], data["IO 1 w(%)"],fmt='g-', label = 'sdb w(%)')
        #sdbt, = plt.plot_date(data['Time'], data["IO 1 t(%)"],fmt='gx', label = 'sdb t(%)' )

        plt.title("Node "+ data['Name node'])
        plt.ylabel("W (%)")
        #ax.autoscale_view()
        plt.savefig(int2ip(struct.pack(">I", ip))+'io_w.pdf')
        plt.close()
#plt.show()









def main(argv):
   global getall
   global Hwcollection
   ipaddress = ''
   inputfile = ''
   outputfile = ''
   hwinputfile = ''
   generate_graph = False
   try:
      opts, args = getopt.getopt(argv,"hi:f:c:g")
   except getopt.GetoptError:
      print 'test.py -ip <IP> -o <outputfile>'
      sys.exit(2)
   for opt, arg in opts:
      if opt == '-h':
         print 'daemon_reader.py -ip <IP>'
         sys.exit()
      elif opt in ("-i"):
         ipaddress = arg
	 if ipaddress == 'a':
		getall = True 
      elif opt in ("-f"):
	 inputfile = arg
      elif opt in ("-g"):
	 generate_graph = True
      elif opt in ("-c"):
         hwinputfile = arg

   print 'IP to be searched is ', ipaddress
   print 'HW conf is : ', hwinputfile
   obtainHWinfo(ipaddress,hwinputfile)
   print 'End HW info'
   searchIP(ipaddress, inputfile)
   if generate_graph:
	for ip, val  in Hwcollection.items():
		create_graph(ip, val)

if __name__ == "__main__":
  main(sys.argv[1:])
