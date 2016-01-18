#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include "ws_discovery.h"
#include "ws_discovery_impl.h"

static const int US_TO_S = 1000000;

static int64_t ws_gettime(void)
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return (int64_t)tv.tv_sec * US_TO_S + tv.tv_usec;
}


//----------------------------------------------------------------------------------
//WSD Client
//----------------------------------------------------------------------------------
pthread_mutex_t WSDiscoveryClient::mutex = PTHREAD_MUTEX_INITIALIZER;
WSDiscoveryClient *WSDiscoveryClient::instance = NULL;
WSDiscoveryClient *WSDiscoveryClient::GetInstance(){
    pthread_mutex_lock(&mutex);
    if(instance == NULL){
        instance = new WSDiscoveryClient();
    }
    pthread_mutex_unlock(&mutex);
    return instance;
}

int WSDiscoveryClient::Delete(){
    delete instance;
    instance = NULL;
    return 0;
}

WSDiscoveryClient::WSDiscoveryClient(){
    memset(net_itf,0,sizeof(net_itf));
    thread_id = -1;
    exit_flag = 0;
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&observer_mutex,&attr);
    ob_list = NULL;

    pthread_mutex_init(&device_mutex,&attr);
    device_list = NULL;
    pthread_mutexattr_destroy(&attr);
}

WSDiscoveryClient::~WSDiscoveryClient(){
    if((int)thread_id != -1){
	exit_flag = 1;
	pthread_join(thread_id,NULL);
       thread_id = -1;
       exit_flag = 0;
    }
    //free ob_list
    pthread_mutex_lock(&observer_mutex);
    while(ob_list){
        ObserverNode *next = ob_list->next;
        delete ob_list;
        ob_list = next;
    }
    pthread_mutex_unlock(&observer_mutex);
    //free device_list
    pthread_mutex_lock(&device_mutex);
    while(device_list){
        DeviceNode *next = device_list->next;
        delete device_list;
        device_list = next;
    }
    pthread_mutex_unlock(&device_mutex);
    //
    pthread_mutex_destroy(&observer_mutex);
    pthread_mutex_destroy(&device_mutex);
}

int WSDiscoveryClient::start(char *net_itf_){
    if((int)thread_id != -1){
        return 0;
    }
    if(get_net_interface(net_itf_, net_itf,sizeof(net_itf)) < 0){
        return -1;
    }
    //assert(net_itf_);
    //snprintf(net_itf,sizeof(net_itf),"%s",net_itf_);
    int err = pthread_create(&thread_id,NULL,WSDiscoveryClient::routine,(void*)this);
    if(err){
        return -1;
    }
    return 0;
}
int WSDiscoveryClient::registerObserver(IWsdObserver *o){
    assert(o);
    ObserverNode *node = new ObserverNode();
    if(!node){
        return -1;
    }
    node->o = o;
    node->next = NULL;
    pthread_mutex_lock(&observer_mutex);
    //check duplicate first
    ObserverNode *curr = ob_list;
    while(curr){
        if(curr->o == o){
            delete node;
            pthread_mutex_unlock(&observer_mutex);
            return 0;
        }
        curr =curr->next;
    }
    if(ob_list){
        node->next = ob_list;
    }
    ob_list = node;
    pthread_mutex_unlock(&observer_mutex);
    return 0;
}

int WSDiscoveryClient::removeObserver(IWsdObserver *o){
    assert(o);
    pthread_mutex_lock(&observer_mutex);
    ObserverNode *prev = NULL,*curr = ob_list;
    while(curr){
        if(curr->o == o){
            if(prev){
                prev->next = curr->next;
            }else{
                ob_list = curr->next;
            }
            delete curr;
            pthread_mutex_unlock(&observer_mutex);
            return 0;
        }
        prev = curr;
    }
    pthread_mutex_unlock(&observer_mutex);
    printf("WARNING --- observer not registered\n");
    return 0;
}

