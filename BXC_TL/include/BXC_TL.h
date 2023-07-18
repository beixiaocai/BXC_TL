#ifndef BXC_TL_H
#define BXC_TL_H
#include <string>

#ifdef BXC_TL_EXPORTS
#define __DECLSPEC_INC __declspec(dllexport)
#else
#define __DECLSPEC_INC __declspec(dllimport)
#endif //!BXC_TL_EXPORTS

#ifndef BXC_TLPACKET_MAX_SIZE
// 1M = 1 * 1024 * 1024 = 1048576 字节
// 4M = 4194304 字节
#define BXC_TLPACKET_MAX_SIZE 4194304 //默认BXC_TLPacket的最大包长度是4M，如需调整，修改宏即可
#endif //!BXC_TLPACKET_MAX_SIZE


namespace BXC_TL {

#ifdef __cplusplus
	extern "C" {
#endif
		enum BXC_TLProtocol {
			PROTOCOL_UNKNOWN = 1,
			PROTOCOL_TCP,
			PROTOCOL_UDP
		};
		struct BXC_TLServerInfo {
		public:
			BXC_TLServerInfo() = delete;
			BXC_TLServerInfo(const char* ip, int port) {
				this->ip = ip;
				this->port = port;
			}
			~BXC_TLServerInfo() {
			}
		public:
			int id;
			const char* ip;
			int port;

			int fd;
		};

		struct BXC_TLServerConnectionInfo {
			BXC_TLServerConnectionInfo() = delete;
			BXC_TLServerConnectionInfo(BXC_TLServerInfo* serverInfo, BXC_TLProtocol protocol,int fd,const char* peerIp,int peerPort,
				int dataLocalFd, const char* dataLocalIp, int dataLocalPort,
				const char* dataPeerIp, int dataPeerPort
				) {
				this->serverInfo = serverInfo;
				this->protocol = protocol;

				this->fd = fd;

				this->peerIp = peerIp;
				this->peerPort = peerPort;

				this->dataLocalFd = dataLocalFd;
				this->dataLocalIp = dataLocalIp;
				this->dataLocalPort = dataLocalPort;

				this->dataPeerIp = dataPeerIp;
				this->dataPeerPort = dataPeerPort;
			}

			~BXC_TLServerConnectionInfo() {}
			BXC_TLServerInfo* serverInfo;
			BXC_TLProtocol protocol;

			int			fd;  //主连接描述符
			const char* peerIp;   //主连接对端IP
			int         peerPort; //主连接对端端口

			int         dataLocalFd;  //数据传输本地描述符
			const char* dataLocalIp;  //数据传输本地IP
			int         dataLocalPort;//数据传输本地端口

			const char* dataPeerIp;  //数据传输对端IP
			int			dataPeerPort;//数据传输对端端口
		};

		struct BXC_TLPacket
		{
		public:
			BXC_TLPacket() = delete;
			BXC_TLPacket(int size) {
				this->size = size;
				this->data = (uint8_t *) malloc(this->size);
			}
			~BXC_TLPacket() {
				free(this->data);
				this->data = nullptr;
			}
		public:
			int size;          //字节流长度
			uint8_t* data;     //字节流
			uint8_t  dataType;  //数据类型
			uint8_t  handleType;//操作类型
			uint8_t  level;     //传输级别
			uint8_t  extend;    //扩展
		};

		struct BXC_TLClientInfo {
		public:
			BXC_TLClientInfo() = delete;
			BXC_TLClientInfo(BXC_TLProtocol protocol, const char* serverIp, int serverPort) {
				this->protocol = protocol;
				this->serverIp = serverIp;
				this->serverPort = serverPort;
			}
			~BXC_TLClientInfo() {
			}
		public:
			int id;
			BXC_TLProtocol protocol;
			const char* serverIp;
			int		    serverPort;

			int			fd;
			const char* ip;
			int         port;

			int         dataLocalFd;
			const char* dataLocalIp;
			int         dataLocalPort;

			const char* dataPeerIp;
			int         dataPeerPort;
		};

		typedef void (*BXC_NewConnectionCallBack)(BXC_TLServerConnectionInfo* conn);
		typedef void (*BXC_ReadPacketCallBack)(void * arg, BXC_TLPacket* packet);

#ifdef __cplusplus
	}
#endif
}
#endif //BXC_TL_H
