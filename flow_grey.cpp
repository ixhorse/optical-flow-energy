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


void motionToGrey(Mat flow, Mat &grey)
{
	if (grey.empty())
		grey.create(flow.rows, flow.cols, CV_8UC1);

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
			uchar *data = grey.data + grey.step[0] * i + grey.step[1] * j;
			Vec2f flow_at_point = flow.at<Vec2f>(i, j);

			float fx = flow_at_point[0];
			float fy = flow_at_point[1];

			if ((fabs(fx) >  UNKNOWN_FLOW_THRESH) || (fabs(fy) >  UNKNOWN_FLOW_THRESH))
			{
				*data = 255;
				continue;
			}

			float f = sqrt(fx * fx + fy * fy) / maxrad;


			*data = 255 * (1 - f);
		}
	}
}

void calcflow(string video_name, string flow_out)
{
	VideoCapture cap(video_name);

	Mat prevgray, gray, flow, cflow, frame, temp_gray;

	Mat motion2grey;
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
			motionToGrey(flow, motion2grey);
			//imshow("g", motion2grey);


			char filename[100];
			string temp;
			snprintf(filename, 100, "%d.jpg", j);
			temp = flow_out + "\\" + filename;
			cout << temp << endl;
			imwrite(temp.c_str(), motion2grey);
			/*cvtColor(motion2grey, temp_gray, CV_BGR2GRAY);
			imshow("g", temp_gray);*/
			//waitKey(10 * 1000);
		}
		/*if (waitKey(10) >= 0)
			break;*/
		std::swap(prevgray, gray);

		t = (double)cvGetTickCount() - t;
		cout << "cost time: " << t / ((double)cvGetTickFrequency()*1000.) << endl;
	}

	cap.release();
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
				if ((lf2 = _findfirst((temp + "\\*.avi").c_str(), &video)) == -1L)
				{
					cout << "no file\n";
				}
				else
				{
					string flow_out, video_path, video_outdir;
					video_path = temp + "\\" + video.name;
					video_outdir = out_dir + file.name;
					if (_access(video_outdir.c_str(), 0) == -1)
						_mkdir(video_outdir.c_str());
					flow_out = out_dir + file.name + "\\" + seq_dir;

					cout << flow_out << endl;
					if (_access(flow_out.c_str(), 0) == -1)
						_mkdir(flow_out.c_str());
					calcflow(video_path, flow_out);
					//return 0;
				}
			}
		} while (!_findnext(lf, &file));

	}
	return 0;
}