#ifndef SERVER_H
#define SERVER_H
#include "../../include/BXC_TL.h"
#include <map>
#include <thread>
#include <mutex>

#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#pragma warning( disable : 4996 )

namespace BXC_TL {
	
	class ServerStream;
	class Server
	{
	public:
		Server(BXC_TLServerInfo* serverInfo, BXC_NewConnectionCallBack newConnCb);
		virtual ~Server();
	public:
		bool start();
		bool stop();
		ServerStream* getStream(int clientFd);
	private:
		static void loopThread(void* arg);
		bool loop();
		void handle(int fd);
		bool parseRequest(ServerStream* stream);
		bool requestParams(ServerStream* stream);
		bool requestPlay(ServerStream* stream);

		int createTcpServerFd(const char* ip, int port);
		int createUdpServerFd(const char* ip,int port);

		bool addStream(int clientFd, ServerStream* stream);
		bool removeStream(int clientFd);

		bool addTransportFdClientFd(int transportFd, int clientFd);
		int  getTransportFdClientFd(int transportFd);
		bool removeTransportFdClientFd(int transportFd);

		int FD_ADD(int fd);
		void FD_REMOVE(int fd);

	private:
		BXC_TLServerInfo*  mServerInfo = nullptr;
		bool mIsStop = true;
		std::thread* mThread = nullptr;

		int mMaxFd;
		fd_set mReadFds;


		BXC_NewConnectionCallBack  mNewConnectionCallBack = nullptr;	


		std::map<int, ServerStream*> mClientStream_Map; // <clientFd,ServerStream> 
		std::mutex                   mClientStream_Map_mtx;

		std::map<int, int>			 mTransportFdClientFd_Map; // <transportFd,clientFd> 
		std::mutex                   mTransportFdClientFd_Map_mtx;


	};

};

#endif //SERVER_H
