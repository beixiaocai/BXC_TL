#include "Server.h"
#include "../Common.h"
#include "ServerStream.h"

namespace BXC_TL {
	Server::Server(BXC_TLServerInfo* serverInfo, BXC_NewConnectionCallBack newConnCb) :mServerInfo(serverInfo), mNewConnectionCallBack(newConnCb)
	{
		LOGI("");
		mMaxFd = 0;
		FD_ZERO(&mReadFds);
	}
	Server::~Server()
	{
		LOGI("");
		//需要先调用stop()，才能够delete进行析构资源
		//TODO 需要清理存在的连接

		if (mServerInfo) {
			if (mServerInfo->fd > 0) {
				FD_REMOVE(mServerInfo->fd);
				closesocket(mServerInfo->fd);
				mServerInfo->fd = -1;
			}
		}


		WSACleanup();
	}
	bool Server::start() {

		LOGI("Server://%s:%d", mServerInfo->ip, mServerInfo->port);

		WSADATA wsaData;
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		{
			LOGE("WSAStartup error");
			return false;
		}
		int serverFd = createTcpServerFd(mServerInfo->ip, mServerInfo->port);
		if (serverFd < 0) {
			return false;
		}
		mServerInfo->fd = serverFd;

		this->FD_ADD(serverFd);

		//开启loop线程
		mIsStop = false;
		mThread = new std::thread(Server::loopThread, this);
		return true;
	}
	bool Server::stop() {
		mIsStop = true;
		if (mThread) {
			mThread->join();
			//std::this_thread::sleep_for(std::chrono::milliseconds(1));
			delete mThread;
			mThread = nullptr;
		}

		return true;

	}
	void Server::loopThread(void* arg) {
		Server* server = (Server*)arg;
		server->loop();
	}
	bool Server::loop() {
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
	void Server::handle(int fd) {
		char recvBuf[10000];
		int  recvBufSize;

		if (fd == mServerInfo->fd) {
			LOGI("fd=%d可读，发现新连接...", fd);

			struct sockaddr_in peer_addr;
			int	               peer_addr_len = sizeof(peer_addr);

			int clientFd = accept(mServerInfo->fd, (SOCKADDR*)&peer_addr, &peer_addr_len);

			const char* peerIp = inet_ntoa(peer_addr.sin_addr);
			int			peerPort = ntohs(peer_addr.sin_port);

			if (clientFd == SOCKET_ERROR) {
				LOGE("accept error");
				return;
			}
			this->FD_ADD(clientFd);

			//创建一个未确认传输协议的流
			ServerStream *stream = new ServerStream(new BXC_TLServerConnectionInfo(
				mServerInfo, PROTOCOL_UNKNOWN,
				clientFd, peerIp, peerPort,
				-1, nullptr, -1,
				nullptr, -1
			));

			addStream(clientFd,stream);
			requestParams(stream);
		}
		else {
			ServerStream* stream = this->getStream(fd);
			if (stream) {//传输主连接所属的描述符
				int clientFd = fd;
				if (TransportProcess_OK != stream->getSt()) {//流通道暂未成功
					parseRequest(stream);
				}
				else {
					recvBufSize = ::recv(clientFd, recvBuf, sizeof(recvBuf), 0);
					if (recvBufSize <= 0) {
						LOGE("tcp::recv fd=%d,recvBufSize=%d", clientFd, recvBufSize);
						this->removeStream(clientFd);
					}
					else {
						stream->recvBuf((uint8_t*)recvBuf, recvBufSize);
					}
				}
			}
			else {//传输通道所属的描述符
				int transportFd = fd;
				int clientFd = getTransportFdClientFd(transportFd);
				if (clientFd  > 0) {
					stream = this->getStream(clientFd);
					if (PROTOCOL_UDP == stream->serverConnInfo->protocol) {

						struct sockaddr_in peer_addr;
						int	               peer_addr_len = sizeof(peer_addr);
						recvBufSize = ::recvfrom(stream->serverConnInfo->dataLocalFd, recvBuf, sizeof(recvBuf), 0, (struct sockaddr*)&peer_addr, &peer_addr_len);
						
						if (recvBufSize <= 0) {
							LOGE("udp::recvfrom fd=%d,recvBufSize=%d", stream->serverConnInfo->dataLocalFd, recvBufSize);
						}
						else {
							//if (!stream->serverConnInfo->dataPeerIp) {
							//	stream->serverConnInfo->dataPeerIp = inet_ntoa(peer_addr.sin_addr);
							//	stream->serverConnInfo->dataPeerPort = ntohs(peer_addr.sin_port);
							//}
							stream->recvBuf((uint8_t*)recvBuf, recvBufSize);
						}

					}
					else {
						LOGE("TODO 当前描述符transportFd=%d，对应的传输通道不支持协议protocol=%d", transportFd, stream->serverConnInfo->protocol);
					}

				}
				else {
					LOGE("TODO 当前描述符transportFd=%d，未匹配到所属传输通道，需要立即删除", transportFd);
				}
			}

		}
	}
	bool Server::parseRequest(ServerStream* stream) {
		LOGI("");
		char recvBuf[40];
		int  recvBufSize = 0;
		recvBufSize = ::recv(stream->serverConnInfo->fd, recvBuf, 1, 0);
		if (1 != recvBufSize) {
			LOGE("tcp::recv fd=%d,recvBufSize=%d", stream->serverConnInfo->fd, recvBufSize);
			this->removeStream(stream->serverConnInfo->fd);
			return false;
		}
		else{
			if (TransportProcess_PARAMS == recvBuf[0]) {
				//PARAMS响应

				recvBufSize = ::recv(stream->serverConnInfo->fd, recvBuf, 39, 0);
				if (39 == recvBufSize) {
					//char dataPeerIp[20];//未使用
					int  dataPeerPort;
					BXC_TLProtocol protocol = (BXC_TLProtocol)recvBuf[0];

					//memcpy(dataPeerIp, recvBuf+1, 20);
					memcpy(&dataPeerPort, recvBuf + 21, 4);
					stream->serverConnInfo->protocol = protocol;

					if (PROTOCOL_TCP == stream->serverConnInfo->protocol) {

						stream->serverConnInfo->dataLocalFd = stream->serverConnInfo->fd;
						//stream->serverConnInfo->dataLocalIp //TCP已经设置
						stream->serverConnInfo->dataLocalPort = stream->serverConnInfo->serverInfo->port;//TCP重新设置

						stream->serverConnInfo->dataPeerIp = stream->serverConnInfo->peerIp;
						stream->serverConnInfo->dataLocalPort = stream->serverConnInfo->peerPort;

					}
					else if (PROTOCOL_UDP == stream->serverConnInfo->protocol) {
						//stream->conn->dataLocalIp //UDP已经设置
						//stream->conn->dataLocalPort//UDP已经设置

						stream->serverConnInfo->dataLocalFd = createUdpServerFd(
							stream->serverConnInfo->dataLocalIp,
							stream->serverConnInfo->dataLocalPort);

						stream->serverConnInfo->dataPeerIp = stream->serverConnInfo->peerIp;
						stream->serverConnInfo->dataPeerPort = dataPeerPort;

						this->FD_ADD(stream->serverConnInfo->dataLocalFd);
						this->addTransportFdClientFd(stream->serverConnInfo->dataLocalFd, stream->serverConnInfo->fd);

					}
					else {
						LOGE("未知的传输协议 protocol=%d", stream->serverConnInfo->protocol);
						return false;
					}
					stream->setSt(TransportProcess_PLAY);
					requestPlay(stream);
				}
			}
			else if (TransportProcess_PLAY == recvBuf[0]) {
				//PLAY响应
				recvBufSize = ::recv(stream->serverConnInfo->fd, recvBuf, 9, 0);
				if (9 == recvBufSize) {

					LOGI("主连接和传输通道连接已接通：fd=%d,peerIp=%s,peerPort=%d,dataLocalFd=%d,dataLocalIp=%s,dataLocalPort=%d,dataPeerIp=%s,dataPeerPort=%d",
						stream->serverConnInfo->fd,stream->serverConnInfo->peerIp,stream->serverConnInfo->peerPort,
						stream->serverConnInfo->dataLocalFd, stream->serverConnInfo->dataLocalIp, stream->serverConnInfo->dataLocalPort,
						stream->serverConnInfo->dataPeerIp, stream->serverConnInfo->dataPeerPort
						);
					stream->setSt(TransportProcess_OK);
					mNewConnectionCallBack(stream->serverConnInfo);//回调通知产生了新连接

				}
			}
			else {
				LOGE("未定义的请求类型");
				return false;
			}
		}

		return true;
	}
	bool Server::requestParams(ServerStream* stream) {
		LOGI("");

		char data[40];
		memset(data, 0, sizeof(data));
		int size = 0;

		data[0] = TransportProcess_PARAMS;
		size += 1;

		data[1] = stream->serverConnInfo->protocol;
		size += 1;

		stream->serverConnInfo->dataLocalIp = stream->serverConnInfo->serverInfo->ip;  //（UDP或TCP）传输服务的IP
		stream->serverConnInfo->dataLocalPort = getRandomPort();//（UDP）传输服务的端口

		memcpy(data + size, stream->serverConnInfo->dataLocalIp, 20);
		size += 20;

		memcpy(data + size, &(stream->serverConnInfo->dataLocalPort), 4);
		size += 4;

		size += 14;//空余扩展


		int ret = ::send(stream->serverConnInfo->fd, data, size, 0);
		if (ret < 0) {
			LOGE("error: %d", WSAGetLastError());
			return false;
		}

		return true;
	}
	bool Server::requestPlay(ServerStream* stream) {
		LOGI("");
		char data[10];
		memset(data, 0, sizeof(data));
		int size = 0;

		data[0] = TransportProcess_PLAY;
		size += 1;

		size += 9;//空余扩展

		int ret = ::send(stream->serverConnInfo->fd, data, size, 0);
		if (ret < 0) {
			LOGE("error: %d", WSAGetLastError());
			return false;
		}

		return true;
	}

	int Server::createTcpServerFd(const char* ip, int port) {
		SOCKADDR_IN server_addr;
		int			server_addr_len = sizeof(SOCKADDR);

		server_addr.sin_family = AF_INET;
		//server_addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
		server_addr.sin_addr.s_addr = inet_addr(ip);
		server_addr.sin_port = htons(port);

		SOCKET fd;
		fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

		if (fd < 0)
		{
			LOGE("create socket error");
			return -1;
		}
		int on = 1;
		setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on));

