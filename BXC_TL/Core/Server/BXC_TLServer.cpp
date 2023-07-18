#include "../../include/BXC_TLServer.h"
#include "Server.h"
#include "ServerStream.h"
#include "../Common.h"
#include <map>
#define MESSAGE "build BXC_TLServer"
#pragma message(MESSAGE)

namespace BXC_TL {

	struct ServerObjManage {
	public:
		ServerObjManage() {
			LOGI("");
		}
		~ServerObjManage() {
			LOGI("");
		}
	public:
		std::map<int, Server*> objMap;
	public:
		int getSize() {
			return objMap.size();
		}
		bool remove(int id) {
			for (auto f = objMap.begin(); f != objMap.end(); ++f) {
				if (id == f->first) {
					Server* obj = f->second;
					delete obj;
					obj = nullptr;

					objMap.erase(f);
					return true;
				}
			}
			return false;
		}
		Server* get(int id) {
			for (auto f = objMap.begin(); f != objMap.end(); ++f) {
				if (id == f->first) {
					Server* obj = f->second;
					return obj;
				}
			}
			return nullptr;
		}
		int add(Server* obj) {
			int count = objMap.size() + 1001;
			objMap[count] = obj;
			return count;
		}
	};
	ServerObjManage objManage;

	int BXC_TLServer_Start(BXC_TLServerInfo*& serverInfo, BXC_NewConnectionCallBack newConnCb) {
		srand((int)time(nullptr));   //每次执行种子不同，生成不同的随机数

		Server* obj = new Server(serverInfo, newConnCb);
		serverInfo->id = objManage.add(obj);
		LOGI("id=%d,objSize=%d", serverInfo->id, objManage.getSize());
		
		if (!obj->start()) {
			BXC_TLServer_Stop(serverInfo);
			return -1;
		}
		return 0;

	}
	int BXC_TLServer_Stop(BXC_TLServerInfo* serverInfo) {
		LOGI("id=%d,objSize=%d", serverInfo->id, objManage.getSize());

		Server* obj = objManage.get(serverInfo->id);
		if (obj) {
			obj->stop();//阻塞停止
			return objManage.remove(serverInfo->id);
		}
		return -1;
	}
	int BXC_TLServer_setReadPacketCallBack(BXC_TLServerConnectionInfo* connInfo, BXC_ReadPacketCallBack connReadPacketCb) {
		Server* obj = objManage.get(connInfo->serverInfo->id);
		if (obj) {
			ServerStream* stream = obj->getStream(connInfo->fd);
			return stream->setReadPacketCallBack(connReadPacketCb);
		}

		return -1;
	}
	int BXC_TLServer_writePacket(BXC_TLServerConnectionInfo* connInfo, BXC_TLPacket* packet) {
		Server* obj = objManage.get(connInfo->serverInfo->id);
		if (obj) {
			ServerStream* stream = obj->getStream(connInfo->fd);
			return stream->writePacket(packet);
		}

		return -1;
	}
	int BXC_TLServer_conn_destory(BXC_TLServerConnectionInfo* conn) {

		return -1;
	}


}