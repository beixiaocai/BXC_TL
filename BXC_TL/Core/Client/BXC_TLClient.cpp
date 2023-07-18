#include "../../include/BXC_TLClient.h"
#include "Client.h"
#include "ClientStream.h"
#include "../Common.h"
#include <map>
#define MESSAGE "build BXC_TLClient"
#pragma message(MESSAGE)

namespace BXC_TL {

	struct ClientObjManage {
	public:
		ClientObjManage() {
			LOGI("");
		}
		~ClientObjManage() {
			LOGI("");
		}
	public:
		std::map<int, Client*> objMap;
	public:
		int getSize() {
			return objMap.size();
		}
		bool remove(int id) {
			for (auto f = objMap.begin(); f != objMap.end(); ++f) {
				if (id == f->first) {
					Client* obj = f->second;
					delete obj;
					obj = nullptr;

					objMap.erase(f);
					return true;
				}
			}
			return false;
		}
		Client* get(int id) {
			for (auto f = objMap.begin(); f != objMap.end(); ++f) {
				if (id == f->first) {
					Client* obj = f->second;
					return obj;
				}
			}
			return nullptr;
		}
		int add(Client* obj) {
			int count = objMap.size() + 1001;
			objMap[count] = obj;
			return count;
		}
	};
	ClientObjManage objManage;

	int BXC_TLClient_Start(BXC_TLClientInfo*& clientInfo, BXC_ReadPacketCallBack cb) {
		srand((int)time(nullptr));   //每次执行种子不同，生成不同的随机数

		Client* obj = new Client(clientInfo);
		clientInfo->id = objManage.add(obj);
		LOGI("id=%d,objSize=%d", clientInfo->id, objManage.getSize());

		if (!obj->start()) {
			BXC_TLClient_Stop(clientInfo);
			return -1;
		}
		else {
			obj->getStream()->setReadPacketCallBack(cb);
		
		}
		return 0;
	}
	int BXC_TLClient_Stop(BXC_TLClientInfo* clientInfo) {
		LOGI("id=%d,objSize=%d", clientInfo->id, objManage.getSize());

		Client* obj = objManage.get(clientInfo->id);
		if (obj) {
			obj->stop();//阻塞停止
			return objManage.remove(clientInfo->id);
		}
		return -1;
	}

	int BXC_TLClient_writePacket(BXC_TLClientInfo* clientInfo, BXC_TLPacket* packet) {
		Client* obj = objManage.get(clientInfo->id);
		if (obj) {
			return obj->getStream()->writePacket(packet);
		}
		return -1;
	
	}



}