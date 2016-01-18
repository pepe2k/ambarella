#ifndef WS_DISCOVERY
#define WS_DISCOVERY

#include <pthread.h>
#include <sys/types.h>

//--------------------------------------------------------------------------
//WsdDiscoveryClient
//--------------------------------------------------------------------------
class IWsdObserver
{
  public:
    virtual ~IWsdObserver()
  {
  }
  //ipaddr == NULL means device service is not available, otherwise device service is added or updated.
  virtual void onDeviceChanged(char *uuid, char *ipaddr) = 0;
};

class IWsdDeviceNotify
{
  public:
    virtual ~IWsdDeviceNotify()
  {
  }
  virtual int addDevice(char *uuid, char *ipaddr) = 0;
  virtual int removeDevice(char *uuid) = 0;
};

class WSDiscoveryClient: public IWsdDeviceNotify
{
  public:
    static WSDiscoveryClient *GetInstance();
    void Delete();
    int registerObserver(IWsdObserver *o);
    int removeObserver(IWsdObserver *o);
    int start(char *net_itf);
    int dumpDeviceList();
    virtual int addDevice(char *uuid, char *ipaddr);
    virtual int removeDevice(char *uuid);
    char *net_interface()
    {
      return (char *) net_itf;
    }
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
    struct ObserverNode
    {
        IWsdObserver *o;
        struct ObserverNode *next;
    }*ob_list;

    pthread_mutex_t device_mutex;
    struct DeviceNode
    {
        char uuid[64];
        char ipaddr[64];
        int64_t time;
        int notified;
        struct DeviceNode *next;
    }*device_list;

    void notifyObservers(char *uuid, char *ipaddr);
    void checkDevices();
    void checkNotify();
};

//--------------------------------------------------------------------------
//WsdDiscoveryService
//--------------------------------------------------------------------------
class IWsdClientNotify
{
  public:
    virtual ~IWsdClientNotify()
  {
  }
  ;
  virtual int addClient(char *uuid, char *ipaddr) = 0;
};
class IProbeMatch;
class WSDiscoveryService: public IWsdClientNotify
{
  public:
    static WSDiscoveryService *GetInstance();
    void Delete();
    int start(char *net_itf);
    int dumpClientList();
    virtual int addClient(char *uuid, char *ipaddr);
    void set_probematch(IProbeMatch *pm_)
    {
      pm = pm_;
    }
    ;
    char *net_interface()
    {
      return (char *) net_itf;
    }
  private:
    WSDiscoveryService();
    ~WSDiscoveryService();
    static WSDiscoveryService *instance;
    static pthread_mutex_t mutex;

  private:
    char net_itf[64];
    pthread_t thread_id;
    volatile int exit_flag;
    static void *routine(void*);

    pthread_mutex_t client_mutex;
    struct ClientNode
    {
        char uuid[64];
        char ipaddr[64];
        int64_t time;
        int replied;
        struct ClientNode *next;
    }*client_list;
    IProbeMatch *pm;
    void checkClients();
    void checkReply();
};

#endif//WS_DISCOVERY
