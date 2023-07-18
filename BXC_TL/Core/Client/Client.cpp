#include "Client.h"
#include "../Common.h"
#include "ClientStream.h"

namespace BXC_TL {

	Client::Client(BXC_TLClientInfo* clientInfo): mStream(new ClientStream(clientInfo))
	{
		LOGI("");
		mMaxFd = 0;
		FD_ZERO(&mReadFds);

	}
	Client::~Client()
	{
		LOGI("");
		//需要先调用stop()，才能够delete进行析构资源

		if (mStream->clientInfo->dataLocalFd > 0) {
			if (PROTOCOL_UDP == mStream->clientInfo->protocol) {
				this->FD_REMOVE(mStream->clientInfo->dataLocalFd);
			}
		}
		if (mStream->clientInfo->fd > 0) {
			this->FD_REMOVE(mStream->clientInfo->fd);
		}

		if (mStream) {
			delete mStream;
			mStream = nullptr;
		}
		WSACleanup();
	}

	bool Client::start() {

		LOGI("start connect Server://%s:%d", mStream->clientInfo->serverIp, mStream->clientInfo->serverPort);

		WSADATA wsaData;
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		{
			LOGE("WSAStartup error");
			return false;
		}
		const char* clientIp;
		int clientPort;

		int fd = createTcpClientFd(mStream->clientInfo->serverIp, mStream->clientInfo->serverPort, clientIp, clientPort);
		if (fd < 0) {
			return false;
		}
		mStream->clientInfo->fd = fd;
		mStream->clientInfo->ip = clientIp;
		mStream->clientInfo->port = clientPort;


		FD_ADD(mStream->clientInfo->fd);

		//开启loop线程
		mIsStop = false;
		mThread = new std::thread(Client::loopThread, this);
		return true;
	}
	bool Client::stop() {
		mIsStop = true;
		if (mThread) {
			mThread->join();
			//std::this_thread::sleep_for(std::chrono::milliseconds(1));
			delete mThread;
			mThread = nullptr;
		}

		return true;
	}
	ClientStream* Client::getStream() {
		return mStream;
	}
	void Client::loopThread(void* arg) {
		Client* client = (Client*)arg;
		client->loop();
	}
	bool Client::loop() {
		struct timeval timeout;
		timeout.tv_sec = 0;// 秒
		timeout.tv_usec = 0;//微秒

		int ret;
		while (true)
		{
			if (mIsStop) {
				break;
			}
			fd_set readFds;
			FD_ZERO(&readFds);

			readFds = mReadFds;
			ret = select(mMaxFd + 1, &readFds, nullptr, nullptr, &timeout);
			if (ret < 0) {
				//LOGE("未检测到活跃fd");
			}
			else {
				for (int fd = 3; fd < mMaxFd + 1; fd++)
				{
					if (FD_ISSET(fd, &readFds)) {
						handle(fd);
					}
				}
			}
		}

		return true;
	}
	void Client::handle(int fd) {
		char recvBuf[10000];
		int  recvBufSize;

		if (fd == mStream->clientInfo->fd) {
			// 传输协议为TCP
			if (TransportProcess_OK == mStream->getSt()) {
				recvBufSize = ::recv(mStream->clientInfo->fd, recvBuf, sizeof(recvBuf), 0);
				if (recvBufSize <= 0) {
					LOGE("tcp::recv fd=%d,recvBufSize=%d", mStream->clientInfo->fd, recvBufSize);
				}
				else {
					mStream->recvBuf((uint8_t*)recvBuf, recvBufSize);
				}
			}
			else {
				parseRequest();//流通道暂未建立
			}
		}
		else if (fd == mStream->clientInfo->dataLocalFd) {
			// 传输协议为UDP
			struct sockaddr_in peer_addr;
			int	               peer_addr_len = sizeof(peer_addr);

			recvBufSize = ::recvfrom(mStream->clientInfo->dataLocalFd, recvBuf, sizeof(recvBuf), 0, (struct sockaddr*)&peer_addr, &peer_addr_len);
			if (recvBufSize <= 0) {
				LOGE("udp::recvfrom fd=%d,recvBufSize=%d", mStream->clientInfo->dataLocalFd, recvBufSize);
			}
			else {
				if (!mStream->clientInfo->dataPeerIp) {
					mStream->clientInfo->dataPeerIp = inet_ntoa(peer_addr.sin_addr);
					mStream->clientInfo->dataPeerPort = ntohs(peer_addr.sin_port);
				}
				mStream->recvBuf((uint8_t*)recvBuf, recvBufSize);
			}
		}
	}
	bool Client::parseRequest() {
		char recvBuf[40];
		int  recvBufSize = 0;

		recvBufSize = ::recv(mStream->clientInfo->fd, recvBuf, 1, 0);
		if (1 == recvBufSize) {
			if (TransportProcess_PARAMS == recvBuf[0]) {
				//PARAMS请求

				recvBufSize = ::recv(mStream->clientInfo->fd, recvBuf, 39, 0);
				if (39 == recvBufSize) {

					//char dataPeerIp[20];//未使用
					int  dataPeerPort;

					//BXC_TransportProtocol protocol = (BXC_TransportProtocol)recvBuf[0];
					//memcpy(dataPeerIp, recvBuf + 1, 20);
					memcpy(&dataPeerPort, recvBuf + 21, 4);


					if (PROTOCOL_TCP == mStream->clientInfo->protocol) {

						mStream->clientInfo->dataLocalFd = mStream->clientInfo->fd;
						mStream->clientInfo->dataLocalIp = mStream->clientInfo->ip;
						mStream->clientInfo->dataLocalPort = mStream->clientInfo->port;

						mStream->clientInfo->dataPeerIp = mStream->clientInfo->serverIp;
						mStream->clientInfo->dataPeerPort = mStream->clientInfo->serverPort;
					
					}
					else if (PROTOCOL_UDP == mStream->clientInfo->protocol) {

						mStream->clientInfo->dataLocalIp = mStream->clientInfo->ip;//和主连接的本地IP保持一致
						mStream->clientInfo->dataLocalPort = dataPeerPort - 1;

						mStream->clientInfo->dataPeerIp = mStream->clientInfo->serverIp;
						mStream->clientInfo->dataPeerPort = dataPeerPort;

						mStream->clientInfo->dataLocalFd = createUdpClientFd(
							mStream->clientInfo->dataPeerIp, mStream->clientInfo->dataPeerPort, 
							mStream->clientInfo->dataLocalPort);



						FD_ADD(mStream->clientInfo->dataLocalFd);

					}
					else {
					
						return false;
					}
					mStream->setSt(TransportProcess_PLAY);
					responseParams();
				}
			}
			else if (TransportProcess_PLAY == recvBuf[0]) {
				//PLAY请求

				recvBufSize = ::recv(mStream->clientInfo->fd, recvBuf, 9, 0);
				if (9 == recvBufSize) {
					//连接建立成功

					LOGI("主连接和传输通道连接已接通：protocol=%d,serverIp=%s,serverPort=%d,fd=%d,ip=%s,port=%d,dataLocalFd=%d,dataLocalIp=%s,dataLocalPort=%d,dataPeerIp=%s,dataPeerPort=%d",
						mStream->clientInfo->protocol,mStream->clientInfo->serverIp, mStream->clientInfo->serverPort, mStream->clientInfo->fd, mStream->clientInfo->ip, mStream->clientInfo->port,
						mStream->clientInfo->dataLocalFd, mStream->clientInfo->dataLocalIp, mStream->clientInfo->dataLocalPort,
						mStream->clientInfo->dataPeerIp, mStream->clientInfo->dataPeerPort
					);

				}
				mStream->setSt(TransportProcess_OK);
				responsePlay();
			}
		}

		return true;
	
	}
	bool Client::responseParams() {
		
		char data[40];
		memset(data, 0, sizeof(data));
		int size = 0;

		data[0] = TransportProcess_PARAMS;
		size += 1;

		data[1] = mStream->clientInfo->protocol;
		size += 1;


		memcpy(data + size, mStream->clientInfo->dataLocalIp, 20);
		size += 20;
		
		memcpy(data + size, &(mStream->clientInfo->dataLocalPort), 4);
		size += 4;

		size += 14;//空余扩展


		int ret = ::send(mStream->clientInfo->fd, data, size, 0);
		if (ret < 0) {
			LOGE("error: %d", WSAGetLastError());
			return false;
		}

		return true;
	}
	bool Client::responsePlay() {
		char data[10];
		memset(data, 0, sizeof(data));
		int size = 0;

		data[0] = TransportProcess_PLAY;
		size += 1;

		size += 9;//空余扩展


		int ret = ::send(mStream->clientInfo->fd, data, size, 0);
		if (ret < 0) {
			LOGE("rerror: %d", WSAGetLastError());

			return false;
		}

		return true;
	}

