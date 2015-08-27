#include "unistd.h"
#include "stdlib.h"
#include "daemon.h"
#include "nmap_work.h"

#include "log.h"






int main(int argc , char *argv[])
{

	init_dae  dae;
	dae.parse_cmd_line(argc, argv);


	nmap_work nw;
    
    nw.read_config_file();
	nw.nmap_do_work();
	for(;;)
	{	
		usleep(1000);
	}
	return 0;	
}
