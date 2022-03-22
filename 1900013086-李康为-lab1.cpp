// 滑动窗口协议实验 李康为 1900013086

#include "sysinclude.h"
#include <queue>

extern void SendFRAMEPacket(unsigned char* pData, unsigned int len);

#define WINDOW_SIZE_STOP_WAIT 1
#define WINDOW_SIZE_BACK_N_FRAME 4

typedef enum{data, ack, nak} frame_kind;

typedef struct frame_head
{
	frame_kind kind;			// 帧类型 
	unsigned int seq;			// 序列号 
	unsigned int ack;			// 确认号 
	unsigned char data[100];	// 数据 
};

typedef struct frame
{
	frame_head head;			// 帧头 
	unsigned int size;			// 数据的大小 
};

queue<frame> sendq;
deque<frame> slides;

/*
* 停等协议测试函数
*/
int stud_slide_window_stop_and_wait(char *pBuffer, int bufferSize, UINT8 messageType)
{
	frame buffer;
	static bool flag = true;
	if (messageType == MSG_TYPE_SEND) {
		memcpy(&buffer, pBuffer, sizeof(buffer));
		buffer.size = bufferSize;
		sendq.push(buffer);

		if (flag) {
			flag = false;
			buffer = sendq.front();
			SendFRAMEPacket((unsigned char*)(&buffer), bufferSize);
		}
	} else if (messageType == MSG_TYPE_RECEIVE) {
		flag = true;
		sendq.pop();
		if (!sendq.empty()) {
			flag = false;
			buffer = sendq.front();
			SendFRAMEPacket((unsigned char*)(&buffer), buffer.size);
		}
	} else if (messageType == MSG_TYPE_TIMEOUT) {
		flag = false;
		buffer = sendq.front();
		SendFRAMEPacket((unsigned char*)(&buffer), buffer.size);
	} else {
		return -1;
	} 
	return 0;
}

/*
* 回退n帧测试函数
*/
int stud_slide_window_back_n_frame(char *pBuffer, int bufferSize, UINT8 messageType)
{
	frame buffer;
	if (messageType == MSG_TYPE_SEND) {
		memcpy(&buffer, pBuffer, sizeof(buffer));
		buffer.size = bufferSize;
		sendq.push(buffer);

		if (slides.size() < WINDOW_SIZE_BACK_N_FRAME) {
			buffer = sendq.front();
			slides.push_back(buffer);
			SendFRAMEPacket((unsigned char*)(&buffer), bufferSize);
			sendq.pop();
		}
	} else if (messageType == MSG_TYPE_RECEIVE) {
		memcpy(&buffer, pBuffer, sizeof(buffer));
		while (ntohl(slides.begin()->head.seq) != ntohl(buffer.head.ack) && !slides.empty()) {
			slides.pop_front();
		}
		slides.pop_front();

		while (slides.size() < WINDOW_SIZE_BACK_N_FRAME && !sendq.empty()) {
			buffer = sendq.front();
			slides.push_back(buffer);
			SendFRAMEPacket((unsigned char*)(&buffer), buffer.size);
			sendq.pop();
		}
	} else if (messageType == MSG_TYPE_TIMEOUT) {
		for (deque<frame>::iterator iter = slides.begin(); iter != slides.end(); ++iter) {
			SendFRAMEPacket((unsigned char*)&(*iter), iter->size);
		}
	} else {
		return -1;
	}
	return 0;
}

/*
* 选择性重传测试函数
*/
int stud_slide_window_choice_frame_resend(char *pBuffer, int bufferSize, UINT8 messageType)
{
	frame buffer;
	if (messageType == MSG_TYPE_SEND) {
		memcpy(&buffer, pBuffer, sizeof(buffer));
		buffer.size = bufferSize;
		sendq.push(buffer);

		if (slides.size() < WINDOW_SIZE_BACK_N_FRAME) {
			buffer = sendq.front();
			slides.push_back(buffer);
			SendFRAMEPacket((unsigned char*)(&buffer), bufferSize);
			sendq.pop();
		}
	} else if (messageType == MSG_TYPE_RECEIVE) {
		memcpy(&buffer, pBuffer, sizeof(buffer));

		if (ntohl(buffer.head.kind) == ack) {
			while (ntohl(slides.begin()->head.seq) != ntohl(buffer.head.ack)) {
				slides.pop_front();
			}
			slides.pop_front();
		} else {
			for (deque<frame>::iterator iter = slides.begin(); iter != slides.end(); ++iter) {
				if (ntohl(buffer.head.ack) == ntohl(iter->head.seq)) {
					SendFRAMEPacket((unsigned char*)&(*iter), iter->size);
					break;
				}
			}
		}
		while (slides.size() < WINDOW_SIZE_BACK_N_FRAME && !sendq.empty())	{
			buffer = sendq.front();
			slides.push_back(buffer);
			SendFRAMEPacket((unsigned char*)(&buffer), buffer.size);
			sendq.pop();
		}
	} else if (messageType == MSG_TYPE_TIMEOUT) {
		for (deque<frame>::iterator iter = slides.begin(); iter != slides.end(); ++iter) {
			if (ntohl(*pBuffer) == ntohl(iter->head.seq)) {
				SendFRAMEPacket((unsigned char*)(&(*iter)), iter->size);
				break;
			}
		}
	} else {
		return -1;
	}
	return 0;
}
