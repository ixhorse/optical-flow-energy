// Farneback dense optical flow calculate and show in Munsell system of colors  
// Author : Zouxy  
// Date   : 2013-3-15  
// HomePage : http://blog.csdn.net/zouxy09  
// Email  : zouxy09@qq.com  

// API calcOpticalFlowFarneback() comes from OpenCV, and this  
// 2D dense optical flow algorithm from the following paper:  
// Gunnar Farneback. "Two-Frame Motion Estimation Based on Polynomial Expansion".  
// And the OpenCV source code locate in ..\opencv2.4.3\modules\video\src\optflowgf.cpp  

#include <iostream>
#include <io.h>
#include <stdio.h>
#include <direct.h>  
#include "opencv2/opencv.hpp"
#include "opencv/highgui.h"

using namespace cv;
using namespace std;

#define UNKNOWN_FLOW_THRESH 1e9  

// Color encoding of flow vectors from:  
// http://members.shaw.ca/quadibloc/other/colint.htm  
// This code is modified from:  
// http://vision.middlebury.edu/flow/data/  
void makecolorwheel(vector<Scalar> &colorwheel)
{
	int RY = 15;
	int YG = 6;
	int GC = 4;
	int CB = 11;
	int BM = 13;
	int MR = 6;

	int i;

	for (i = 0; i < RY; i++) colorwheel.push_back(Scalar(255, 255 * i / RY, 0));
	for (i = 0; i < YG; i++) colorwheel.push_back(Scalar(255 - 255 * i / YG, 255, 0));
	for (i = 0; i < GC; i++) colorwheel.push_back(Scalar(0, 255, 255 * i / GC));
	for (i = 0; i < CB; i++) colorwheel.push_back(Scalar(0, 255 - 255 * i / CB, 255));
	for (i = 0; i < BM; i++) colorwheel.push_back(Scalar(255 * i / BM, 0, 255));
	for (i = 0; i < MR; i++) colorwheel.push_back(Scalar(255, 0, 255 - 255 * i / MR));
}

void motionToColor(Mat flow, Mat &color)
{
	if (color.empty())
		color.create(flow.rows, flow.cols, CV_8UC3);

	static vector<Scalar> colorwheel; //Scalar r,g,b  
	if (colorwheel.empty())
		makecolorwheel(colorwheel);

	// determine motion range:  
	float maxrad = -1;

	// Find max flow to normalize fx and fy  
	for (int i = 0; i < flow.rows; ++i)
	{
		for (int j = 0; j < flow.cols; ++j)
		{
			Vec2f flow_at_point = flow.at<Vec2f>(i, j);
			float fx = flow_at_point[0];
			float fy = flow_at_point[1];
			if ((fabs(fx) >  UNKNOWN_FLOW_THRESH) || (fabs(fy) >  UNKNOWN_FLOW_THRESH))
				continue;
			float rad = sqrt(fx * fx + fy * fy);
			maxrad = maxrad > rad ? maxrad : rad;
		}
	}

	for (int i = 0; i < flow.rows; ++i)
	{
		for (int j = 0; j < flow.cols; ++j)
		{
			uchar *data = color.data + color.step[0] * i + color.step[1] * j;
			Vec2f flow_at_point = flow.at<Vec2f>(i, j);

			float fx = flow_at_point[0] / maxrad;
			float fy = flow_at_point[1] / maxrad;
			if ((fabs(fx) >  UNKNOWN_FLOW_THRESH) || (fabs(fy) >  UNKNOWN_FLOW_THRESH))
			{
				data[0] = data[1] = data[2] = 0;
				continue;
			}
			float rad = sqrt(fx * fx + fy * fy);

			float angle = atan2(-fy, -fx) / CV_PI;
			float fk = (angle + 1.0) / 2.0 * (colorwheel.size() - 1);
			int k0 = (int)fk;
			int k1 = (k0 + 1) % colorwheel.size();
			float f = fk - k0;
			//f = 0; // uncomment to see original color wheel  

			for (int b = 0; b < 3; b++)
			{
				float col0 = colorwheel[k0][b] / 255.0;
				float col1 = colorwheel[k1][b] / 255.0;
				float col = (1 - f) * col0 + f * col1;
				if (rad <= 1)
					col = 1 - rad * (1 - col); // increase saturation with radius  
				else
					col *= .75; // out of range  
				data[2 - b] = (int)(255.0 * col);
			}
		}
	}
}

void calcflow(string video_name, string flow_out)
{
	VideoCapture cap(video_name);

	Mat prevgray, gray, flow, cflow, frame, temp_gray;

	Mat motion2color;
	int frame_count;
	frame_count = cap.get(CV_CAP_PROP_FRAME_COUNT);

	int i, j;
	for (i = 0, j = 0; j < frame_count; i++, j++)
	{
		double t = (double)cvGetTickCount();

		cap >> frame;
		cvtColor(frame, gray, CV_BGR2GRAY);
		//imshow("original", frame);

		if (prevgray.data)
		{
			calcOpticalFlowFarneback(prevgray, gray, flow, 0.5, 3, 15, 3, 5, 1.2, 0);
			motionToColor(flow, motion2color);


			char filename[100];
			string temp;
			snprintf(filename, 100, "%d.jpg", j);
			temp = flow_out + "\\" + filename;
			cout << temp << endl;
			imwrite(temp.c_str(), motion2color);
			cvtColor(motion2color, temp_gray, CV_BGR2GRAY);
			imshow("g", temp_gray);
			waitKey(10 * 1000);
		}
		if (waitKey(10) >= 0)
			break;
		std::swap(prevgray, gray);

		t = (double)cvGetTickCount() - t;
		cout << "cost time: " << t / ((double)cvGetTickFrequency()*1000.) << endl;
	}
}

int main()
{
	string src_dir = "E:\\mcc\\ucfsports\\ucf action";
	string out_dir = "E:\\mcc\\ucfsports\\flow_color\\";
	string p;
	struct _finddata_t file, video;
	intptr_t lf, lf2;
	int seq;

	if ((lf = _findfirst(p.assign(src_dir).append("\\*").c_str(), &file)) == -1L)
	{
		cout << "no file\n";
	}
	else
	{
		do
		{
			p = src_dir + "\\" + file.name + "\\";
			for (seq = 1; ; seq++)
			{
				string temp;
				char seq_dir[10];
				snprintf(seq_dir, 10, "%03d", seq);
				temp.assign(p).append(seq_dir);
				if (_access(temp.c_str(), 0) == -1)
					break;
				if ((lf2 = _findfirst((temp+"\\*.avi").c_str(), &video)) == -1L)
				{
					cout << "no file\n";
				}
				else
				{
					string flow_out, video_path;
					video_path = temp + "\\" + video.name;
					flow_out = out_dir + file.name + "\\" + seq_dir;
					
					cout << flow_out << endl;
					if (_access(flow_out.c_str(), 0) == -1)
						_mkdir(flow_out.c_str());
					calcflow(video_path, flow_out);
					return 0;
				}
			}
		} while (!_findnext(lf, &file));

	}
	return 0;
}