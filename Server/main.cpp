#include <thread>
#include <BXC_TLServer.h>
#include <iostream>
#include <thread>
#include <vector>
#include "Common.h"
using namespace BXC_TL;

static void readPacketCallBack(void* arg,BXC_TLPacket* packet) {
    BXC_TLServerConnectionInfo* connInfo = (BXC_TLServerConnectionInfo*)arg;
    LOGI("size=%d,dataType=%d,handleType=%d,level=%d,extend=%d",packet->size,packet->dataType,packet->handleType,packet->level,packet->extend);

    //BXC_TLServer_writePacket(connInfo, packet);

}

static void newConnectionCallBack(BXC_TLServerConnectionInfo* connInfo) {
    LOGI("fd=%d", connInfo->fd);
    BXC_TLServer_setReadPacketCallBack(connInfo, readPacketCallBack);
}


int main()
{

    //std::thread([]() {

    //	}).detach();
    //std::this_thread::sleep_for(std::chrono::milliseconds(100));

    BXC_TLServerInfo* serverInfo = new BXC_TLServerInfo("0.0.0.0",9900);
    int ret = BXC_TLServer_Start(serverInfo, newConnectionCallBack);
    if (ret < 0) {
        system("pause");
    }

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    BXC_TLServer_Stop(serverInfo);
    delete serverInfo;
    serverInfo = nullptr;



    system("pause");


    return 0;
}
