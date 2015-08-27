#ifndef __NMAP_WORK_H__
#define __NMAP_WORK_H__
#include "unistd.h"
#include "config.h"
     
class thread_pool;

class nmap_work
{
public:
	nmap_work();
	
	void nmap_do_work(); 
    int nmap_read_from_db();
    int nmap_dispose_command(int sock_server);
    void read_config_file();
public:
    thread_pool* pool;
public:
   std::string socket_ip;
   uint32_t socket_port;
  
   std::string dbuser;
   std::string dbpassword;
   std::string dbserver;
   std::string dbname;
};


#endif