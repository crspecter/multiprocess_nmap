#include "task.h"
#include <unistd.h> 
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include "log.h"
#include </usr/local/freetds/include/sqlfront.h>

#define _PATH_ "/home/pyl/pyl_nmap/nmap"
#define _FILE_ "nmap"
//#define _ARG_  "-A 10.2.15.13"

pthread_rwlock_t rwlock;

char** split_command(void *arg)
{
  char *p;
  char str[1024*5];
  int num=0;
  char *argument=(char*)arg;
  char **fakeargv; 
  fakeargv = (char **) malloc(sizeof(char *) * 4);

  while(1)
   {
    p=strchr(argument,' ');
    if(num ==4)  break;
     if(p==NULL)
     {
        fakeargv[num++]=strdup(argument);
        break;
     }
     else
     {
       strncpy(str,argument,p-argument); 
       str[p-argument]=0;
       fakeargv[num++]=strdup(str);
       argument=p+1;
     }
   }
  return fakeargv;
}
int save_pid_to_db(int id,char* pd)
{
   char szUsername[32] = "sa"; 
   char szPassword[32]= "1qazxsw2*";
   char szDBName[32] = "ids_db";
   char szServer[32] = "10.2.10.201";
   char   sqlquery[1024]={0};
   snprintf(sqlquery,1024,"update ids_policy_event set pid='%s' where id='%d'",pd,id);
   dbinit();
   LOGINREC *loginrec = dblogin();
   DBSETLUSER(loginrec, szUsername);
   DBSETLPWD(loginrec, szPassword);
   DBPROCESS *dbprocess = dbopen(loginrec, szServer);
   if(dbprocess == FAIL)
   {
    LOG_INFO(" Conect MS SQL SERVER fail       ");
    return -1;
   }
   if(dbuse(dbprocess, szDBName) == FAIL)
   {
    LOG_INFO(" Open database name fail");
    return -1;
   }
   dbcmd(dbprocess,sqlquery);
   if(dbsqlexec(dbprocess) == FAIL)
   {
    LOG_INFO("task.cpp dbsqlexec  error!!!");
   }
  dbclose(dbprocess);
  return 0; 
}
void*  task_nmap(void *comd)
{
    char **fake=NULL;
    int i;
    pid_t pid;
    
    int id;
    char pd[32];
    pid_t del_pid;
    char  del_comd[256]={0};
 //   LOG_INFO("here is %s",comd);  
   fake=split_command(comd);
  ////删除扫描 
  if(strcmp(fake[2],"type=stop")==0)
   {
     del_pid=atoi(fake[3]+4); 
     snprintf(del_comd,256,"kill -9 %d",del_pid); 
     system(del_comd);
     goto over;
   }
  ////新建扫描任务

   pid=fork();  
   signal(SIGCHLD, SIG_IGN);  
	if(pid == 0 )
	{
	 
    if( execl(_PATH_, _FILE_,fake[1],fake[2],fake[3], NULL)<0)
          LOG_INFO("execl error!!!");
	}
	id=atoi(fake[1]+3);
    sprintf(pd,"%d",pid);
   // printf("child process pid is %d  id is %d pd is %s\n",pid,id ,pd);
   save_pid_to_db(id,pd); 
  
over:  if(fake!=NULL)
       {
        for(i=0;i<4;i++)
          free(fake[i]);
         free(fake);
         fake=NULL;
       }
     free(comd);    
	return NULL;
}

void*  task_scanip(void *ip)
{
  signal(SIGCHLD, SIG_IGN); 
  if(fork() == 0 )
  {
    if( execl(_PATH_, _FILE_,"id=0","-O",ip, NULL)<0)
       LOG_INFO("execl error!!!\n");
   }
  if(fork()==0)
  {
     if( execl(_PATH_, _FILE_,"id=0","-script=broadcast-netbios-master-browser",ip, NULL)<0)
      LOG_INFO("execl error!!!\n");
  }
  free(ip);
  return NULL;
}
