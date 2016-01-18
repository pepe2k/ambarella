#ifndef WS_DISCOVERY_IMPL
#define WS_DISCOVERY_IMPL

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

class IWsdDeviceNotify;
class IWsdClientNotify;

class IProbeMatch
{
  public:
    virtual ~IProbeMatch()
  {
  }
  ;
  virtual int probe_match(char *ipaddr) = 0;
};
class WsdInstance: public IProbeMatch
{
  public:
    WsdInstance(IWsdDeviceNotify *o, IWsdClientNotify *c);
  public:
    struct content_t
    {
      char uuid[64];
      char ipaddr[64];
    };

    struct message_t
    {
        char type[64];
        char device[64];
        content_t content;
    };

    int initialize(char *interface);
    int uninitialize();

    int hello();
    int bye();
    int probe();
    virtual int probe_match(char *ipaddr);
    int on_hello(content_t *content);
    int on_bye(content_t *content);
    int on_probe(content_t *content);
    int on_probe_match(content_t *content);

    int parse_message(char *msg, int len, message_t *message);
    int create_recv_socket();
    int close_recv_socket(int sock_fd);
    int udp_recv(int sock_fd, char *buf, int len);
  private:
    int udp_muliticast_send(char *buf, int len);
    int udp_unicast_send(char *buf, int len, char *dst_ipaddr);
    void init_message(message_t* message);
  private:
    char net_interface[32];
    char local_ipaddr[64];
    char local_macaddr[64];
    char session_uuid[64];
    IWsdDeviceNotify *observer;
    IWsdClientNotify *client_observer;
};

#endif//WS_DISCOVERY_IMPL
