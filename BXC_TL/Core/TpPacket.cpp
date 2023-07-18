#include "TpPacket.h"

bool TpHeaderSerialize(TpHeader* tpHeader, uint8_t* data) {
	int pos = 0;

	// 1-4
	data[0] = tpHeader->flag1;
	data[1] = tpHeader->flag2;
	pos += 2;
	memcpy(data + pos, &(tpHeader->tpBodySize), 2);
	pos += 2;

	//5-8
	data[4] = tpHeader->dataType;
	data[5] = tpHeader->handleType;
	data[6] = tpHeader->level;
	data[7] = tpHeader->extend;
	pos += 4;

	//9-12
	memcpy(data + pos, &(tpHeader->subSeqNumber), 2);
	pos += 2;
	memcpy(data + pos, &(tpHeader->totalSubSeqNumber), 2);
	pos += 2;

	//13-16
	memcpy(data + pos, &(tpHeader->seqNumber), 4);
	pos += 4;

	//17-20
	memcpy(data + pos, &(tpHeader->timestamp), 4);
	pos += 4;

	return true;
}
bool TpHeaderDeSerialize(uint8_t* data, TpHeader* tpHeader) {
	int pos = 0;

	// 1-4
	tpHeader->flag1 = data[0];
	tpHeader->flag2 = data[1];
	pos += 2;
	memcpy(&(tpHeader->tpBodySize), data + pos, 2);
	pos += 2;

	//5-8
	tpHeader->dataType = data[4];
	tpHeader->handleType = data[5];
	tpHeader->level = data[6];
	tpHeader->extend = data[7];
	pos += 4;

	//9-12
	memcpy(&(tpHeader->subSeqNumber), data + pos, 2);
	pos += 2;
	memcpy(&(tpHeader->totalSubSeqNumber), data + pos, 2);
	pos += 2;

	//13-16
	memcpy(&(tpHeader->seqNumber), data + pos, 4);
	pos += 4;

	//17-20
	memcpy(&(tpHeader->timestamp), data + pos, 4);
	pos += 4;

	return true;
}