#if NEW_RTSP_CLIENT

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/select.h>
#include<signal.h>
#include "xpipe.h"
#include "misc_utils.h"

static int do_fcntl(int fd, int cmd, long arg){
    int result;
    do{
        result = fcntl(fd,cmd,arg);
        if(result < 0){
            if(errno != EINTR){
                KILL_SELF();
            }
        }else{
            break;
        }
    }while(1);
    return result;
}

XPipe::XPipe():lput(0),lget(0){
    if(pipe(pipe_fd) < 0){
        KILL_SELF();
    }
    int flags = do_fcntl(pipe_fd[0],F_GETFL,0);
    do_fcntl(pipe_fd[0],F_SETFL,flags|O_NONBLOCK);

    flags = do_fcntl(pipe_fd[1],F_GETFL,0);
    do_fcntl(pipe_fd[1],F_SETFL,flags|O_NONBLOCK);
}

XPipe::~XPipe(){
    close(pipe_fd[0]);
    close(pipe_fd[1]);
}
int XPipe::write(int obj,XPipeOperateType op){
    char buf[64];
    sprintf(buf,"%d|%d;",obj,op);
    return ::write(pipe_fd[1],buf,strlen(buf));
}
int XPipe::read(int fd){
    char *p=buf;
    int len;
    while(lput>lget){
        *(p++)=*(buf+lget);
        lget++;
    }
    *p='\0';
    lput=p-buf;
    lget=0;

    do{
        if((len=::read(fd,buf+lput,PIPE_BUF_LEN-lput))<0){
            if(errno!=EINTR){
                KILL_SELF();
            }
        }else{
            break;
        }
    }while(1);

    lput+=len;
    buf[lput]='\0';
    return lput;
}

int XPipe::parse(int *obj,XPipeOperateType *op){
    char *pColon;
    char *pInter;
    if((pInter=strchr(buf+lget,'|'))==NULL)
        return -1;
    if((pColon=strchr(pInter,';'))==NULL)
        return -1;
    *pInter='\0';
    *pColon='\0';
    *obj=atoi(buf+lget);
    *op=(XPipeOperateType)(atoi(pInter+1));
    lget=pColon-buf+1;
    if(lget==lput){
        lget=lput=0;
    }
    return lget;
}

#endif //NEW_RTSP_CLIENT