void WSDiscoveryClient::notifyObservers(char *uuid,char *ipaddr,char *stream_url){
    pthread_mutex_lock(&observer_mutex);
    ObserverNode *curr = ob_list;
    while(curr){
        curr->o->onDeviceChanged(uuid,ipaddr,stream_url);
        curr = curr->next;
    }
    pthread_mutex_unlock(&observer_mutex);
}

int WSDiscoveryClient::getDeviceList(cb_list_func func,void *usr){
    pthread_mutex_lock(&device_mutex);
    DeviceNode *curr = device_list;
    while(curr){
        if(func){
            (*func)(usr,curr->uuid,curr->ipaddr,curr->stream_url);
        }
        curr = curr->next;
    }
    pthread_mutex_unlock(&device_mutex);
    return 0;
}

int WSDiscoveryClient::dumpDeviceList(){
    printf("WSDiscovery display devices found:\n");
    pthread_mutex_lock(&device_mutex);
    DeviceNode *curr = device_list;
    while(curr){
        printf("\tDevice: uuid[%s], ipaddr[%s],stream_url[%s]\n",curr->uuid,curr->ipaddr,curr->stream_url);
        curr = curr->next;
    }
    pthread_mutex_unlock(&device_mutex);
    printf("WSDiscovery display devices END\n");
    return 0;
}

int WSDiscoveryClient::addDevice(char *uuid,char *ipaddr,char *version,char *stream_url){
    assert(uuid);
    assert(ipaddr);
    assert(version);
    pthread_mutex_lock(&device_mutex);
    DeviceNode *curr = device_list;
    while(curr){
        if(!strcmp(curr->uuid,uuid)){
            if(!strcmp(version,"")){
                //default stream_url for cloudcamera
                snprintf(curr->stream_url,sizeof(curr->stream_url),"rtsp://%s/stream1",ipaddr);
            }else{
                //support GetStreamUrl
                if(stream_url &&strcmp(stream_url,"")&& strcmp(curr->stream_url,stream_url)){
                     snprintf(curr->ipaddr,sizeof(curr->ipaddr),"%s",ipaddr);
                     snprintf(curr->stream_url,sizeof(curr->stream_url),"%s",stream_url);
                     curr->time = ws_gettime();
                     curr->notified = 0;
                 }
            }
	     if(strcmp(curr->ipaddr,ipaddr)){
                 //ipaddr changed,update and notify observers
                 snprintf(curr->ipaddr,sizeof(curr->ipaddr),"%s",ipaddr);
                 curr->time = ws_gettime();
                 curr->notified = 0;
                 //printf("WSDiscovery addDevice--ipaddr changed-- uuid[%s], ipaddr[%s]\n",curr->uuid,curr->ipaddr);
                 pthread_mutex_unlock(&device_mutex);
                 return 0;
            }else{
                 //duplicate device
                 curr->time = ws_gettime();
                 pthread_mutex_unlock(&device_mutex);
                 return 0;
            }
        }
        curr =curr->next;
    }
    //new device, add to list and notify observers
    DeviceNode *node = new DeviceNode;
    if(!node){
        pthread_mutex_unlock(&device_mutex);
        return -1;
    }
    snprintf(node->uuid,sizeof(node->uuid),"%s",uuid);
    snprintf(node->ipaddr,sizeof(node->ipaddr),"%s",ipaddr);
    if(!strcmp(version,"")){
        //default stream_url for cloudcamera
        snprintf(node->stream_url,sizeof(node->stream_url),"rtsp://%s/stream1",ipaddr);
    }else{
        if(stream_url&&strcmp(stream_url,"")){
            snprintf(node->stream_url,sizeof(node->stream_url),"%s",stream_url);
        }else{
            node->stream_url[0] = '\0';
        }
    }
    node->time = ws_gettime();
    node->notified = 0;
    node->next = NULL;
    //printf("WSDiscovery addDevice--new - uuid[%s], ipaddr[%s]\n",node->uuid,node->ipaddr);

    if(device_list){
        node->next = device_list;
    }
    device_list = node;
    pthread_mutex_unlock(&device_mutex);
    return 0;
}

