#include <thread>
#include <BXC_TLClient.h>
#include <iostream>
#include <thread>
#include "Common.h"
using namespace BXC_TL;

static void readPacketCallBack(void* arg, BXC_TLPacket* packet) {
    BXC_TLClientInfo* clientInfo = (BXC_TLClientInfo*)arg;
    LOGI("readPacketCallBack size=%d", packet->size);

}
int main()
{
    //std::thread([]() {

    //	}).detach();
    //std::this_thread::sleep_for(std::chrono::milliseconds(100));
    BXC_TLClientInfo* clientInfo = new BXC_TLClientInfo(PROTOCOL_UDP, "127.0.0.1", 9900);
    int ret = BXC_TLClient_Start(clientInfo, readPacketCallBack);
    if (ret < 0) {
        return -1;
    }

    BXC_TLPacket* packet = new BXC_TLPacket(20000);
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
        int64_t ts = getCurTimestamp();
        uint8_t t = ts % 200;

        packet->dataType = t;  //数据类型
        packet->handleType = t;//操作类型
        packet->level = t;     //传输级别
        packet->extend = t;    //扩展

        BXC_TLClient_writePacket(clientInfo, packet);
    }

    delete packet;
    packet = nullptr;

    BXC_TLClient_Stop(clientInfo);
    delete clientInfo;
    clientInfo = nullptr;


    system("pause");


    return 0;
}
