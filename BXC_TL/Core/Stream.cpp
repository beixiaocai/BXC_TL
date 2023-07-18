#include "Stream.h"
#include "TpPacket.h"
#include "Common.h"

namespace BXC_TL {
	Stream::Stream(): mSt(TransportProcess_PARAMS)
	{
		int max_size = BXC_TLPACKET_MAX_SIZE;

		mCache1 = (uint8_t*)malloc(max_size);

		mCache2 = (uint8_t*)malloc(max_size);

		mReadPacket = new BXC_TLPacket(max_size);
	}

	Stream::~Stream()
	{
		LOGI("");
		if (mCache1) {
			free(mCache1);
			mCache1 = nullptr;
		}
		if (mCache2) {
			free(mCache2);
			mCache2 = nullptr;
		}
		if (this->mReadPacket) {
			delete this->mReadPacket;
			this->mReadPacket = nullptr;
		}

	}
	int Stream::setReadPacketCallBack(BXC_ReadPacketCallBack cb) {
		mReadPacketCallBack = cb;
		return 0;
	}

	int Stream::writePacket(BXC_TLPacket* packet) {
		++(writePacketCount);
		//分片打包

		int a = packet->size / TP_BODY_MAX_SIZE;
		int b = packet->size % TP_BODY_MAX_SIZE;

		uint16_t totalSubSeqNumber = a;
		if (b > 0) {
			++totalSubSeqNumber;
		}

		uint8_t tpHeaderData[20];
		TpHeader tpHeader;
		tpHeader.flag1 = TP_HEADER_FLAG1;
		tpHeader.flag2 = TP_HEADER_FLAG2;
		tpHeader.tpBodySize = 0;

		tpHeader.dataType = packet->dataType;
		tpHeader.handleType = packet->handleType;
		tpHeader.level = packet->level;
		tpHeader.extend = packet->extend;

		tpHeader.subSeqNumber = 0;
		tpHeader.totalSubSeqNumber = totalSubSeqNumber;//分包总数量
		tpHeader.seqNumber = writePacketCount;
		tpHeader.timestamp = 0;


		int pos = 0;
		uint16_t subSeqNumber = 0;

		int ret;

		for (int i = 0; i < a; i++)
		{//发送完整包
			subSeqNumber = i + 1;

			tpHeader.tpBodySize = TP_BODY_MAX_SIZE;
			tpHeader.subSeqNumber = subSeqNumber;
			tpHeader.timestamp = getCurTimestamp();
			TpHeaderSerialize(&tpHeader, tpHeaderData);

			ret = this->write(tpHeaderData, TP_HEADER_SIZE);
			if (ret < 0) {
				return -1;
			}
			ret = this->write(packet->data + pos, tpHeader.tpBodySize);
			if (ret < 0) {
				return -1;
			}
			pos += tpHeader.tpBodySize;
		}

		if (b > 0) {//发送最后一包非完整包
			++subSeqNumber;

			tpHeader.tpBodySize = packet->size - pos;
			tpHeader.subSeqNumber = subSeqNumber;
			tpHeader.timestamp = getCurTimestamp();
			TpHeaderSerialize(&tpHeader, tpHeaderData);

			ret = this->write(tpHeaderData, TP_HEADER_SIZE);
			if (ret < 0) {
				return -1;
			}
			ret = this->write(packet->data + pos, tpHeader.tpBodySize);
			if (ret < 0) {
				return -1;
			}
			pos += tpHeader.tpBodySize;

		}

		return 0;


	}

	TRANSPORTPROCESS Stream::getSt() {
		return mSt;
	}
	void Stream::setSt(TRANSPORTPROCESS st) {
		mSt = st;
	}

	void Stream::recvBuf(uint8_t* recvBuf, int recvBufSize) {
		/*
		uint64_t speedTotalSize = 0;
		uint64_t  t1 = getCurTimestamp();
		uint64_t  t2 = 0;
		speedTotalSize += recvBufSize;
		if (speedTotalSize > 2097152) // 2097152=2*1024*1024=2mb
		{
			t2 = getCurTimestamp();
			if (t2 - t1 > 0) {
				uint64_t speed = speedTotalSize / 1024 / (t2 - t1);
				printf("fd=%d,speedTotalSize=%llu,speed=%llu Kbps\n", mConn->fd, speedTotalSize, speed);
				speedTotalSize = 0;
				t1 = getCurTimestamp();
			}
		}
		*/


		if ((mCache1_size + recvBufSize) > BXC_TLPACKET_MAX_SIZE) {
			LOGE("超过缓冲容量上限，mCache1_size=%d,recvBufSize=%d", mCache1_size, recvBufSize);
		}
		else {
			memcpy(mCache1 + mCache1_size, recvBuf, recvBufSize);
			mCache1_size += recvBufSize;
		}

		TpHeader tpHeader;
		int rc = 0;
		while (true) {

			bool success = false;
			if (mCache1_size > TP_HEADER_SIZE) {
				if (TP_HEADER_FLAG1 == mCache1[0] && TP_HEADER_FLAG2 == mCache1[1]) {
					success = TpHeaderDeSerialize(mCache1, &tpHeader);
				}
			}
			if (success) {
				if (mCache1_size >= (TP_HEADER_SIZE + tpHeader.tpBodySize)) {
					mCache1_size -= TP_HEADER_SIZE;

					//将完整的TpPacket提取到缓存2 start
					memcpy(mCache2 + mCache2_size, mCache1 + TP_HEADER_SIZE, tpHeader.tpBodySize);
					mCache2_size += tpHeader.tpBodySize;
					//将完整的TpPacket提取到缓存2 end

					mCache1_size -= tpHeader.tpBodySize;
					memmove(mCache1, mCache1 + TP_HEADER_SIZE + tpHeader.tpBodySize, mCache1_size);


					++rc;
					//LOGI("rc=%d,mReciver->cacheSize=%d,mRecvCacheSize=%d", rc, mReciver->cacheSize, mRecvCacheSize);
					LOGI("rc=%d,subSeqNumber=%d,totalSubSeqNumber=%d,seqNumber=%d", rc, tpHeader.subSeqNumber, tpHeader.totalSubSeqNumber, tpHeader.seqNumber);

					if (tpHeader.subSeqNumber < tpHeader.totalSubSeqNumber) {
						//非最后一包
					}
					else {
						//最后一包TpPacket

						//将缓存2获取的完整packet赋值 start
						memcpy(mReadPacket->data, mCache2, mCache2_size);
						mReadPacket->size = mCache2_size;
						mReadPacket->dataType = tpHeader.dataType;
						mReadPacket->handleType = tpHeader.handleType;
						mReadPacket->level = tpHeader.level;
						mReadPacket->extend = tpHeader.extend;
						//将缓存2获取的完整packet赋值 end
						mCache2_size = 0;

						this->callReadPacketCallBack(mReadPacket);

					}
				}
				else {
					break;
				}
			}
			else {
				break;
			}
		}
	}
};