int WSDiscoveryClient::removeDevice(char *uuid){
    assert(uuid);
    pthread_mutex_lock(&device_mutex);
    DeviceNode *prev = NULL,*curr = device_list;
    while(curr){
        if(!strcmp(curr->uuid,uuid)){
            if(prev){
                prev->next = curr->next;
            }else{
                device_list = curr->next;
            }
            pthread_mutex_unlock(&device_mutex);
            notifyObservers(uuid,NULL);
            delete curr;
            return 0;
        }
        prev = curr;
        curr = curr->next;
    }
    printf("WARNING --- device not registered\n");
    pthread_mutex_unlock(&device_mutex);
    return 0;
}

void WSDiscoveryClient::checkDevices(){
    pthread_mutex_lock(&device_mutex);
    DeviceNode *prev = NULL,*curr = device_list;
    int64_t  now = ws_gettime();
    while(curr){
        if( (now - curr->time)/US_TO_S >= 10){
            if(prev){
                prev->next = curr->next;
            }else{
                device_list = curr->next;
            }
            DeviceNode *node = curr->next;
            notifyObservers(curr->uuid,NULL);
            delete curr;
            curr = node;
            continue;
        }
        prev = curr;
        curr = curr->next;
    }
    pthread_mutex_unlock(&device_mutex);
}

void WSDiscoveryClient::checkNotify(IGetStreamUrl *handler){
    pthread_mutex_lock(&device_mutex);
    DeviceNode *curr = device_list;
    while(curr){
        if(!curr->notified){
            if(!strcmp(curr->stream_url,"")){
                handler->get_url(curr->ipaddr);
                curr->time_request_url = ws_gettime();
                notifyObservers(curr->uuid,curr->ipaddr,NULL);
            }else{
                notifyObservers(curr->uuid,curr->ipaddr,curr->stream_url);
            }
            curr->notified = 1;
        }
        curr = curr->next;
    }
    pthread_mutex_unlock(&device_mutex);
}

void WSDiscoveryClient::checkRequestUrl(IGetStreamUrl *handler){
    pthread_mutex_lock(&device_mutex);
    DeviceNode *prev = NULL,*curr = device_list;
    int64_t  now = ws_gettime();
    while(curr){
        int seconds = (now - curr->time_request_url)/US_TO_S;
        if(curr->notified && (curr->stream_url[0] == '\0') && (seconds >= 2)){
            if(seconds < 5){
                //try again
                handler->get_url(curr->ipaddr);
                curr->time_request_url = now;
            }else{
                if(prev){
                    prev->next = curr->next;
                }else{
                    device_list = curr->next;
                }
                DeviceNode *node = curr->next;
                notifyObservers(curr->uuid,NULL);
                delete curr;
                curr = node;
                continue;
            }
        }
        prev = curr;
        curr = curr->next;
    }
    pthread_mutex_unlock(&device_mutex);
}

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include  <sys/param.h>
#include  <sys/ioctl.h>
#include <linux/if.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>

