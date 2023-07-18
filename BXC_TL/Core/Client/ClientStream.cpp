#include "ClientStream.h"
#include "../Common.h"
#include <WinSock2.h>
#include <WS2tcpip.h>

namespace BXC_TL {
	ClientStream::ClientStream(BXC_TLClientInfo* clientInfo):clientInfo(clientInfo), Stream()
	{
		LOGI("");
	}
	ClientStream::~ClientStream()
	{
		LOGI("");
		if (clientInfo) {
			if (clientInfo->dataLocalFd > 0) {//传输通道的描述符
				if (PROTOCOL_UDP == clientInfo->protocol) {//UDP的传输通道为新建UDP连接，描述符需要close
					closesocket(clientInfo->dataLocalFd);
				}
				clientInfo->dataLocalFd = -1;
			}

			if (clientInfo->fd > 0) {
				closesocket(clientInfo->fd);
				clientInfo->fd = -1;
			}
		}

	}
	int ClientStream::callReadPacketCallBack(BXC_TLPacket* packet) {
		mReadPacketCallBack(clientInfo, packet);
		return 0;
	}
	int ClientStream::write(uint8_t* data, int len) {
		int ret = -1;

		if (PROTOCOL_TCP == clientInfo->protocol) {
			ret = ::send(clientInfo->dataLocalFd, (const char*)data, len, 0);
			if (ret < 0) {
				LOGE("ClientStream::write error: %d", WSAGetLastError());
			}
		}
		else if (PROTOCOL_UDP == clientInfo->protocol) {

			struct sockaddr_in peer_addr;
			peer_addr.sin_family = AF_INET;
			peer_addr.sin_port = htons(clientInfo->dataPeerPort);
			//peer_addr.sin_addr.s_addr = inet_addr(mConn->dataPeerIp);
			inet_pton(AF_INET, clientInfo->dataPeerIp, &peer_addr.sin_addr);
			int peer_addr_len = sizeof(sockaddr_in);

			ret = ::sendto(clientInfo->dataLocalFd, (const char*)data, len, 0, (struct sockaddr*)&peer_addr, peer_addr_len);
			if (ret < 0) {
				LOGE("ClientStream::write error: ret=%d,%d", ret, WSAGetLastError());
			}
		}
		return ret;
	}
}