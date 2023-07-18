#ifndef CLIENT_H
#define CLIENT_H
#include "../../include/BXC_TL.h"
#include <map>
#include <thread>
#include <mutex>

#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#pragma warning( disable : 4996 )

namespace BXC_TL {
	class ClientStream;
	class Client
	{
	public:
		Client(BXC_TLClientInfo* clientInfo);
		~Client();
	public:
		bool start();
		bool stop();
		ClientStream* getStream();
	private:
		static void loopThread(void* arg);
		bool loop();
		void handle(int fd);

		bool parseRequest();
		bool responseParams();
		bool responsePlay();

		int createTcpClientFd(const char* serverIp, int serverPort, const char* &clientIp, int& clientPort);
		int createUdpClientFd(const char* serverIp, int serverPort, int clientPort);

		int FD_ADD(int fd);
		void FD_REMOVE(int fd);

	private:
		bool mIsStop = true;
		std::thread* mThread = nullptr;

		int mMaxFd;
		fd_set mReadFds;

		ClientStream* mStream = nullptr;
	};

};

#endif //CLIENT_H
