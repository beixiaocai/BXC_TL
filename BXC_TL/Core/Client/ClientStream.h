#ifndef CLIENTSTREAM_H
#define CLIENTSTREAM_H
#include "../Stream.h"

namespace BXC_TL {

	class ClientStream : public Stream
	{
	public:
		ClientStream(BXC_TLClientInfo* clientInfo);
		virtual ~ClientStream();
	public:
		virtual int callReadPacketCallBack(BXC_TLPacket* packet);
		virtual int write(uint8_t* data, int len);
	public:
		BXC_TLClientInfo* clientInfo = nullptr;
	};

};

#endif //CLIENTSTREAM_H
