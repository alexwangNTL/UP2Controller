#include "MoveDetector.h"

Mat MoveDetect(Mat _oldFrame, Mat _newFrame)
{
	//Mat result = lastestFrame.clone();
	//1.��background��frameתΪ�Ҷ�ͼ
	Mat gray1, gray2;
	cvtColor(_oldFrame, gray1, CV_BGR2GRAY);
	cvtColor(_newFrame, gray2, CV_BGR2GRAY);
	//2.��background��frame����
	Mat diff;
	absdiff(gray1, gray2, diff);
	//imshow("diff", diff);
	//3.�Բ�ֵͼdiff_thresh������ֵ������
	Mat diff_thresh;
	threshold(diff, diff_thresh, 50, 255, CV_THRESH_BINARY);
	//imshow("diff_thresh", diff_thresh);
	Mat result;
	cvtColor(diff_thresh, diff_thresh, CV_GRAY2BGR);
	bitwise_and(diff_thresh, _newFrame, result);
	return result;
}

bool PointCheck(Vec3b v3) {
	if (v3[1] > v3[0] + v3[2] && v3[1] >= 100) {
		return true;
	}
	else {
		return false;
	}
}

int GreenDetect(Mat src) {
	int row = src.rows;
	int col = src.cols;

	bool ret;

	//��������ͼ�񣬶�ȡ��ɫΪ��ɫ�Ĳ��֣�G > R + B��9x9
	for (int i = 4; i < row - 5; ++i) {
		for (int j = 4; j < col - 5; ++j) {
			ret = PointCheck(src.at<Vec3b>(i - 4, j - 4))
				&& PointCheck(src.at<Vec3b>(i - 4, j - 3))
				&& PointCheck(src.at<Vec3b>(i - 4, j - 2))
				&& PointCheck(src.at<Vec3b>(i - 4, j - 1))
				&& PointCheck(src.at<Vec3b>(i - 4, j))
				&& PointCheck(src.at<Vec3b>(i - 4, j + 1))
				&& PointCheck(src.at<Vec3b>(i - 4, j + 2))
				&& PointCheck(src.at<Vec3b>(i - 4, j + 3))
				&& PointCheck(src.at<Vec3b>(i - 4, j + 4))

				&& PointCheck(src.at<Vec3b>(i - 3, j - 4))
				&& PointCheck(src.at<Vec3b>(i - 3, j - 3))
				&& PointCheck(src.at<Vec3b>(i - 3, j - 2))
				&& PointCheck(src.at<Vec3b>(i - 3, j - 1))
				&& PointCheck(src.at<Vec3b>(i - 3, j))
				&& PointCheck(src.at<Vec3b>(i - 3, j + 1))
				&& PointCheck(src.at<Vec3b>(i - 3, j + 2))
				&& PointCheck(src.at<Vec3b>(i - 3, j + 3))
				&& PointCheck(src.at<Vec3b>(i - 3, j + 4))

				&& PointCheck(src.at<Vec3b>(i - 2, j - 4))
				&& PointCheck(src.at<Vec3b>(i - 2, j - 3))
				&& PointCheck(src.at<Vec3b>(i - 2, j - 2))
				&& PointCheck(src.at<Vec3b>(i - 2, j - 1))
				&& PointCheck(src.at<Vec3b>(i - 2, j))
				&& PointCheck(src.at<Vec3b>(i - 2, j + 1))
				&& PointCheck(src.at<Vec3b>(i - 2, j + 2))
				&& PointCheck(src.at<Vec3b>(i - 2, j + 3))
				&& PointCheck(src.at<Vec3b>(i - 2, j + 4))

				&& PointCheck(src.at<Vec3b>(i - 1, j - 4))
				&& PointCheck(src.at<Vec3b>(i - 1, j - 3))
				&& PointCheck(src.at<Vec3b>(i - 1, j - 2))
				&& PointCheck(src.at<Vec3b>(i - 1, j - 1))
				&& PointCheck(src.at<Vec3b>(i - 1, j))
				&& PointCheck(src.at<Vec3b>(i - 1, j + 1))
				&& PointCheck(src.at<Vec3b>(i - 1, j + 2))
				&& PointCheck(src.at<Vec3b>(i - 1, j + 3))
				&& PointCheck(src.at<Vec3b>(i - 1, j + 4))

				&& PointCheck(src.at<Vec3b>(i, j - 4))
				&& PointCheck(src.at<Vec3b>(i, j - 3))
				&& PointCheck(src.at<Vec3b>(i, j - 2))
				&& PointCheck(src.at<Vec3b>(i, j - 1))
				&& PointCheck(src.at<Vec3b>(i, j))
				&& PointCheck(src.at<Vec3b>(i, j + 1))
				&& PointCheck(src.at<Vec3b>(i, j + 2))
				&& PointCheck(src.at<Vec3b>(i, j + 3))
				&& PointCheck(src.at<Vec3b>(i, j + 4))

				&& PointCheck(src.at<Vec3b>(i + 1, j - 4))
				&& PointCheck(src.at<Vec3b>(i + 1, j - 3))
				&& PointCheck(src.at<Vec3b>(i + 1, j - 2))
				&& PointCheck(src.at<Vec3b>(i + 1, j - 1))
				&& PointCheck(src.at<Vec3b>(i + 1, j))
				&& PointCheck(src.at<Vec3b>(i + 1, j + 1))
				&& PointCheck(src.at<Vec3b>(i + 1, j + 2))
				&& PointCheck(src.at<Vec3b>(i + 1, j + 3))
				&& PointCheck(src.at<Vec3b>(i + 1, j + 4))

				&& PointCheck(src.at<Vec3b>(i + 2, j - 4))
				&& PointCheck(src.at<Vec3b>(i + 2, j - 3))
				&& PointCheck(src.at<Vec3b>(i + 2, j - 2))
				&& PointCheck(src.at<Vec3b>(i + 2, j - 1))
				&& PointCheck(src.at<Vec3b>(i + 2, j))
				&& PointCheck(src.at<Vec3b>(i + 2, j + 1))
				&& PointCheck(src.at<Vec3b>(i + 2, j + 2))
				&& PointCheck(src.at<Vec3b>(i + 2, j + 3))
				&& PointCheck(src.at<Vec3b>(i + 2, j + 4))

				&& PointCheck(src.at<Vec3b>(i + 3, j - 4))
				&& PointCheck(src.at<Vec3b>(i + 3, j - 3))
				&& PointCheck(src.at<Vec3b>(i + 3, j - 2))
				&& PointCheck(src.at<Vec3b>(i + 3, j - 1))
				&& PointCheck(src.at<Vec3b>(i + 3, j))
				&& PointCheck(src.at<Vec3b>(i + 3, j + 1))
				&& PointCheck(src.at<Vec3b>(i + 3, j + 2))
				&& PointCheck(src.at<Vec3b>(i + 3, j + 3))
				&& PointCheck(src.at<Vec3b>(i + 3, j + 4))

				&& PointCheck(src.at<Vec3b>(i + 4, j - 4))
				&& PointCheck(src.at<Vec3b>(i + 4, j - 3))
				&& PointCheck(src.at<Vec3b>(i + 4, j - 2))
				&& PointCheck(src.at<Vec3b>(i + 4, j - 1))
				&& PointCheck(src.at<Vec3b>(i + 4, j))
				&& PointCheck(src.at<Vec3b>(i + 4, j + 1))
				&& PointCheck(src.at<Vec3b>(i + 4, j + 2))
				&& PointCheck(src.at<Vec3b>(i + 4, j + 3))
				&& PointCheck(src.at<Vec3b>(i + 4, j + 4));

			if (ret) {
				return j;
			}
		}
	}
	return -1;
}