//How to implement heart-beat?
//Now we will multicast probe periodically.
void *WSDiscoveryClient::routine(void*arg){
    WSDiscoveryClient *thiz = (WSDiscoveryClient*)arg;
    WsdInstance wsd(thiz,NULL);
    if(wsd.initialize(thiz->net_interface()) < 0){
       return (void*)(-1);
    }
    int sock_fd = wsd.create_recv_socket();
    if(sock_fd < 0){
       return (void*)-1;
    }

    int64_t  last_cmd_time = ws_gettime();
    int64_t  last_notify_time = ws_gettime();
    int check_notify = 1;
    wsd.probe();

    char buf[2048];
    int len = sizeof(buf);
    while(!thiz->exit_flag){
        int recv_len,n;
        struct pollfd p[1] = {{sock_fd, POLLIN, 0}};
        n = poll(p, 1, 100);
        if(n > 0){
            if (p[0].revents & POLLIN){
                recv_len = wsd.udp_recv(sock_fd ,buf,len);
                if(recv_len < 0){
                    printf("wsd_target_service udp_recv EIO\n");
                    break;
                }else{
                    WsdInstance::message_t message;
                    if(!wsd.parse_message(buf,recv_len,&message)){
                        if(!strcmp(message.type,"WSD_HELLO")){
                            wsd.on_hello(&message.content);
                        }else if(!strcmp(message.type,"WSD_BYE")){
                            wsd.on_bye(&message.content);
                        }else if(!strcmp(message.type,"WSD_PROBEMATCH")){
                            wsd.on_probe_match(&message.content);
                        }else if(!strcmp(message.type,"WSD_REPORTURL")){
                            wsd.on_report_url(&message.content);
                        }
                    }
                }
            }
        }else if(n < 0){
            if(errno == EINTR) continue;
            break;//EIO
        }else{
             //TIMEOUT
        }
        if(check_notify  && (ws_gettime() - last_notify_time)/US_TO_S >= 2){
            thiz->checkNotify(&wsd);
            check_notify = 0;
        }
        if(!check_notify){
            thiz->checkRequestUrl(&wsd);
        }
        if((ws_gettime() - last_cmd_time)/US_TO_S >= 5){
            thiz->checkDevices();
            last_notify_time = ws_gettime();
            last_cmd_time = ws_gettime();
            wsd.probe();
            check_notify = 1;
        }
    }
    wsd.close_recv_socket(sock_fd);
    wsd.uninitialize();
    return (void*)NULL;
}

//------------------------------------------------------------------------------------
//WSD Service
//------------------------------------------------------------------------------------
pthread_mutex_t WSDiscoveryService::mutex = PTHREAD_MUTEX_INITIALIZER;
WSDiscoveryService *WSDiscoveryService::instance = NULL;
WSDiscoveryService *WSDiscoveryService::GetInstance(){
    pthread_mutex_lock(&mutex);
    if(instance == NULL){
        instance = new WSDiscoveryService();
    }
    pthread_mutex_unlock(&mutex);
    return instance;
}

int WSDiscoveryService::Delete(){
    delete instance;
    instance = NULL;
    return 0;
}

WSDiscoveryService::WSDiscoveryService(){
    memset(net_itf,0,sizeof(net_itf));
    thread_id = -1;
    exit_flag = 0;

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

    pthread_mutex_init(&client_mutex,NULL);
    client_list = NULL;
    pthread_mutex_init(&observer_mutex,&attr);
    ob_list = NULL;
    pthread_mutexattr_destroy(&attr);

    setRtspInfo();
}

WSDiscoveryService::~WSDiscoveryService(){
    if((int)thread_id != -1){
	exit_flag = 1;
	pthread_join(thread_id,NULL);
	thread_id = -1;
       exit_flag = 0;
    }
    //free ob_list
    pthread_mutex_lock(&client_mutex);
    while(client_list){
        ClientNode *next = client_list->next;
        delete client_list;
        client_list = next;
    }
    pthread_mutex_unlock(&client_mutex);
    //free ob_list
    pthread_mutex_lock(&observer_mutex);
    while(ob_list){
        ObserverNode *next = ob_list->next;
        delete ob_list;
        ob_list = next;
    }
    pthread_mutex_unlock(&observer_mutex);
    pthread_mutex_destroy(&client_mutex);
    pthread_mutex_destroy(&observer_mutex);
}

int WSDiscoveryService::setRtspInfo(unsigned short rtsp_port, const char *url){
    rtsp_port_ = rtsp_port;
    snprintf(rtsp_url_,sizeof(rtsp_url_),"%s",url);
    return 0;
}
int WSDiscoveryService::start(char *net_itf_){
    if((int)thread_id != -1){
        return 0;
    }
    if(get_net_interface(net_itf_, net_itf,sizeof(net_itf)) < 0){
        return -1;
    }
    //assert(net_itf_);
    //snprintf(net_itf,sizeof(net_itf),"%s",net_itf_);
    int err = pthread_create(&thread_id,NULL,WSDiscoveryService::routine,(void*)this);
    if(err){
        return -1;
    }
    return 0;
}

