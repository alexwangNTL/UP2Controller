// IntelProgram.cpp : �������̨Ӧ�ó������ڵ㡣
//
#pragma warning(disable:4996)
#pragma comment(lib, "ws2_32.lib")

#include "stdafx.h"
#using "Car.dll"

#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"

#include <math.h>
#include <fstream> 
#include <iostream>
#include <conio.h>
#include <time.h>
#include <WINSOCK2.h>
#include <STDIO.H>
#include <Ws2tcpip.h>
#include <windows.h>
#include <string>
#include <vector>

#include "FPGAController.h"
#include "ThreadController.h"
#include "MoveDetector.h"

using namespace Car;
using namespace std;
using namespace cv;

#define _WINSOCK_DEPRECATED_NO_WARNINGS 0
#define SERVERIP "192.168.0.100"
#define CAMERAIP "rtmp://192.168.0.101/live/insta360"

int timeout = 1; //ms

				  //�������ڲ�����
				  //��0�Ⱦ���Ϊ��׼����Ϊ��������Ϊ����
				  //�趫��Ϊ��������Ϊ�������ɣ�������


				  //���ȵ����
#define Longitude_Error 20

				  //ǰ������ת���ٶ�
#define fs 3	//forward speed 
#define ts 2	//turn speed

				  //С���������˻�ǰ����ʱ�䣨����Ϊ��λ)
#define actime 1000

				  //С��״̬��
#define STAY 0		//���ֲ���
#define FORWARD 1	//��ǰ�ƶ�
#define BACK 2		//����ƶ�
#define ROTATE 3	//ԭ����ת
#define REMOTE 4	//Զ�̿���

				  //ͼ������ĺ궨��
#define PI 3.141592
#define DEPTH 422500     /*������ȣ����洢��Ԫ�ĸ���*/
#define WIDTH 10       /*�洢��Ԫ�Ŀ��*/
#define RADIUS 640
#define DIAMETER 1280



double opencvRed(IplImage*);


double opencvRed(IplImage* hsv) {
	bool over = 0;

	CvScalar s_hsv;
	for (int i = 0; i < 1280; i += 32)
		for (int j = 0; j < 2560; j += 32)
		{
			for (int k = 0; k <32 && over == 0; k++)
				for (int m = 0; m < 32 && over == 0; m++)
				{
					s_hsv = cvGet2D(hsv, i + k, j + m);
					if ((!(((s_hsv.val[0]>0) && (s_hsv.val[0]<8)) || (s_hsv.val[0]>120) && (s_hsv.val[0]<180))) || s_hsv.val[1] < 80 || s_hsv.val[2] < 50 || s_hsv.val[2] > 220)
					{
						//s.val[0] = 0;
						//s.val[1] = 0;
						//s.val[2] = 0;
						//cvSet2D(hsv, i, j, s);
						over = 1;
					}
				}

			if (over == 0) {
				//cout << j  << endl;
				return (j + 16);
			}
			else
				over = 0;

		}
	return -1;
}



double opencvFire(IplImage*);

double opencvFire(IplImage* stretch) {
	bool over = 0;

	uchar* data = (uchar*)stretch->imageData;
	int step = stretch->widthStep;

	int number = 0, r, g, b = 0;
	double S, maxv, minv;

	for (int i = DIAMETER / 2; i < DIAMETER; i += 32)
		for (int j = 0; j < DIAMETER * 2; j += 32)
		{
			for (int k = 0; k <32 && over == 0; k++)
				for (int m = 0; m < 32 && over == 0; m++)
				{

					r = data[(i + k)*step + (j + m) * 3 + 2];
					g = data[(i + k)*step + (j + m) * 3 + 1];
					b = data[(i + k)*step + (j + m) * 3];
					maxv = max(max(b, g), r);
					minv = min(min(b, g), r);
					S = (maxv - minv) / maxv;
					if ((r > g) && (g > b) && (r > 100) && (S >((255 - r) * 2.5 / 100)))
					{
						number++;
						if (number > 550)
							over = 1;
					}
				}
			number = 0;

			if (over == 1) {
				//cout << j  << endl;
				return j;
			}


		}
	return -1;
}

// ���߳�ȫ�ֱ���
extern VideoCapture globalCapture;
extern vector<Mat> globalFrames;
extern int lastestFrame;
extern int oldFrame;

