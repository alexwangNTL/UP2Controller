// IntelProgram.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#using "Car.dll"
#include <iostream>
#include <conio.h>	//_getch()�������ڿ⣬�������Ŀ��Ӧ���ò���
#include <time.h>
#include <windows.h>

#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"

#include <math.h>
#include <fstream> 

#pragma warning(disable:4996)

using namespace Car;
using namespace std;
using namespace cv;

//�������ڲ�����
//��0�Ⱦ���Ϊ��׼����Ϊ��������Ϊ����
//�趫��Ϊ��������Ϊ�������ɣ�������


//���ȵ����
#define Longitude_Error 20

//ǰ������ת���ٶ�
#define fs 1.0	//forward speed
#define ts 1.0	//turn speed

//С���������˻�ǰ����ʱ�䣨����Ϊ��λ)
#define actime 1000

//С��״̬��
#define STAY 0		//���ֲ���
#define FORWARD 1	//��ǰ�ƶ�
#define BACK 2		//����ƶ�
#define ROTATE 3	//ԭ����ת

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
					if ((!(((s_hsv.val[0]>0) && (s_hsv.val[0]<8)) || (s_hsv.val[0]>120) && (s_hsv.val[0]<180))) || s_hsv.val[1] < 80)
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
				return j;
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

	for (int i = 0; i < DIAMETER; i += 32)
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