int WSDiscoveryService::registerObserver(IWsdClientObserver *o){
    assert(o);
    ObserverNode *node = new ObserverNode();
    if(!node){
        return -1;
    }
    node->o = o;
    node->next = NULL;
    pthread_mutex_lock(&observer_mutex);
    //check duplicate first
    ObserverNode *curr = ob_list;
    while(curr){
        if(curr->o == o){
            delete node;
            pthread_mutex_unlock(&observer_mutex);
            return 0;
        }
        curr =curr->next;
    }
    if(ob_list){
        node->next = ob_list;
    }
    ob_list = node;
    pthread_mutex_unlock(&observer_mutex);
    return 0;
}

int WSDiscoveryService::removeObserver(IWsdClientObserver *o){
    assert(o);
    pthread_mutex_lock(&observer_mutex);
    ObserverNode *prev = NULL,*curr = ob_list;
    while(curr){
        if(curr->o == o){
            if(prev){
                prev->next = curr->next;
            }else{
                ob_list = curr->next;
            }
            delete curr;
            pthread_mutex_unlock(&observer_mutex);
            return 0;
        }
        prev = curr;
    }
    pthread_mutex_unlock(&observer_mutex);
    printf("WARNING --- observer not registered\n");
    return 0;
}

void WSDiscoveryService::notifyObservers(char *uuid,char *ipaddr,char *stream_url){
    pthread_mutex_lock(&observer_mutex);
    ObserverNode *curr = ob_list;
    while(curr){
        curr->o->onClientChanged(uuid,ipaddr,stream_url);
        curr = curr->next;
    }
    pthread_mutex_unlock(&observer_mutex);
}

int WSDiscoveryService::getClientList(cb_listclient_func func,void *usr){
    pthread_mutex_lock(&client_mutex);
    ClientNode *curr = client_list;
    while(curr){
        if(func){
            (*func)(usr,curr->uuid,curr->ipaddr,curr->stream_url);
        }
        curr = curr->next;
    }
    pthread_mutex_unlock(&client_mutex);
    return 0;
}

int WSDiscoveryService::dumpClientList(){
    printf("WSDiscoveryService display clients:\n");
    pthread_mutex_lock(&client_mutex);
    ClientNode *curr = client_list;
    while(curr){
        printf("\tClient: uuid[%s], ipaddr[%s]\n",curr->uuid,curr->ipaddr);
        curr = curr->next;
    }
    pthread_mutex_unlock(&client_mutex);
    printf("WSDiscoveryService display clients END\n");
    return 0;
}

int WSDiscoveryService::addClient(char *uuid,char *ipaddr){
    assert(uuid);
    assert(ipaddr);
    pthread_mutex_lock(&client_mutex);
    ClientNode *curr = client_list;
    while(curr){
        if(!strcmp(curr->uuid,uuid)){
            if(strcmp(curr->ipaddr,ipaddr)){
                 snprintf(curr->ipaddr,sizeof(curr->ipaddr),"%s",ipaddr);
                 snprintf(curr->stream_url,sizeof(curr->stream_url),"rtsp://%s/stream_0",ipaddr);//hard code now.
                 curr->time = ws_gettime();
                 curr->replied = 0;
                 curr->notified = 0;
            }else if(curr->replied){
                 curr->replied = 0;//reset when replied but a new probe command received.
                 curr->time = ws_gettime();
            }
            pthread_mutex_unlock(&client_mutex);
            return 0;
        }
        curr =curr->next;
    }
    //new client, add to list
    ClientNode *node = new ClientNode;
    if(!node){
        pthread_mutex_unlock(&client_mutex);
        return -1;
    }
    snprintf(node->uuid,sizeof(node->uuid),"%s",uuid);
    snprintf(node->ipaddr,sizeof(node->ipaddr),"%s",ipaddr);
    snprintf(node->stream_url,sizeof(node->stream_url),"rtsp://%s/stream_0",ipaddr);//hard code now.
    node->time = ws_gettime();
    node->replied = 0;
    node->notified = 0;
    node->next = NULL;
    if(client_list){
        node->next = client_list;
    }
    client_list = node;
    //printf("WSDiscoveryService addClient,uuid[%s], ipaddr[%s]\n",node->uuid,node->ipaddr);
    pthread_mutex_unlock(&client_mutex);
    return 0;
}