Mat stdMoveDetect(Mat oldFrame, Mat lastestFrame)
{
	Mat result = lastestFrame.clone();
	//1.��background��frameתΪ�Ҷ�ͼ
	Mat gray1, gray2;
	cvtColor(oldFrame, gray1, CV_BGR2GRAY);
	cvtColor(lastestFrame, gray2, CV_BGR2GRAY);
	//2.��background��frame����
	Mat diff;
	absdiff(gray1, gray2, diff);
	//imshow("diff", diff);
	//3.�Բ�ֵͼdiff_thresh������ֵ������
	Mat diff_thresh;
	threshold(diff, diff_thresh, 50, 255, CV_THRESH_BINARY);
	//imshow("diff_thresh", diff_thresh);
	//4.��ʴ
	Mat kernel_erode = getStructuringElement(MORPH_RECT, Size(3, 3));
	Mat kernel_dilate = getStructuringElement(MORPH_RECT, Size(18, 18));
	erode(diff_thresh, diff_thresh, kernel_erode);
	//imshow("erode", diff_thresh);
	//5.����
	dilate(diff_thresh, diff_thresh, kernel_dilate);
	//imshow("dilate", diff_thresh);
	//6.������������������
	vector<vector<Point>> contours;
	findContours(diff_thresh, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);
	drawContours(result, contours, -1, Scalar(0, 0, 255), 2);//��result�ϻ�������
															 //7.��������Ӿ���
	vector<Rect> boundRect(contours.size());
	for (int i = 0; i < contours.size(); i++)
	{
		boundRect[i] = boundingRect(contours[i]);
		rectangle(result, boundRect[i], Scalar(0, 255, 0), 2);//��result�ϻ�������Ӿ���
	}
	return result;//����result
}
