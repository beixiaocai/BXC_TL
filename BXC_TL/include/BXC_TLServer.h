#ifndef BXC_TLSERVER_H
#define BXC_TLSERVER_H
#include "BXC_TL.h"

namespace BXC_TL {

#ifdef __cplusplus
	extern "C" {
#endif

		int __DECLSPEC_INC BXC_TLServer_Start(BXC_TLServerInfo*& serverInfo, BXC_NewConnectionCallBack newConnCb);
		int __DECLSPEC_INC BXC_TLServer_Stop(BXC_TLServerInfo* serverInfo);

		int __DECLSPEC_INC BXC_TLServer_setReadPacketCallBack(BXC_TLServerConnectionInfo* connInfo, BXC_ReadPacketCallBack connReadPacketCb);
		int __DECLSPEC_INC BXC_TLServer_writePacket(BXC_TLServerConnectionInfo* connInfo, BXC_TLPacket* packet);
		int __DECLSPEC_INC BXC_TLServer_conn_destory(BXC_TLServerConnectionInfo* connInfo);


#ifdef __cplusplus
	}
#endif
}
#endif //BXC_TLSERVER_H