void WSDiscoveryService::checkClients(){
    pthread_mutex_lock(&client_mutex);
    ClientNode *prev = NULL,*curr = client_list;
    int64_t  now = ws_gettime();
    while(curr){
        if( (now - curr->time)/US_TO_S >= 10){
            if(prev){
                prev->next = curr->next;
            }else{
                client_list = curr->next;
            }
            ClientNode *node = curr->next;
            printf("WSDiscoveryService remove client,uuid[%s], ipaddr[%s]\n",curr->uuid,curr->ipaddr);
            notifyObservers	(curr->uuid,NULL,NULL);
            delete curr;
            curr = node;
            continue;
        }
        prev = curr;
        curr = curr->next;
    }
    pthread_mutex_unlock(&client_mutex);
}

void WSDiscoveryService::checkReply(){
    assert(pm);
    pthread_mutex_lock(&client_mutex);
    ClientNode *curr = client_list;
    while(curr){
        int64_t now = ws_gettime();
        if((!curr->replied) && ((now - curr->time)/US_TO_S >= 2)){
            pm->probe_match(curr->ipaddr);//TODO
            curr->replied = 1;
            if(!curr->notified){
                notifyObservers(curr->uuid,curr->ipaddr,curr->stream_url);
                curr->notified = 1;
            }
        }
        curr = curr->next;
    }
    pthread_mutex_unlock(&client_mutex);
}

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include  <sys/param.h>
#include  <sys/ioctl.h>
#include <linux/if.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>

//How to implement heart-beat?
//Now client will multicast probe periodically.
void *WSDiscoveryService::routine(void*arg){
    WSDiscoveryService *thiz = (WSDiscoveryService*)arg;
    WsdInstance wsd(NULL,thiz);

    if(wsd.initialize(thiz->net_interface()) < 0){
       return (void*)(-1);
    }
    thiz->set_probematch(&wsd);

    int sock_fd = wsd.create_recv_socket();
    if(sock_fd < 0){
       return (void*)-1;
    }
    int64_t  last_check_time = ws_gettime();
    wsd.hello();

    char buf[2048];
    int len = sizeof(buf);
    while(!thiz->exit_flag){
        int recv_len,n;
        struct pollfd p[1] = {{sock_fd, POLLIN, 0}};
        n = poll(p, 1, 100);
        if(n > 0){
            if (p[0].revents & POLLIN){
                recv_len = wsd.udp_recv(sock_fd ,buf,len);
                if(recv_len < 0){
                    printf("wsd_target_service udp_recv EIO\n");
                    break;
                }else{
                    WsdInstance::message_t message;
                    if(!wsd.parse_message(buf,recv_len,&message)){
                        if(!strcmp(message.type,"WSD_PROBE")){
                           wsd.on_probe(&message.content);
                        }else if(!strcmp(message.type,"WSD_GETURL")){
                           wsd.on_get_url(message.content.ipaddr,thiz->rtsp_port(),thiz->rtsp_url());
                        }
                    }
                }
            }
        }else if(n < 0){
            if(errno == EINTR) continue;
            break;//EIO
        }else{
             //TIMEOUT
        }
	 thiz->checkReply();
        if((ws_gettime() - last_check_time)/US_TO_S >= 5){
            thiz->checkClients();
            last_check_time = ws_gettime();
        }
    }
    wsd.close_recv_socket(sock_fd);
    wsd.uninitialize();
    return (void*)NULL;
}

