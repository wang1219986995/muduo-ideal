
/******************************************************
*   Copyright (C)2019 All rights reserved.
*
*   Author        : owb
*   Email         : 2478644416@qq.com
*   File Name     : InetAddress.cc
*   Last Modified : 2019-11-23 15:16
*   Describe      :
*
*******************************************************/

#include "ideal/base/Logger.h"
#include "ideal/net/Endian.h"
#include "ideal/net/InetAddress.h"
#include "ideal/net/SocketUtil.h"

#include <assert.h>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <net/if.h>

using namespace ideal;
using namespace ideal::net;

//     /* Structure describing an Internet socket address.  */
//     struct sockaddr_in {

//         sa_family_t    sin_family; /* address family: AF_INET */
//         uint16_t       sin_port;   /* port in network byte order */
//         struct in_addr sin_addr;   /* internet address */
//     };

//     /* Internet address. */
//     typedef uint32_t in_addr_t;
//     struct in_addr {
//         in_addr_t       s_addr;     /* address in network byte order */
//     };

InetAddress::InetAddress(uint16_t port, bool lookbackOnly) {
    memset(&_addr, 0, sizeof _addr);
    _addr.sin_family = AF_INET;
    in_addr_t ip = lookbackOnly? INADDR_LOOPBACK : INADDR_ANY;
    _addr.sin_addr.s_addr = sockets::hostToNetwork32(ip);
    _addr.sin_port = sockets::hostToNetwork16(port);
}

InetAddress::InetAddress(StringArg ip, uint16_t port) {
    memset(&_addr, 0, sizeof _addr);
    sockets::fromIpPort(ip.c_str(), port, &_addr);
}

std::string InetAddress::toIpPort() const {
    char buf[32] = "";
    sockets::toIpPort(buf, sizeof buf, getSockAddr());
    return buf;
}

std::string InetAddress::toIp() const {
    char buf[32] = "";
    sockets::toIp(buf, sizeof buf, getSockAddr());
    return buf;
}

uint16_t InetAddress::toPort() const {
    return sockets::networkToHost16(portNetEndian());
}


static __thread char t_resolveBuffer[64 * 1024];

// DNS
bool InetAddress::resolve(StringArg hostname, InetAddress* result) {
    assert(result != nullptr);
    struct hostent hent;
    struct hostent* phent = nullptr;
    int herrno = 0;
    memset(&hent, 0, sizeof hent);

    int ret = gethostbyname_r(hostname.c_str(), &hent, t_resolveBuffer, sizeof t_resolveBuffer, &phent, &herrno);
    if(ret == 0 && phent != nullptr) {
        assert(phent->h_addrtype == AF_INET && phent->h_length == sizeof(uint32_t));
        result->_addr.sin_addr = *reinterpret_cast<struct in_addr*>(phent->h_addr);
        return true;
    }
    else {
        if(ret) {
            LOG_SYSERR << "InetAddress::resolve";
        }
        return false;
    }
}

std::string InetAddress::getLocalIpAddress() {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        close(sockfd);
        return "0.0.0.0";
    }
 	
	struct ifconf ifconf;
	char buf[512] = { 0 };
    ifconf.ifc_len = sizeof buf;
    ifconf.ifc_buf = buf;
    if(ioctl(sockfd, SIOCGIFCONF, &ifconf) == -1) {
        close(sockfd);
        return "0.0.0.0";
    }

    close(sockfd);

    struct ifreq* ifreq = ifconf.ifc_req;
    for(int i = (ifconf.ifc_len / sizeof(struct ifreq)); i > 0; --i) {
        if (ifreq->ifr_flags == AF_INET) {
            if(strcmp(ifreq->ifr_name, "lo") != 0) {
                return inet_ntoa(((struct sockaddr_in*)&(ifreq->ifr_addr))->sin_addr); // 取第一个
            }
            ifreq++;
        }
    }
    return "0.0.0.0";
}