//��Ŀ������
int main()
{
	FPGA_Open();
	char *controllFlag = new char[128];
	FPGAOutData *outData = new FPGAOutData();
	bool FPGA_Working = false;

	//init������Ŀ��ʹ��mode3�����С���ƶ�
	PcanControl ^car = gcnew PcanControl();
	//���ü��ٶȺͼ��ٶ�
	car->SetAcceleratedSpeed(0.2, 0.2);
	car->SetDeceleratedSpeed(-0.2, -0.2);
	car->SetMode(car->Mode3);

	//Web service init
	WORD sockVersion = MAKEWORD(2, 2);
	WSADATA initdata;
	if (WSAStartup(sockVersion, &initdata) != 0) {
		FPGA_Close();
		car->Stop();
		return 0;
	}

	SOCKET sclient = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sclient == INVALID_SOCKET) {
		printf("invalid socket!");
		FPGA_Close();
		car->Stop();
		return 0;
	}

	int iMode = 1;
	int retVal = ioctlsocket(sclient, FIONBIO, (u_long FAR*)&iMode);
	if (retVal == SOCKET_ERROR) {
		printf("ioctlsocket wrong!");
		FPGA_Close();
		car->Stop();
		return 0;
	}

	sockaddr_in serAddr;
	serAddr.sin_family = AF_INET;
	serAddr.sin_port = htons(8888);
	serAddr.sin_addr.S_un.S_addr = inet_addr(SERVERIP);
	while (true) {
		retVal = connect(sclient, (sockaddr *)&serAddr, sizeof(serAddr));
		if (retVal == SOCKET_ERROR) {
			int err = WSAGetLastError();
			if (err == WSAEWOULDBLOCK || err == WSAEINVAL) {
				Sleep(500);
				continue;
			}
			else if (err == WSAEISCONN) {
				break;
			}
			else {
				printf("connect error\n");
				closesocket(sclient);
				WSACleanup();
				FPGA_Close();
				return -1;
			}
		}
	}

	const char * sendData = "���,����UP2!\n";
	send(sclient, sendData, strlen(sendData), 0);

	char *sdata;
	char cmd;
	char recvData[1];	//�յ�����Ϣֻ��һ����ĸ��wsadqe
	int recvtimeoutchange = setsockopt(sclient, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));


	//��û�м�⵽
	bool detected = false;
	//��⵽���Ǻ�(1)������(0)
	bool danger = true;

	//С��Ŀǰ��״̬
	int state = 4;

	//С���Ŀ���mode
	//1,Զ�̿��ƣ�2������ƣ����߷ֿ�,Ĭ������������
	bool remote_mode = true;

	//״̬����
	vector<string> debugtest;
	debugtest.push_back("����");
	debugtest.push_back("��ǰ");
	debugtest.push_back("���");
	debugtest.push_back("��ת");
	debugtest.push_back("ң��");

	//�˲���data123���α������ε�ֵ��ÿ�������ʱ������������ľ�ֵ
	double data1 = 0;
	double data2 = 0;

	//the detected thing's longitude
	double longitude = 0;

	string temps;

	//ͼ�����ֵ�init
	double rotation = 0;
	IplImage* output = cvCreateImage(cvSize(2560, 1280), 8, 3);

	IplImage* outputOld = cvCreateImage(cvSize(2560, 1280), 8, 3);

	IplImage* hsv = cvCreateImage(cvSize(2560, 1280), 8, 3);

	uchar* data; uchar* dataOld;

	uchar* dataOut = (uchar *)output->imageData;

	uchar* dataOutOld = (uchar *)outputOld->imageData;

	unsigned int* stretchData = new unsigned int[DIAMETER *DIAMETER];

	double rate;

	VideoCapture capture;
	Mat frame, showImg;
	IplImage * src; IplImage * srcOld;
	IplImage * turned1; IplImage * turned2;

	int redPart = 0;
	int greenPart = 0;

	for (int row = 0; row < RADIUS; row++) {
		rate = sqrt(2 * row * RADIUS - row * row) / RADIUS;
		for (int col = 0; col < DIAMETER; col++) {
			if (col <RADIUS)
				stretchData[row*DIAMETER + col] = -abs(col - RADIUS)*rate + 720;
			else
				stretchData[row*DIAMETER + col] = abs(col - RADIUS)*rate + 720;
		}
	}

	for (int row = RADIUS; row < DIAMETER; row++) {
		rate = sqrt(2 * (2 * RADIUS - row) * RADIUS - (2 * RADIUS - row)*(2 * RADIUS - row)) / RADIUS;
		for (int col = 0; col < DIAMETER; col++) {
			if (col <RADIUS)
				stretchData[row*DIAMETER + col] = -abs(col - RADIUS)*rate + 720;
			else
				stretchData[row*DIAMETER + col] = abs(col - RADIUS)*rate + 720;
		}
	}

	int stepOut = output->widthStep / sizeof(uchar);
	int stepIn = 2880 * 3;
	
	//�򿪴�����ȡͼ��

	globalCapture.open(CAMERAIP);

	if (!globalCapture.isOpened())
	{
		cout << "����ͷ��ʧ�ܣ�" << endl;
		return 0;
	}
	
	// Initial globalFrames
	globalCapture.grab();
	globalCapture.retrieve(globalFrames[oldFrame]);
	globalCapture.grab();
	globalCapture.retrieve(globalFrames[lastestFrame]);

	HANDLE handle = CreateThread(NULL, 0, readFrame, NULL, 0, NULL);


	}*/

	int counter = 0;

	char opt;

	while (1) {
		//���ó�������İ�ť
		//if (_kbhit()) {
		//	opt = _getch();
		//	if (opt == VK_ESCAPE) {
		//		//��⵽esc�����£�ֹͣС��
		//		car->SetSpeed(0, 0);
		//		car->Stop();
		//		capture.release();
		//		FPGA_Close();
		//		return 0;
		//	}
		//}

		if (!remote_mode) {

			//���¾���ֵ�ͼ�⵽����ɫ
			//��ȡһ֡
			//capture >> frame;

			//imshow("mdzz", frame);
			//waitKey(1);


			src = &IplImage(globalFrames[lastestFrame]);

			srcOld = &IplImage(globalFrames[oldFrame]);

			data = (uchar *)src->imageData;

			dataOld = (uchar *)srcOld->imageData;

			for (int row = 0; row < (output->height); row++) {
				for (int col = 0; col < (output->width) / 2; col++) {
					dataOut[row*stepOut + col * 3] = data[(row + 80)*stepIn + (stretchData[row*DIAMETER + col]) * 3];
					dataOut[row*stepOut + col * 3 + 1] = data[(row + 80)*stepIn + (stretchData[row*DIAMETER + col]) * 3 + 1];
					dataOut[row*stepOut + col * 3 + 2] = data[(row + 80)*stepIn + (stretchData[row*DIAMETER + col]) * 3 + 2];
					dataOutOld[row*stepOut + col * 3] = dataOld[(row + 80)*stepIn + (stretchData[row*DIAMETER + col]) * 3];
					dataOutOld[row*stepOut + col * 3 + 1] = dataOld[(row + 80)*stepIn + (stretchData[row*DIAMETER + col]) * 3 + 1];
					dataOutOld[row*stepOut + col * 3 + 2] = dataOld[(row + 80)*stepIn + (stretchData[row*DIAMETER + col]) * 3 + 2];
				}
			}
			for (int row = 0; row < (output->height); row++) {
				for (int col = (output->width) / 2; col < (output->width); col++) {
					dataOut[row*stepOut + col * 3] = data[(row + 80)*stepIn + (stretchData[row*DIAMETER + col - DIAMETER] + 1440) * 3];
					dataOut[row*stepOut + col * 3 + 1] = data[(row + 80)*stepIn + (stretchData[row*DIAMETER + col - DIAMETER] + 1440) * 3 + 1];
					dataOut[row*stepOut + col * 3 + 2] = data[(row + 80)*stepIn + (stretchData[row*DIAMETER + col - DIAMETER] + 1440) * 3 + 2];
					dataOutOld[row*stepOut + col * 3] = dataOld[(row + 80)*stepIn + (stretchData[row*DIAMETER + col - DIAMETER] + 1440) * 3];
					dataOutOld[row*stepOut + col * 3 + 1] = dataOld[(row + 80)*stepIn + (stretchData[row*DIAMETER + col - DIAMETER] + 1440) * 3 + 1];
					dataOutOld[row*stepOut + col * 3 + 2] = dataOld[(row + 80)*stepIn + (stretchData[row*DIAMETER + col - DIAMETER] + 1440) * 3 + 2];
				}
			}

			//cvCvtColor(output, hsv, CV_BGR2HSV);

			//redPart = opencvFire(output);
			//redPart = opencvRed(hsv);
			// FPGA����
			// FPGAдͼƬ
			if (!FPGA_WriteSourceImg((char *)output->imageData, output->imageSize)) {
				cout << "Write Img Fail" << endl;
			}
			// FPGA����
			if (!FPGA_Working) {
				controllFlag[0] = 1;
				if (!FPGA_DmaWrite(PCIE_MEM_ADDR, controllFlag, CONTROLLER_SIZE)) {
					cout << "Write Control flag Fail" << endl;
				}
				FPGA_Working = true;
			}

			// opencv�Ĵ���
			Mat Mnew = cvarrToMat(src);
			Mat Mold = cvarrToMat(srcOld);

			Mat result = MoveDetect(Mnew, Mold);
			greenPart = GreenDetect(result);

			// FPGA��������
			while (FPGA_Working) {
				if (!FPGA_DmaRead(PCIE_MEM_ADDR, controllFlag, CONTROLLER_SIZE)) {
					cout << "Read Control flag Fail" << endl;
				}
				if (FPGA_Working && controllFlag[0] == 0) {
					// FPGA������
					if (!FPGA_DmaRead(OUT_DATA_ADDR, (char *)outData, CONTROLLER_SIZE)) {
						cout << "Read Out data Fail" << endl;
					}
					//FPGA_ReadDestImg(output->imageData, output->imageSize);
					//String outname = "image/ziyi1.png";
					//cvSaveImage(outname.c_str(), output);
					FPGA_Working = false;
				}
			}
			redPart = outData->RedLongitude;
			// FPGA�������

			// ����ɫ���ڸ��ٶ���������
			if (greenPart != -1) redPart = greenPart;
			cout << redPart << endl;

			if (redPart != -1) {
				if (redPart < 1920)
					rotation = (redPart - 640) * 360 / 2560;
				else
					rotation = (redPart - 640 - 2560) * 360 / 2560;
				//cout << "longitude: " << rotation << endl;
			}
			else {
				rotation = 0;
			}


			//���������˲�
			
			//�ж��Ƿ��иı�
			if (redPart != -1) {
				//detected
				detected = true;
				longitude = rotation;
				//cout << longitude << endl;
			}
			else {
				detected = false;
				//cout << "û�м�鵽" << endl;
			}

			//danger = true;

			//����о��ȸı䣬С����Ҫ������Ӧ���˶�
			if (detected) {

				//green
				if (greenPart != -1) {
					if (abs(longitude) > 140) {
						car->SetSpeed(fs, fs);
						state = FORWARD;
					}
					else if (abs(longitude) < 40) {
						car->SetSpeed(-fs, -fs);
						state = BACK;
					}
					else if (longitude > 90) {
						car->SetSpeed(-ts, ts);
						state = ROTATE;
					}
					else if (longitude > 0) {
						car->SetSpeed(ts, -ts);
						state = ROTATE;
					}
					else if (longitude > -90) {
						car->SetSpeed(-ts, ts);
						state = ROTATE;
					}
					else {
						car->SetSpeed(ts, -ts);
						state = ROTATE;
					}
				}
				//red
				else {
					if (abs(longitude) > 160) {
						car->SetSpeed(fs, fs);
						state = FORWARD;
					}
					else if (abs(longitude) < 20) {
						car->SetSpeed(-fs, -fs);
						state = BACK;
					}
					else if (longitude > 90) {
						car->SetSpeed(-ts, ts);
						state = ROTATE;
					}
					else if (longitude > 0) {
						car->SetSpeed(ts, -ts);
						state = ROTATE;
					}
					else if (longitude > -90) {
						car->SetSpeed(-ts, ts);
						state = ROTATE;
					}
					else {
						car->SetSpeed(ts, -ts);
						state = ROTATE;
					}
				}

				//run the car
				car->Start();

				////���С��Ŀǰ��״̬��ֹͣ,��ôС����Ҫ������ת��Ȼ���ƶ�
				//if (state == STAY) {
				//	//�޸�״̬Ϊ��ת
				//	state = ROTATE;
				//	//���½����ʱ��
				//}

				////���С��Ŀǰ��״̬Ϊǰ������ô���2s����С���ƶ�
				//else if (state == FORWARD) {

				//	car->SetSpeed(fs, fs);
				//	car->Start();

				//	//Sleep(actime);
				//	//�������֮��Ĭ��С���Ѿ����ﰲȫλ�ã����û�У���һ��ѭ������С�������˶�
				//	//car->SetSpeed(0, 0);
				//	//car->Stop();
				//	//detected = false;
				//	//longitude = 0;
				//	//state = STAY;
				//}
				//else if (state == BACK) {

				//	car->SetSpeed(-fs, -fs);
				//	car->Start();

				//	//Sleep(actime);
				//	//�������֮��Ĭ��С���Ѿ�����Σ��λ�ã����û�У���һ��ѭ������С�������˶�
				//	//car->SetSpeed(0, 0);
				//	//car->Stop();
				//	//detected = false;
				//	//longitude = 0;
				//	//state = STAY;
				//}
				//else {
				//	//state == ROTATE
				//	//С����ʼ��ת
				//	//�ж�С��Ӧ����ʲô�˶�״̬
				//	if (abs(longitude) < Longitude_Error) {
				//		//���ֵ�ǰ״̬���˻���ǰ��
				//		if (danger) {
				//			state = BACK;
				//		}
				//		else {
				//			state = FORWARD;
				//		}
				//		//longitude = 0;
				//		car->SetSpeed(0, 0);
				//		car->Stop();
				//	}
				//	//���ܵ����⣬��180�㵽-180��֮���ʱ�򣬻�������������䣬�γɺ�����ҵ���ת����
				//	//��Ҫע��������д����
				//	else if (abs(longitude) > 140) {
				//		car->SetSpeed(ts, -ts);
				//		car->Start();
				//		//Sleep(actime);
				//		//car->SetSpeed(0, 0);
				//		//car->Stop();
				//	}
				//	else if (longitude > 0) {
				//		//������С����ת
				//		car->SetSpeed(ts, -ts);
				//		car->Start();
				//		//Sleep(actime);
				//		//car->SetSpeed(0, 0);
				//		//car->Stop();
				//	}
				//	else {
				//		//������С����ת
				//		car->SetSpeed(-ts, ts);
				//		car->Start();
				//		//Sleep(actime);
				//		//car->SetSpeed(0, 0);
				//		//car->Stop();
				//	}
				//}
			}
			else {
				counter += 1;
				if (counter >= 7) {
					car->SetSpeed(0, 0);
					car->Stop();
					state = STAY;
					counter = 0;
				}
			//	//û�м�⵽�������Ѿ������ⷶΧ��С�����ֲ���
			//	car->SetSpeed(0, 0);
			//	car->Stop();
				longitude = 0;
			//	state = STAY;
			}
		}

		////���С��Ŀǰ��״̬
		//cout << "state:  " << debugtest[state] << endl << endl;

		//��Զ�̿�����������Ϣ����
		//�������Դ���������Ϣ��wsadq һ��5��
		int ret = recv(sclient, recvData, 1, 0);
		if (ret == 1) {
			//�ж�ǰ̨��Ϣ
			cmd = recvData[0];
			switch (cmd) {
			case 'w':
				if (remote_mode) {
					car->SetSpeed(fs, fs);
					car->Start();
					detected = false;
					longitude = 0;
					state = REMOTE;
					break;
				}
				break;

			case 's':
				if (remote_mode) {
					car->SetSpeed(-fs, -fs);
					car->Start();
					detected = false;
					longitude = 0;
					state = REMOTE;
					break;
				}
				break;

			case 'a':
				if (remote_mode) {
					car->SetSpeed(-ts, ts);
					car->Start();
					detected = false;
					longitude = 0;
					state = REMOTE;
					break;
				}
				break;

			case 'd':
				if (remote_mode) {
					car->SetSpeed(ts, -ts);
					car->Start();
					detected = false;
					longitude = 0;
					state = REMOTE;
					break;
				}
				break;

			case 'q':
				if (remote_mode) {
					car->SetSpeed(0, 0);
					car->Stop();
					detected = false;
					longitude = 0;
					state = REMOTE;
					break;
				}
				break;

			case 'e':
				if (remote_mode) {
					state = STAY;
					remote_mode = false;

				}
				else {
					state = REMOTE;
					remote_mode = true;
				}
				break;
			}
		}

		//��������
		temps = (remote_mode ? "Զ�̿��� " : "�������� ") + debugtest[state] + " "  + (greenPart == -1 ? "���� " : "�˶����� ") + "�Ƕ�: " + to_string(longitude) + '\n';
		sendData = temps.c_str();
		send(sclient, sendData, strlen(sendData), 0);

		if (remote_mode) {
			//����֡��
			Sleep(100);
		}
	}
}
