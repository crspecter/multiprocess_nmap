#include "nmap_work.h"
#include "thread_pool.h"
#include "task.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <stdio.h>
#include </usr/local/freetds/include/sqlfront.h>
#include "log.h"

//#define SER_IP "10.2.15.13"
//#define SER_PORT 11114
#define BUFSIZE  1024*10
#define TIMEOUT  240

nmap_work::nmap_work()
{
	pool = new thread_pool(10);
}

int ready_for_socket(std::string socket_ip,uint32_t socket_port)
{
   struct sockaddr_in ser_addr;
   int   sock_server = 0 ;
   int optval = 1; 
   socklen_t optlen = -1;
  
   ser_addr.sin_family = PF_INET;
   ser_addr.sin_addr.s_addr = inet_addr(socket_ip.c_str());  
   ser_addr.sin_port = htons(socket_port);
   memset(ser_addr.sin_zero,0,8); 
    //创建套接字--IPv4协议，面向连接通信   
   sock_server = socket(PF_INET,SOCK_STREAM,0); 
   if(sock_server < 0)
      return -1;
   //设置地址和端口号可以重复使用
   optlen = sizeof(optval);
   setsockopt(sock_server, SOL_SOCKET, SO_REUSEADDR, &optval, optlen);
   //绑定
   if(bind(sock_server, (const struct sockaddr *)(&ser_addr), sizeof(struct sockaddr)) != 0)
   {   close(sock_server); 
     return -1; }

  return sock_server;
}
int nmap_work::nmap_dispose_command(int sock_server)
{
  
   int   sock_client = 0 ;
   int   command_len = 0 ;
   char* command=NULL;   
   int clientlen;   
   struct sockaddr_in cli_addr;
  //等待客户端连接请求到达
//  LOG_INFO("wait for scan command....");

 //监听连接请求--监听队列长度为5
  listen(sock_server,20);
  //////////////////////////
   fd_set wset;
  int maxfd = sock_server + 1;
  struct timeval timeout = {2, 0};
  int status = -1;
   
  FD_ZERO(&wset);
  FD_SET(sock_server,&wset);
  status=select(maxfd,&wset,NULL,NULL,&timeout);
   if(status == 0||status<0)
   {
     return -1;
   }
  /////////////////////////
  clientlen=sizeof(struct sockaddr);
  sock_client= accept(sock_server,(struct sockaddr *)&cli_addr,(socklen_t*)&clientlen);
  if(sock_client < 0)
   return -1;   
  
   command=(char*)malloc(sizeof(char)*BUFSIZE);  
   strcpy(command,""); 
 if((command_len=recv(sock_client,command,BUFSIZE,0))>0)
  {
  // LOG_INFO("command is %s",command);  
   pool->thread_pool_add_task(task_nmap,command);
  }
 else
 {
   free(command);
 }

 close(sock_client);
// close(sock_server);
  return 1;
}

int nmap_work::nmap_read_from_db()
{
   char  *sqlquery=(char*)"select ip from ids_IPScanner";
   DBCHAR  scan_ip[100];
   static char* ip_data=NULL;
   //初始化
   dbinit();
   //连接数据库
   LOGINREC *loginrec = dblogin();
   DBSETLUSER(loginrec, dbuser.c_str());
   DBSETLPWD(loginrec, dbpassword.c_str());
   DBPROCESS *dbprocess = dbopen(loginrec, dbserver.c_str());
   if(dbprocess == FAIL)
   {
     LOG_INFO(" Conect MS SQL SERVER fail       ");
       return -1;
    }
   if(dbuse(dbprocess, dbname.c_str()) == FAIL)
    {
     LOG_INFO(" Open database name fail");
       return -1;
    }

    dbcmd(dbprocess,sqlquery);
    if(dbsqlexec(dbprocess) == FAIL) 
      LOG_INFO("nmap_work.cpp dbsqlexec  error!!!"); 
   
   int result=dbresults(dbprocess);
    if(result==1)  //结果不空
   {
     dbbind (dbprocess, 1, NTBSTRINGBIND, 0, (BYTE*)scan_ip); 
     
     while(dbnextrow(dbprocess) !=  NO_MORE_ROWS)
     {
      
      ip_data=(char*)malloc(sizeof(char)*256);
      strcpy(ip_data,"");
      strcpy(ip_data,(char*)scan_ip);   
     // printf("create ip is %p:%s \n",ip_data,ip_data); 
      pool->thread_pool_add_task(task_scanip,(void*)ip_data);
     }
  }    
 
    dbclose(dbprocess);
    return 0;

}
void nmap_work::read_config_file()
{
  Config_parser info;
  info.parse_file();
  socket_ip=info.get_string("socket","ip","0");
  socket_port=info.get_int("socket","port",0);
 
  dbuser=info.get_string("db","user","0");
  dbpassword=info.get_string("db","password","0");
  dbserver=info.get_string("db","server","0");
  dbname=info.get_string("db","dbname","0");
}

void nmap_work::nmap_do_work()
{ 
  system("killall -9 nmap");
  LOG_INFO("start nmap work!!");
  time_t start,end;
  start=time(NULL);
  end=time(NULL);
  int sock_server=-1;
 
 do{
     sock_server= ready_for_socket(socket_ip,socket_port);
	 usleep(100);
    }while(sock_server < 0);
    
  LOG_INFO("ready for socket!!"); 
  nmap_read_from_db();
  pool->create_thread_pool();
  while(1)
   {  
     nmap_dispose_command(sock_server);
     end=time(NULL);
    if (difftime(end,start)>= TIMEOUT)
    {
        nmap_read_from_db();
      LOG_INFO("time:%f excute here!!!",difftime(end,start));  
        start=time(NULL);
    }
	usleep(100);
 }
}