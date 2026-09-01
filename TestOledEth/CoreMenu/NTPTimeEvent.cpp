//
// Created by David Cherry on 25/06/2020.
//

#include <IoLogging.h>
#include "NTPTimeEvent.h"

#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "lwip/netif.h"

extern struct netif gnetif;

void acquireNtpTimeThreadProc(void *argument) {
    auto* ntpTime = static_cast<NTPTimeEvent*>(argument);
    ntpTime->acquireNtpOnThread();
    osThreadExit();
}

NTPTimeEvent::NTPTimeEvent(const char *serverName, int port)
        : timeServer(serverName), timePort(port), ntpThread(nullptr), _presentValue(0) {
        constexpr osThreadAttr_t attr = {
            .name = "NTP",
            .attr_bits = 0,
            .cb_mem = nullptr,
            .cb_size = 0,
            .stack_mem = nullptr,
            .stack_size = 4096,
            .priority = osPriorityNormal,
            .tz_module = 0,
            .reserved = 0
    };
    ntpThread = osThreadNew(acquireNtpTimeThreadProc, this, &attr);
}

void NTPTimeEvent::acquireNtpOnThread() {
    int retriesLeft = 500;
    while(--retriesLeft > 0) {
        // wait before trying to avoid a tight loop.
        osDelay( 10000U);

        serdebugF2("Starting ntp loop retries = ", retriesLeft);

        if (!netif_is_up(&gnetif) || !netif_is_link_up(&gnetif) || ip4_addr_isany_val(*netif_ip4_addr(&gnetif))) {
            continue;
        }

        addrinfo hints = {};
        addrinfo* result = nullptr;
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_DGRAM;

        char portText[8];
        snprintf(portText, sizeof(portText), "%d", timePort);

        if (lwip_getaddrinfo(timeServer, portText, &hints, &result) != 0 || result == nullptr) {
            continue;
        }

        serdebugF("Found NTP host");

        int32_t ntpRx[12] = {0};
        char ntpTx[48] = {0};
        ntpTx[0] = 0x1b;

        int socketFd = lwip_socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (socketFd < 0) {
            lwip_freeaddrinfo(result);
            continue;
        }

        struct timeval timeout = {};
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        lwip_setsockopt(socketFd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        lwip_setsockopt(socketFd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

        int sendCount = 0;
        int sendRet;
        while((sendRet = lwip_sendto(socketFd, ntpTx, sizeof(ntpTx), 0,
                                     result->ai_addr, result->ai_addrlen)) <= 0) {
            if(++sendCount > 20) break;
            osDelay(500U);
        }

        lwip_freeaddrinfo(result);

        serdebugF("Sent NTP message");

        struct sockaddr_storage sourceAddr = {};
        socklen_t addrLen = sizeof(sourceAddr);
        sendCount = 0;
        while(sendRet > 0 && ++sendCount < 20) {
            int n = lwip_recvfrom(socketFd, ntpRx, sizeof(ntpRx), 0, reinterpret_cast<sockaddr*>(&sourceAddr), &addrLen);

            if (n > 10) {
                uint32_t tmNowRaw = ntpRx[10];
                uint32_t ret = (tmNowRaw & 0xffU) << 24U;
                ret |= (tmNowRaw & 0xff00U) << 8U;
                ret |= (tmNowRaw & 0xff0000UL) >> 8U;
                ret |= (tmNowRaw & 0xff000000UL) >> 24U;
                _presentValue = ret - EPOCH_CONVERT_OFFSET;
                markTriggeredAndNotify();
                serdebugF2("Time was set to ", static_cast<unsigned long>(_presentValue));
                lwip_close(socketFd);
                return;
            }
        }
        // finally close the socket
        lwip_close(socketFd);
    }
}

uint32_t NTPTimeEvent::timeOfNextCheck() {
    return secondsToMicros(1);
}
