#ifndef _XPIPE_H__
#define _XPIPE_H__

#if NEW_RTSP_CLIENT

typedef enum{
    XPIPE_DELETE=1,
    XPIPE_CONNECT_SUCC,
    XPIPE_CONNECTING,
    XPIPE_STOP_SERVICE
}XPipeOperateType;

class XPipe
{
public:
    XPipe();
    ~XPipe();
    int get_fd0() {return pipe_fd[0];}
    int get_fd1() {return pipe_fd[1];}
    int write(int obj,XPipeOperateType op);
    int read(int fd);
    int parse(int *obj,XPipeOperateType *op);
private:
    int pipe_fd[2];

    enum {PIPE_BUF_LEN = 10240};
    char buf[PIPE_BUF_LEN + 1];
    int lput;
    int lget;
};
#endif //NEW_RTSP_CLIENT

#endif

