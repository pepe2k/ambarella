#if NEW_RTSP_CLIENT

#include <stdio.h>
#include "RtspTcpAgent.h"
#include "RtspTcpConnection.h"

RtspTcpAgent * RtspTcpAgent::instance_=NULL;
pthread_mutex_t RtspTcpAgent::mutex_=PTHREAD_MUTEX_INITIALIZER;
RtspTcpAgent * RtspTcpAgent::instance(){
    if(instance_==NULL){
        pthread_mutex_lock(&mutex_);
        if(instance_==NULL){
             instance_=new RtspTcpAgent();
        }
        pthread_mutex_unlock(&mutex_);
    }
    return instance_;
}

RtspTcpAgent::~RtspTcpAgent(){
    //printf("RtspTcpAgent::~RtspTcpAgent() called\n");
    pthread_mutex_lock(&mutex_);
    instance_= NULL;
    pthread_mutex_unlock(&mutex_);
}

int RtspTcpAgent::connect(const std::string &ip,int port)
{
    RtspTcpConnection *xconn=new RtspTcpConnection();
    xconn->set_peer_addr(ip,port);
    xconn->connect();
    return xconn->get_id();
}
void RtspTcpAgent::connect_completed(void * XConn, bool result)
{
    RtspTcpConnection *conn=(RtspTcpConnection *)XConn;
    if(result==true)
    {
        pthread_mutex_lock(&mutex_);
        connIdentifers_[conn->get_id()]=XConn;
        pthread_mutex_unlock(&mutex_);
    }
    if(server_)
    {
        server_->connect_completed(conn->get_id(),result);
    }
}
void RtspTcpAgent::peer_close(void *XConn)
{
    RtspTcpConnection *conn=(RtspTcpConnection *)XConn;
    pthread_mutex_lock(&mutex_);
    connIdentifers_.erase(conn->get_id());
    pthread_mutex_unlock(&mutex_);
    if(server_)
    {
        server_->peer_close(conn->get_id());
    }
}
void RtspTcpAgent::close(unsigned int id)
{
    RtspTcpConnection *conn;
    pthread_mutex_lock(&mutex_);
    if(connIdentifers_.find(id)!=connIdentifers_.end())
    {
        conn=(RtspTcpConnection *)connIdentifers_[id];
        conn->close();
    }
    pthread_mutex_unlock(&mutex_);
}
void RtspTcpAgent::send_message(unsigned int id,const std::string & msg)
{
    RtspTcpConnection *conn;

    //printf("%s\n",msg.c_str());

    pthread_mutex_lock(&mutex_);
    if(connIdentifers_.find(id)!=connIdentifers_.end())
    {
        conn=(RtspTcpConnection *)connIdentifers_[id];
        conn->write(msg,false);
    }
    pthread_mutex_unlock(&mutex_);
}
void RtspTcpAgent::receive_message(void * XConn,char * base,int len)
{
    RtspTcpConnection *conn=(RtspTcpConnection *)XConn;
    if(server_)
    {
        server_->receive_message(conn->get_id(),base,len);
    }
}

#endif //NEW_RTSP_CLIENT

