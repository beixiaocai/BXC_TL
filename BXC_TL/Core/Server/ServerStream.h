#ifndef SERVERSTREAM_H
#define SERVERSTREAM_H
#include "../Stream.h"

namespace BXC_TL {
	class ServerStream : public Stream
	{
	public:
		ServerStream(BXC_TLServerConnectionInfo* serverConnInfo);
		virtual ~ServerStream();
	public:
		virtual int write(uint8_t* data, int len);
		virtual int callReadPacketCallBack(BXC_TLPacket* packet);
	public:
		BXC_TLServerConnectionInfo* serverConnInfo = nullptr;

	};
};
#endif //SERVERSTREAM_H

