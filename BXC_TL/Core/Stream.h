#ifndef STREAM_H
#define STREAM_H
#include "../include/BXC_TL.h"

namespace BXC_TL {

	enum TRANSPORTPROCESS {//传输通道的状态
		TransportProcess_PARAMS = 1, // 共计40字节
		TransportProcess_PLAY,       // 共计10字节
		TransportProcess_OK
	};

	class Stream
	{
	public:
		Stream();
		virtual ~Stream();
	public:
		virtual int write(uint8_t* data, int len) = 0;
		virtual int callReadPacketCallBack(BXC_TLPacket* packet) = 0;
	public:
		int writePacket(BXC_TLPacket* packet);
		int setReadPacketCallBack(BXC_ReadPacketCallBack cb);
		void recvBuf(uint8_t* recvBuf, int recvBufSize);
		TRANSPORTPROCESS getSt();
		void setSt(TRANSPORTPROCESS st);
	public:
		uint32_t writePacketCount = 0;//发送包序号

	protected:
		BXC_ReadPacketCallBack   mReadPacketCallBack = nullptr;
	private:
		uint8_t* mCache1 = nullptr;
		int      mCache1_size = 0;

		uint8_t* mCache2 = nullptr;
		int	     mCache2_size = 0;

		BXC_TLPacket* mReadPacket;
		TRANSPORTPROCESS mSt;
	};
};
#endif //STREAM_H