		if (bind(fd, (SOCKADDR*)&server_addr, server_addr_len) == SOCKET_ERROR) {
			LOGE("bind error");
			return -1;
		}

		if (listen(fd, SOMAXCONN) < 0) {
			LOGE("listen error");
			return -1;
		}
		return fd;

	}
	int Server::createUdpServerFd(const char* ip, int port) {
	
		SOCKADDR_IN server_addr;
		int			server_addr_len = sizeof(SOCKADDR);

		server_addr.sin_family = AF_INET;
		//server_addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
		server_addr.sin_addr.s_addr = inet_addr(ip);
		server_addr.sin_port = htons(port);

		SOCKET fd;

		fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (fd < 0)
		{
			LOGE("create socket error");
			return -1;
		}
		int on = 1;
		setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on));

		if (bind(fd, (SOCKADDR*)&server_addr, server_addr_len) == SOCKET_ERROR) {
			LOGE("bind error");
			return -1;
		}
		return fd;
	
	}
	bool Server::addStream(int clientFd, ServerStream* stream) {
		mClientStream_Map_mtx.lock();
		bool isAdd = mClientStream_Map.end() != mClientStream_Map.find(clientFd);
		if (!isAdd) {
			//未添加
			mClientStream_Map.insert(std::make_pair(clientFd, stream));
			mClientStream_Map_mtx.unlock();

			return true;
		}
		mClientStream_Map_mtx.unlock();

		return false;
	}
	bool Server::removeStream(int clientFd) {
		mClientStream_Map_mtx.lock();

		auto f = mClientStream_Map.find(clientFd);
		if (mClientStream_Map.end() != f) {
			//已添加
			ServerStream* stream = f->second;

			if (stream->serverConnInfo->dataLocalFd > 0 ) {
				if (PROTOCOL_UDP == stream->serverConnInfo->protocol) {
					this->removeTransportFdClientFd(stream->serverConnInfo->dataLocalFd);
					this->FD_REMOVE(stream->serverConnInfo->dataLocalFd);
				}
			}
			if (stream->serverConnInfo->fd > 0) {
				this->FD_REMOVE(stream->serverConnInfo->fd);
			}

			delete stream;
			stream = nullptr;
			
			mClientStream_Map.erase(clientFd);
			mClientStream_Map_mtx.unlock();
			return true;
		}
		mClientStream_Map_mtx.unlock();
		return false;
	
	}
	ServerStream* Server::getStream(int clientFd) {
		ServerStream* stream = nullptr;
		mClientStream_Map_mtx.lock();
		for (auto f = mClientStream_Map.begin(); f != mClientStream_Map.end(); ++f) {
			if (clientFd == f->first) {
				stream = f->second;
				break;
			}
		}
		mClientStream_Map_mtx.unlock();
		return stream;
	
	}




	bool Server::addTransportFdClientFd(int transportFd, int clientFd) {
		mTransportFdClientFd_Map_mtx.lock();
		bool isAdd = mTransportFdClientFd_Map.end() != mTransportFdClientFd_Map.find(transportFd);
		if (!isAdd) {
			//未添加
			mTransportFdClientFd_Map.insert(std::make_pair(transportFd, clientFd));
			mTransportFdClientFd_Map_mtx.unlock();
			return true;
		}
		mTransportFdClientFd_Map_mtx.unlock();
	
		return false;
	}
	int  Server::getTransportFdClientFd(int transportFd) {
		int clientFd = -1;
		mTransportFdClientFd_Map_mtx.lock();
		for (auto f = mTransportFdClientFd_Map.begin(); f != mTransportFdClientFd_Map.end(); ++f) {
			if (transportFd == f->first) {
				clientFd = f->second;
				break;
			}
		}
		mTransportFdClientFd_Map_mtx.unlock();

		return clientFd;
	}
	bool Server::removeTransportFdClientFd(int transportFd) {

		mTransportFdClientFd_Map_mtx.lock();
		auto f = mTransportFdClientFd_Map.find(transportFd);
		if (mTransportFdClientFd_Map.end() != f) {
			//已添加
			mTransportFdClientFd_Map.erase(f);
			mTransportFdClientFd_Map_mtx.unlock();
			return true;
		}
		mTransportFdClientFd_Map_mtx.unlock();
		return false;
	
	}
	void Server::FD_REMOVE(int fd) {
		FD_CLR(fd, &mReadFds); //从可读集合中删除
	}
	int Server::FD_ADD(int fd) {

		//将fd 添加进入集合内，并更新最大文件描述符
		FD_SET(fd, &mReadFds);
		mMaxFd = mMaxFd > fd ? mMaxFd : fd;

		return mMaxFd;
	}
}