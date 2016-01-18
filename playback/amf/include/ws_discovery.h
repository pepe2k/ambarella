#ifndef WS_DISCOVERY
#define WS_DISCOVERY

#include <pthread.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//--------------------------------------------------------------------------
//WsdDiscoveryClient
//--------------------------------------------------------------------------
class IWsdObserver{
public:
    virtual ~IWsdObserver(){};
    //ipaddr == NULL means device service is not available, otherwise device service is added or updated.
    //When ipaddr != NUL, stream_url != NULL means  StreamUrl got.
    virtual void onDeviceChanged(char *uuid,char *ipaddr, char *stream_url) = 0;
};

typedef void (*cb_list_func)(void *usr, char *uuid,char *ipaddr,char *stream_url);

class IWsdDeviceNotify{
public:
    virtual ~IWsdDeviceNotify(){};
    virtual int addDevice(char *uuid,char *ipaddr,char *version,char *stream_url = NULL) = 0;
    virtual int removeDevice(char *uuid) = 0;
};

class IGetStreamUrl;
class WSDiscoveryClient:public IWsdDeviceNotify{
public:
    static WSDiscoveryClient *GetInstance();
    int Delete();
    int registerObserver(IWsdObserver *o);
    int removeObserver(IWsdObserver *o);
    //net_itf priority, "net_itf"  > "wlan0" > "eth0"
    int start(char *net_itf = NULL);
    int getDeviceList(cb_list_func func,void *usr);
    int dumpDeviceList();
    virtual int addDevice(char *uuid,char *ipaddr,char *version,char *stream_url = NULL);
    virtual int removeDevice(char *uuid);
    char *net_interface(){return (char *)net_itf;}
private:
    WSDiscoveryClient();
    ~WSDiscoveryClient();
    static WSDiscoveryClient *instance;
    static pthread_mutex_t mutex;

private:
    char net_itf[64];
    pthread_t thread_id;
    volatile int exit_flag;
    static void *routine(void*);

    pthread_mutex_t observer_mutex;
    struct ObserverNode{
        IWsdObserver *o;
        struct ObserverNode *next;
    }*ob_list;

    pthread_mutex_t device_mutex;
    struct DeviceNode{
        char uuid[64];
        char ipaddr[64];
        int64_t time;
        int notified;
        char stream_url[1024];
        int64_t time_request_url;
        struct DeviceNode *next;
        DeviceNode(){
            memset(uuid,0,sizeof(uuid));
            memset(ipaddr,0,sizeof(ipaddr));
            memset(stream_url,0,sizeof(stream_url));
            notified = 0;
            time = 0;
            time_request_url = 0;
        }
    }*device_list;

    void notifyObservers(char *uuid,char *ipaddr,char *stream_url = NULL);
    void checkDevices();
    void checkNotify(IGetStreamUrl *handler);
    void checkRequestUrl(IGetStreamUrl *handler);
};


//--------------------------------------------------------------------------
//WsdDiscoveryService
//--------------------------------------------------------------------------
class IWsdClientObserver{
public:
    virtual ~IWsdClientObserver(){};
    //ipaddr == NULL means Client is not available, otherwise Client is added or updated.
    //streamUrl is added for current solution for bi-direction audio.
    //        Client will implement a rtsp-server, and TargetService requests audio streaming and playback.
    //        When ipaddr != NUL, stream_url != NULL means  StreamUrl got.
    virtual void onClientChanged(char *uuid,char *ipaddr, char *stream_url) = 0;
};
typedef void (*cb_listclient_func)(void *usr, char *uuid,char *ipaddr,char *stream_url);

class IWsdClientNotify{
public:
    virtual ~IWsdClientNotify(){};
    virtual int addClient(char *uuid,char *ipaddr) = 0;
};
class IProbeMatch;
class WSDiscoveryService:public IWsdClientNotify{
public:
    static WSDiscoveryService *GetInstance();
    int Delete();
    int registerObserver(IWsdClientObserver *o);
    int removeObserver(IWsdClientObserver *o);

    //StreamUrl =  rtsp://10.0.0.2:rtsp_port/url
    int setRtspInfo(unsigned short rtsp_port = 554, const char *url = "stream1");
    //net_itf priority, "net_itf"  > "wlan0" > "eth0"
    int start(char *net_itf = NULL);
    int getClientList(cb_listclient_func func,void *usr);
    int dumpClientList();
    virtual int addClient(char *uuid,char *ipaddr);
    void set_probematch(IProbeMatch *pm_){pm = pm_;};

    unsigned short rtsp_port() {return rtsp_port_;}
    char *rtsp_url() {return (char*)rtsp_url_;}
    char *net_interface(){return (char *)net_itf;}
private:
    WSDiscoveryService();
    ~WSDiscoveryService();
    static WSDiscoveryService *instance;
    static pthread_mutex_t mutex;

private:
    unsigned short rtsp_port_;
    char rtsp_url_[1024];
    char net_itf[64];
    pthread_t thread_id;
    volatile int exit_flag;
    static void *routine(void*);

    pthread_mutex_t client_mutex;
    struct ClientNode{
        char uuid[64];
	 char ipaddr[64];
        char stream_url[1024];
        int64_t time;
        int replied;
        int notified;
        struct ClientNode *next;
    }*client_list;
    IProbeMatch *pm;
    void checkClients();
    void checkReply();

    pthread_mutex_t observer_mutex;
    struct ObserverNode{
        IWsdClientObserver *o;
        struct ObserverNode *next;
    }*ob_list;
    void notifyObservers(char *uuid,char *ipaddr,char *stream_url = NULL);
};

#endif//WS_DISCOVERY
