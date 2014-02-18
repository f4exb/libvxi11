// Example code to demonstrate how one searches for VXI-11 devices on a subnet.
// M. Marino 4 November 2011

#include <map>
#include <string>
#include <iostream>
#include "clnt_find_services.h"
#include <rpc/pmap_clnt.h>
#include <arpa/inet.h>
#include "vxi11_user.h"

using std::endl;
using  std::cout;

class Ports {
  public:
    Ports(int tcp = 0, int udp = 0) : tcp_port(tcp), udp_port(udp) {}
    int tcp_port;
    int udp_port;
};

typedef std::map<std::string, Ports> AddrMap;
AddrMap gfFoundDevs;

bool_t who_responded(struct sockaddr_in *addr) 
{
  char str[INET_ADDRSTRLEN];
  const char* an_addr = inet_ntop(AF_INET, &(addr->sin_addr), str, INET_ADDRSTRLEN);
  if ( gfFoundDevs.find( std::string(an_addr) ) != gfFoundDevs.end() ) return 0;
  int port_T = pmap_getport(addr, DEVICE_CORE, DEVICE_CORE_VERSION, IPPROTO_TCP);
  int port_U = pmap_getport(addr, DEVICE_CORE, DEVICE_CORE_VERSION, IPPROTO_UDP);
  gfFoundDevs[ std::string( an_addr ) ] = Ports(port_T, port_U);
  return 0;
}


int main() 
{
  enum clnt_stat clnt_stat;
  const size_t MAXSIZE = 100;
  char rcv[MAXSIZE];
  timeval t;
  t.tv_sec = 1;
  t.tv_usec = 0;

  // Why 6 for the protocol for the VXI-11 devices?  Not sure, but the devices
  // will otherwise not respond. 
  clnt_stat = clnt_find_services(DEVICE_CORE, DEVICE_CORE_VERSION, 6, &t,
                                 who_responded);

  AddrMap::const_iterator iter;
  for (iter=gfFoundDevs.begin();iter!= gfFoundDevs.end();iter++) {
      const Ports& port = iter->second;
      cout << " Found: " << iter->first << " : TCP " << port.tcp_port 
           << "; UDP " << port.udp_port << endl;
      CLINK vxi_link;
      rcv[0] = '\0';
      if ( vxi11_open_device(iter->first.c_str(), &vxi_link) < 0 ) continue;
      int found = vxi11_send_and_receive(&vxi_link, "*IDN?", rcv, MAXSIZE, 10);
      if (found > 0) rcv[found] = '\0';
      cout << "  Output: " << rcv << endl;
  }

  return 0;
}