	int Client::createTcpClientFd(const char* serverIp, int serverPort, const char*& clientIp, int& clientPort) {

		int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (fd < 0)
		{
			LOGE("create socket error");
			return -1;
		}
		int on = 1;
		setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on));

		//设置 client_addr
		
		sockaddr_in client_addr;
		client_addr.sin_family = AF_INET;
		client_addr.sin_addr.S_un.S_addr = INADDR_ANY;
		client_addr.sin_port = htons(getRandomPort());
		//inet_pton(AF_INET, clientIp, &client_addr.sin_addr);
		int client_addr_len = sizeof(sockaddr_in);

		if (bind(fd, (LPSOCKADDR)&client_addr, client_addr_len) == -1)
		{
			LOGE("bind socket error");
			return -1;
		}
		clientIp = inet_ntoa(client_addr.sin_addr);
		clientPort = ntohs(client_addr.sin_port);

		LOGI("serverIp=%s,serverPort=%d,clientIp=%s,clientPort=%d", 
			serverIp, serverPort,
			clientIp, clientPort);

		//设置 server_addr
		struct sockaddr_in server_addr;
		server_addr.sin_family = AF_INET;
		server_addr.sin_port = htons(serverPort);
		//server_addr.sin_addr.s_addr = inet_addr(serverIp);
		inet_pton(AF_INET, serverIp, &server_addr.sin_addr);
		int server_addr_len = sizeof(sockaddr_in);

		if (connect(fd, (struct sockaddr*)&server_addr, server_addr_len) == -1)
		{
			LOGE("connect error");
			return -1;
		}
		else {
			LOGI("connect success fd=%d", fd);
		}
		return fd;
	}
	int Client::createUdpClientFd(const char* serverIp, int serverPort, int clientPort) {
		int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (fd < 0)
		{
			LOGE("create socket error");
			return -1;
		}
		int on = 1;
		setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on));

		//设置 client_addr
		sockaddr_in client_addr;
		client_addr.sin_family = AF_INET;
		client_addr.sin_addr.S_un.S_addr = INADDR_ANY;
		client_addr.sin_port = htons(clientPort);
		//inet_pton(AF_INET, clientIp, &client_addr.sin_addr);
		int client_addr_len = sizeof(sockaddr_in);

		if (bind(fd, (LPSOCKADDR)&client_addr, client_addr_len) == -1)
		{
			LOGE("bind socket error");
			return -1;
		}
		const char* re_clientIp = inet_ntoa(client_addr.sin_addr);
		int re_clientPort = ntohs(client_addr.sin_port);

		LOGI("serverIp=%s,serverPort=%d,clientPort=%d,re_clientIp=%s,re_clientPort=%d",
			serverIp, serverPort,clientPort, 
			re_clientIp, re_clientPort);

		return fd;
	}
	void Client::FD_REMOVE(int fd) {
		FD_CLR(fd, &mReadFds); //从可读集合中删除
	}
	int Client::FD_ADD(int fd) {

		//将fd 添加进入集合内，并更新最大文件描述符
		FD_SET(fd, &mReadFds);
		mMaxFd = mMaxFd > fd ? mMaxFd : fd;

		return mMaxFd;
	}
}