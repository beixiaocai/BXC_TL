#ifndef TPPACKET_H
#define TPPACKET_H

#include <string>

#define TP_HEADER_SIZE      20
#define TP_BODY_MAX_SIZE    1400
#define TP_HEADER_FLAG1     '@'
#define TP_HEADER_FLAG2     '@'


/*
 *    0                   1                   2                   3
 *    7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |	    flag1    |    flag2      |	     packet body size        |
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |	 data type   | 	handle type  |	   level     |     extend    |
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |      sub sequence number      |   total sub sequence number   |
 * 	 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |					       sequence number                       |
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |							 timestamp
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
struct TpHeader {
    /* byte 0 */
    uint8_t flag1;
    /* byte 1 */
    uint8_t flag2;
    /* bytes 2,3 */
    uint16_t tpBodySize;

    /* byte 4 */
    uint8_t dataType;
    /* byte 5 */
    uint8_t handleType;
    /* byte 6 */
    uint8_t level;
    /* byte 7 */
    uint8_t extend;

    /* bytes 8,9 */
    uint16_t subSeqNumber;
    /* bytes 10,11 */
    uint16_t totalSubSeqNumber;

    /* bytes 12,13,14,15 */
    uint32_t seqNumber;

    /* bytes 16,17,18,19 */
    uint32_t timestamp;
};
struct TpPacket
{
    TpHeader tpHeader;
    uint8_t* tpBodyData;
};
bool TpHeaderSerialize(TpHeader* tpHeader, uint8_t* data);//将tpHeader序列化到data
bool TpHeaderDeSerialize(uint8_t* data, TpHeader* tpHeader);//将data反序列号到tpHeader



#endif //TPPACKET_H