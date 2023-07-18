#include "ServerStream.h"
#include "../Common.h"
#include <WinSock2.h>
#include <WS2tcpip.h>

namespace BXC_TL {
	ServerStream::ServerStream(BXC_TLServerConnectionInfo* serverConnInfo):serverConnInfo(serverConnInfo),Stream()
	{
		LOGI("");
	}

	ServerStream::~ServerStream()
	{
		LOGI("");
		if (serverConnInfo) {

			if (serverConnInfo->dataLocalFd > 0) {//传输通道的描述符
				if (PROTOCOL_UDP == serverConnInfo->protocol) {//UDP的传输通道为新建UDP连接，描述符需要close
					closesocket(serverConnInfo->dataLocalFd);
				}
				serverConnInfo->dataLocalFd = -1;
			}

			if (serverConnInfo->fd > 0) {
				closesocket(serverConnInfo->fd);
				serverConnInfo->fd = -1;
			}



		}

	}
	int ServerStream::write(uint8_t* data, int len) {
		int ret = -1;
		if (PROTOCOL_TCP == serverConnInfo->protocol) {
			ret = ::send(serverConnInfo->dataLocalFd, (const char*)data, len, 0);
			if (ret < 0) {
				LOGE("ServerStream::write error: %d", WSAGetLastError());
			}
		}
		else if (PROTOCOL_UDP == serverConnInfo->protocol) {
			struct sockaddr_in peer_addr;
			peer_addr.sin_family = AF_INET;
			peer_addr.sin_port = htons(serverConnInfo->dataPeerPort);
			//peer_addr.sin_addr.s_addr = inet_addr(mConn->dataPeerIp);
			inet_pton(AF_INET, serverConnInfo->dataPeerIp, &peer_addr.sin_addr);
			int peer_addr_len = sizeof(sockaddr_in);

			ret = ::sendto(serverConnInfo->dataLocalFd, (const char*)data, len, 0, (struct sockaddr*)&peer_addr, peer_addr_len);
			if (ret < 0) {
				LOGE("ServerStream::write error: %d", WSAGetLastError());
			}

		}
		return ret;
	}
	int ServerStream::callReadPacketCallBack(BXC_TLPacket* packet) {
		mReadPacketCallBack(serverConnInfo, packet);
		return 0;
	}
};
