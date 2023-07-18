#ifndef BXC_TLCLIENT_H
#define BXC_TLCLIENT_H
#include "BXC_TL.h"


namespace BXC_TL {

#ifdef __cplusplus
	extern "C" {
#endif
		int __DECLSPEC_INC BXC_TLClient_Start(BXC_TLClientInfo*& clientInfo, BXC_ReadPacketCallBack cb);
		int __DECLSPEC_INC BXC_TLClient_Stop(BXC_TLClientInfo* clientInfo);

		int __DECLSPEC_INC BXC_TLClient_writePacket(BXC_TLClientInfo* clientInfo, BXC_TLPacket* packet);

#ifdef __cplusplus
	}
#endif
}
#endif //BXC_TLCLIENT_H