//��Ŀ������
int main()
{
	//init������Ŀ��ʹ��mode3�����С���ƶ�
	PcanControl ^car = gcnew PcanControl();
	car->SetMode(car->Mode3);


	//��û�м�⵽
	bool detected = false;
	//��⵽���Ǻ�(1)������(0)
	bool danger = true;

	//С��Ŀǰ��״̬
	int state = 0;

	//״̬����
	vector<string> debugtest;
	debugtest.push_back("����");
	debugtest.push_back("��ǰ");
	debugtest.push_back("���");
	debugtest.push_back("��ת");


	//the detected thing's longitude
	double longitude = 0;

	//ͼ�����ֵ�init
	double rotation = 0;
	IplImage* output = cvCreateImage(cvSize(2560, 1280), 8, 3);

	IplImage* hsv = cvCreateImage(cvSize(2560, 1280), 8, 3);

	uchar* data;

	uchar* dataOut = (uchar *)output->imageData;

	unsigned int* stretchData = new unsigned int[DIAMETER *DIAMETER];

	double rate;

	VideoCapture capture;
	Mat frame, showImg;
	IplImage * src;

	int redPart = 0;

	for (int row = 0; row < RADIUS; row++) {
		rate = sqrt(2 * row * RADIUS - row*row) / RADIUS;
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
	capture.open("rtmp://192.168.0.100/live/insta360");

	if (!capture.isOpened())
	{
		cout << "����ͷ��ʧ�ܣ�" << endl;
		return 0;
	}

	char opt;
	while (1) {
		//���ó�������İ�ť
		if (_kbhit()) {
			opt = _getch();
			if (opt == VK_ESCAPE) {
				//��⵽esc�����£�ֹͣС��
				car->SetSpeed(0, 0);
				car->Stop();
				capture.release();
				return 0;
			}
		}

		//���¾���ֵ�ͼ�⵽����ɫ
		//��ȡһ֡
		capture >> frame;
		src = &IplImage(frame);

		data = (uchar *)src->imageData;

		for (int row = 0; row < (output->height); row++) {
			for (int col = 0; col < (output->width) / 2; col++) {
				dataOut[row*stepOut + col * 3] = data[(row + 80)*stepIn + (stretchData[row*DIAMETER + col]) * 3];
				dataOut[row*stepOut + col * 3 + 1] = data[(row + 80)*stepIn + (stretchData[row*DIAMETER + col]) * 3 + 1];
				dataOut[row*stepOut + col * 3 + 2] = data[(row + 80)*stepIn + (stretchData[row*DIAMETER + col]) * 3 + 2];
			}
		}
		for (int row = 0; row < (output->height); row++) {
			for (int col = (output->width) / 2; col < (output->width); col++) {
				dataOut[row*stepOut + col * 3] = data[(row + 80)*stepIn + (stretchData[row*DIAMETER + col - DIAMETER] + 1440) * 3];
				dataOut[row*stepOut + col * 3 + 1] = data[(row + 80)*stepIn + (stretchData[row*DIAMETER + col - DIAMETER] + 1440) * 3 + 1];
				dataOut[row*stepOut + col * 3 + 2] = data[(row + 80)*stepIn + (stretchData[row*DIAMETER + col - DIAMETER] + 1440) * 3 + 2];
			}
		}

		cvCvtColor(output, hsv, CV_BGR2HSV);

		//redPart = opencvFire(output);
		redPart = opencvRed(hsv);

		if (redPart != -1) {
			if (redPart < 1920)
				rotation = (redPart - 640) * 360 / 2560;
			else
				rotation = (redPart - 640 - 2560) * 360 / 2560;
			cout << "longitude: " << rotation << endl;
		}

		//rotation���ǿ���С����Ҫ���Ǹ��Ƕ�

		longitude = rotation;
		danger = true;


		//�ж��Ƿ��иı�
		if (redPart != -1) {
			//detected
			detected = true;
		}

		//����о��ȸı䣬С����Ҫ������Ӧ���˶�
		if (detected) {

			//���С��Ŀǰ��״̬��ֹͣ,��ôС����Ҫ������ת��Ȼ���ƶ�
			if (state == STAY) {
				//�޸�״̬Ϊ��ת
				state = ROTATE;
				//���½����ʱ��
			}

			//���С��Ŀǰ��״̬Ϊǰ������ô���2s����С���ƶ�
			else if (state == FORWARD) {

				car->SetSpeed(fs, fs);
				car->Start();

				Sleep(actime);
				//�������֮��Ĭ��С���Ѿ����ﰲȫλ�ã����û�У���һ��ѭ������С�������˶�
				car->SetSpeed(0, 0);
				car->Stop();
				detected = false;
				longitude = 0;
				state = STAY;
			}
			else if (state == BACK) {

				car->SetSpeed(-fs, -fs);
				car->Start();

				Sleep(actime);
				//�������֮��Ĭ��С���Ѿ�����Σ��λ�ã����û�У���һ��ѭ������С�������˶�
				car->SetSpeed(0, 0);
				car->Stop();
				detected = false;
				longitude = 0;
				state = STAY;
			}
			else {
				//state == ROTATE
				//С����ʼ��ת
				//�ж�С��Ӧ����ʲô�˶�״̬
				if (abs(longitude) < Longitude_Error) {
					//���ֵ�ǰ״̬���˻���ǰ��
					if (danger) {
						state = BACK;
					}
					else {
						state = FORWARD;
					}
					longitude = 0;
					car->SetSpeed(0, 0);
					car->Stop();
				}
				//���ܵ����⣬��180�㵽-180��֮���ʱ�򣬻�������������䣬�γɺ�����ҵ���ת����
				//��Ҫע��������д����
				else if (abs(longitude) > 140) {
					car->SetSpeed(ts, -ts);
					car->Start();
				}
				else if (longitude > 0) {
					//������С����ת
					car->SetSpeed(ts, -ts);
					car->Start();

				}
				else {
					//������С����ת
					car->SetSpeed(-ts, ts);
					car->Start();

				}
			}
		}
		else {
			//û�м�⵽�������Ѿ������ⷶΧ��С�����ֲ���
			car->SetSpeed(0, 0);
			car->Stop();
			longitude = 0;
			state = STAY;
		}

		//���С��Ŀǰ��״̬
		cout << "state:  " << debugtest[state] << endl << endl;

		waitKey(30);
	}
}
