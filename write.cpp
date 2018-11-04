#include <iostream>  
#include <string>  
#include "opencv2/highgui/highgui.hpp"  
#include "opencv2/core/core.hpp"

using namespace std;

void Image_To_Video()
{
	cout << "---------------Video_To_Image-----------------" << endl;
	char image_name[50];
	string s_image_name;
	cv::VideoWriter writer;
	int isColor = 1;//��֪���Ǹ�ɶ�õ�  
	int frame_fps = 30;
	int frame_width = 1280;
	int frame_height = 720;
	using namespace cv;
	string video_name = "out.avi";
	writer = VideoWriter(video_name, CV_FOURCC('X', 'V', 'I', 'D'), frame_fps, Size(frame_width, frame_height), isColor);
	cout << "frame_width is " << frame_width << endl;
	cout << "frame_height is " << frame_height << endl;
	cout << "frame_fps is " << frame_fps << endl;
	cv::namedWindow("image to video", CV_WINDOW_AUTOSIZE);
	int num = 320;//�����ͼƬ������  
	int i = 0;
	Mat img;
	while (i <= num)
	{
		sprintf_s(image_name, "%s%d%s", "E:\\mcc\\hand\\seg02\\", ++i, ".jpg");
		s_image_name = image_name;
		img = imread(s_image_name);//����ͼƬ  
		if (!img.data)//�ж�ͼƬ�����Ƿ�ɹ�  
		{
			cout << "Could not load image file...\n" << endl;
		}
		imshow("image to video", img);
		//д��  
		writer.write(img);
		if (cv::waitKey(30) == 27 || i == 320)
		{
			cout << "����ESC��" << endl;
			break;
		}
	}
}

int main()
{
	Image_To_Video();
	return 0;
}