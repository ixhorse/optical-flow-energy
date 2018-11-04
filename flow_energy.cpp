////////////////////////////////////////////////////////////////////////

#ifdef _CH_
#pragma package <opencv>
#endif

#ifndef _EiC
#include "opencv/cv.h"
#include "opencv/highgui.h"
#include <stdio.h>
#include <ctype.h>
#include "algorithm"
#include <stdlib.h>
#include <math.h>  
#include "iostream"
#include <stdlib.h>
#include <opencv2/opencv.hpp>
#include "opencv2/legacy/legacy.hpp"
#include <fstream>
#include "list"
#endif



using namespace std;
deque<IplImage*> ImageQueue;           // 保存计算背景的图片

CvScalar hsv2rgb( float hue )
{
	int rgb[3], p, sector;
	static const int sector_data[][3]=
	{{0,2,1}, {1,2,0}, {1,0,2}, {2,0,1}, {2,1,0}, {0,1,2}};
	hue *= 0.033333333333333333333333333333333f;
	sector = cvFloor(hue);
	p = cvRound(255*(hue - sector));
	p ^= sector & 1 ? 255 : 0;

	rgb[sector_data[sector][0]] = 255;
	rgb[sector_data[sector][1]] = 0;
	rgb[sector_data[sector][2]] = p;

	return cvScalar(rgb[2], rgb[1], rgb[0],0);
}

typedef struct Vel
{
	int flag=0;
	int dx;
	int dy;
} Vel;

typedef struct Person
{
	list <CvRect> History_list;
	int tag;                   
	Vel vel;

}Person;

int main(int argc,char* argv[])
{
	// 读入视频文件

	CvCapture* capture = cvCreateFileCapture("frame3.avi");   //名字需要修改，159行同样需要修改

	
	int Num_frame=150;            // 对于不同的视频本身应该可以设定同样的数字，但由于测试视频的影响，对于不同视频Num_frame需要进行不同设置，参数如下                
	double B[60];                // Browse3.mpg=25;               Fight_OneManDown.mpg=90;         LeftBag.mpg=70;
	double G[60];                // Rest_WiggleOnFloor.mpg=45;    Meet_WalkSplit.mpg=60;             Walk1.mpg=60;
	double R[60];
	int pos=0,i,j,k;	
	int flag=1;
	double Ep[200];			//个人能量
	double Ein[200];		//交互能量
	double E[200];
	char Image_name[100];
	int n=1;
	IplImage *frame=NULL;                    
	IplImage *Background=NULL;   

	CvScalar pix_temp;
	CvScalar temp;
	CvSize czSize;

	frame = cvQueryFrame(capture);//读取图像

	czSize.width = frame->width;
	czSize.height = frame->height;

	Background=cvCreateImage(czSize,frame->depth,frame->nChannels); //建立背景

	Background = cvCreateImage(czSize,IPL_DEPTH_8U,3);

	for (i = 0; i<Background->height; i++)
	{
		for (j = 0; j<Background->width; j++)
		{

			temp.val[0] = 0;
			temp.val[1] = 0;
			temp.val[2] = 0;


			cvSet2D(Background, i, j, temp);

		}
	}

	IplImage *Imask =NULL;
	IplImage *Image_temp =NULL;
	IplImage *Image =NULL;
	IplImage *pbackground =NULL;
	IplImage *hsv = 0;
	hsv = cvCreateImage( cvGetSize(frame), 8, 3 );
	Imask =cvCreateImage(cvGetSize(frame),IPL_DEPTH_8U,1);
	Image_temp =cvCreateImage(cvGetSize(frame),IPL_DEPTH_8U,1);	
	Image =cvCreateImage(cvGetSize(frame),IPL_DEPTH_8U,3);	
	pbackground =cvCreateImage(cvGetSize(frame),IPL_DEPTH_8U,1);
	list <Person> Person_List;           
	list <Person> ::iterator iter;
	cvCvtColor(Background,pbackground,CV_BGR2GRAY);	
	CvRect track_window;
	CvConnectedComp track_comp;
	CvBox2D track_box;


	frame=NULL;
	IplImage *src_img1=NULL, *src_img2=NULL, *dst_img1=NULL, *dst_img2=NULL;
	CvMat *velx, *vely;
	CvTermCriteria criteria;
	capture = cvCreateFileCapture("frame3.avi");
	cvQueryFrame( capture );
	CvSize frame_size;
	frame_size.height =    (int) cvGetCaptureProperty( capture, CV_CAP_PROP_FRAME_HEIGHT );
	frame_size.width =    (int) cvGetCaptureProperty( capture, CV_CAP_PROP_FRAME_WIDTH );

	long number_of_frames;
	cvSetCaptureProperty( capture, CV_CAP_PROP_POS_AVI_RATIO, 1. );
	number_of_frames = (int) cvGetCaptureProperty( capture, CV_CAP_PROP_POS_FRAMES );

	cvSetCaptureProperty( capture, CV_CAP_PROP_POS_FRAMES, 0. );
	int frame_tag=0;
	long current_frame = 0;

	
	int  dx, dy, rows, cols;
	cols = frame_size.width;
	rows = frame_size.height;
	CvMat *vel,*theta,*velmax,*thetamax,*delt_thetamax,*delt_theta,*tmptheta,*m,*tmpp;
	vel= cvCreateMat(rows, cols, CV_32FC1);
	cvSetZero(vel);
	theta=cvCreateMat(rows, cols, CV_32FC1);
	cvSetZero(theta);
	velmax=cvCreateMat(rows, cols, CV_32FC1);
	cvSetZero(velmax);
	thetamax=cvCreateMat(rows, cols, CV_32FC1);
	cvSetZero(thetamax);
	delt_thetamax=cvCreateMat(rows, cols, CV_32FC1);
	cvSetZero(delt_thetamax);		
	delt_theta=cvCreateMat(rows, cols, CV_32FC1);
	cvSetZero(delt_theta);	
	tmptheta=cvCreateMat(rows, cols, CV_32FC1);
	cvSetZero(tmptheta);
	m=cvCreateMat(rows, cols, CV_32FC1);
	cvSetZero(m);
	tmpp=cvCreateMat(rows, cols, CV_32FC1);
	cvSetZero(tmpp);

	
	while(true)
	{	
		
		cvSetCaptureProperty( capture, CV_CAP_PROP_POS_FRAMES, current_frame );
		frame = cvQueryFrame( capture );

		cvCopy( frame, Image, 0 );
		cvCvtColor(frame,Image_temp,CV_BGR2GRAY);
		cvAbsDiff(Image_temp,pbackground,Imask);                      

		int mbest=40;

		cvSmooth(Imask, Imask, CV_MEDIAN);                   
		cvThreshold(Imask,Imask,mbest, 255, CV_THRESH_BINARY);  

		cvDilate(Imask,Imask,NULL,3);
		cvErode(Imask,Imask,NULL,3);
		temp.val[0]=0;
		temp.val[1]=0;
		temp.val[2]=0;

		for(i=160;i<Background->height;i++)
		{
			for(j=0;j<170;j++)
			{
				cvSet2D(Imask,i,j,temp); 
			}
		}

		CvMemStorage *stor;
		CvSeq *cont, *result, *squares;
		const int CONTOUR_MAX_AERA = 500;
		stor = cvCreateMemStorage(0);
		cont = cvCreateSeq(CV_SEQ_ELTYPE_POINT, sizeof(CvSeq), sizeof(CvPoint) , stor);

		//cvShowImage("Imask",Imask); 

	
		cvFindContours( Imask, stor, &cont, sizeof(CvContour), 
			CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, cvPoint(0,0));


		char c = cvWaitKey(10);
		CvRect r,r_temp;
		int num1=0;
		int num2=0;

		//*******face*******
		/*if (current_frame != 0)
		{
			Person face_temp;
			r.x = 0;
			r.y = 0;
			r.width = 300;
			r.height = 240;
			face_temp.History_list.push_back(r);
			face_temp.tag = 1;
			Person_List.push_back(face_temp);
		}*/
		//

		for(;cont;cont = cont->h_next)
		{	
			r= ((CvContour*)cont)->rect;	
			num1=num1+1;
			if(r.height * r.width > CONTOUR_MAX_AERA ) 
			{
				cvRectangle( Imask, cvPoint(r.x,r.y), 
					cvPoint(r.x + r.width, r.y + r.height),
					cvScalar(255,0,0));
				num2=num2+1;
				
				bool bRet = Person_List.empty();              
				double MaxDis=5000;
				int value;
				double a[20]={25000,25000,25000,25000,25000,25000,25000,25000,25000,25000,25000,25000,25000,25000,25000,25000,25000,25000,25000,25000};

				CvMat dis=cvMat(1,20,CV_64FC1,a); 

				/*for(i=0;i<20;i++)
				{
					printf("%f\n",cvmGet(&dis,0,i));
				}*/

				double min=0;
				CvPoint MinLocation;
				int temp_num=0;
				int temp_num_sb=0;
				if (bRet)                             
				{
					Person person_temp1;
					person_temp1.History_list.push_back(r);
					person_temp1.tag=1;                        

					Person_List.push_back(person_temp1);
				}	
				// 采用camshift进行跟踪
				else      
				{			

					for(iter=Person_List.begin();iter!=Person_List.end();iter++)
					{
						if((*iter).tag!=1)
						{
							r_temp=(*iter).History_list.back();
							value=(r_temp.x-r.x)*(r_temp.x-r.x)+(r_temp.y-r.y)*(r_temp.y-r.y);		
							cvmSet(&dis,0,temp_num,value);							
							temp_num=temp_num+1;
						}
					}
					/*for(i=0;i<20;i++)
					{
						printf("%f\n",cvmGet(&dis,0,i));
					}*/

					cvMinMaxLoc(&dis,&min,NULL,&MinLocation,NULL);
					if(min<MaxDis&&min!=5*MaxDis)
					{

						for(iter=Person_List.begin();iter!=Person_List.end();iter++)
						{
							if((*iter).tag!=1)
							{							
								if(temp_num_sb==MinLocation.y)
								{
									(*iter).tag=1;
									break;
								}

								else 
									temp_num_sb=temp_num_sb+1;

							}

						}
					}
					else
					{
						Person person_temp1;
						person_temp1.History_list.push_back(r);
						person_temp1.tag=1;                       		
						Person_List.push_back(person_temp1);
					}
				}
			}
		}

		for(iter=Person_List.begin();iter!=Person_List.end();)
		{		
			list<Person>::iterator iter_temp;
			iter_temp=iter;
			r_temp=(*iter_temp).History_list.back();
			iter++;
			if ((!(*iter_temp).tag) && ((r_temp.y>200 || r_temp.y<50) || (r_temp.x>300 || r_temp.x<50)) || (r_temp.y>10))
				Person_List.erase(iter_temp);
		}
		


		for(iter=Person_List.begin();iter!=Person_List.end();)
		{		
			list<Person>::iterator iter_temp;
			iter_temp=iter;
			r=(*iter_temp).History_list.back();

			//******face*******
			/*if (r.x == 0 && r.y == 0)
			{
				iter++;
				continue;
			}*/


			int track_object = -1;
			cvCvtColor( Image, hsv, CV_BGR2HSV );
			if( track_object )
			{
				int vmin = 10, vmax = 40, smin = 20, bin_w ,hdims=16, backproject_mode = 0;
				int _vmin = vmin, _vmax = vmax;
				float hranges_arr[] = {0,180};
				float* hranges = hranges_arr;
				IplImage *mask = 0,*hue=0,*histimg=0,*backproject = 0;
				mask = cvCreateImage( cvGetSize(frame), 8, 1 );
				hue = cvCreateImage( cvGetSize(frame), 8, 1 );
				CvHistogram *hist = 0;
				hist = cvCreateHist( 1, &hdims, CV_HIST_ARRAY, &hranges, 1 );
				histimg = cvCreateImage( cvSize(320,200), 8, 3 );
				backproject = cvCreateImage( cvGetSize(frame), 8, 1 );

				CvConnectedComp track_comp;
				CvBox2D track_box;

				cvInRangeS( hsv, cvScalar(0,smin,MIN(_vmin,_vmax),0),
					cvScalar(180,256,MAX(_vmin,_vmax),0), mask );
				cvSplit( hsv, hue, 0, 0, 0 );

				if( track_object < 0 )
				{
					float max_val = 0.f;
					cvSetImageROI( hue, r );
					cvSetImageROI( mask, r );
					cvCalcHist( &hue, hist, 0, mask );
					cvGetMinMaxHistValue( hist, 0, &max_val, 0, 0 );
					cvConvertScale( hist->bins, hist->bins, max_val ? 255. / max_val : 0., 0 );
					cvResetImageROI( hue );
					cvResetImageROI( mask );
					track_window = r;
					track_object = 1;
					cvZero( histimg );
					bin_w = histimg->width / hdims;
					for( i = 0; i < hdims; i++ )
					{
						int val = cvRound( cvGetReal1D(hist->bins,i)*histimg->height/255 );
						CvScalar color = hsv2rgb(i*180.f/hdims);
						cvRectangle( histimg, cvPoint(i*bin_w,histimg->height),
							cvPoint((i+1)*bin_w,histimg->height - val),
							color, -1, 8, 0 );
					}
				}

				cvCalcBackProject( &hue, backproject, hist );
				cvAnd( backproject, mask, backproject, 0 );
				cvCamShift( backproject, track_window,
					cvTermCriteria( CV_TERMCRIT_EPS | CV_TERMCRIT_ITER, 10, 1 ),
					&track_comp, &track_box );
				track_window = track_comp.rect;

				if( backproject_mode )
					cvCvtColor( backproject, Image, CV_GRAY2BGR );

				if( Image->origin )
					track_box.angle = -track_box.angle;

				cvRectangle( Image, cvPoint(track_window.x,track_window.y), 
					cvPoint(track_window.x + track_window.width, track_window.y + track_window.height),
					cvScalar(255,0,0));

				(*iter).History_list.push_back(track_window);

				iter++;

			}

		}
		printf("%d\n", Person_List.size());
		for(iter=Person_List.begin();iter!=Person_List.end();)
		{
			(*iter).tag=0;
			(*iter).vel.flag = 0;

			track_window = (*iter).History_list.back();

			cvRectangle(Image, cvPoint(track_window.x, track_window.y),
				cvPoint(track_window.x + track_window.width, track_window.y + track_window.height),
				cvScalar(0, 0, 255));

			iter++;
		}


		dst_img2 =(IplImage *) cvClone (frame);
		IplImage* frame1_1C = cvCreateImage(cvGetSize(frame), IPL_DEPTH_8U, 1);
		cvCvtColor(frame,frame1_1C,CV_BGR2GRAY);
		frame = cvQueryFrame( capture );
		if (frame == NULL)
		{
			fprintf(stderr, "Error: Hmm. The end came sooner than we thought.\n");
			return -1;
		}
		IplImage* frame2_1C = cvCreateImage(cvGetSize(frame), IPL_DEPTH_8U, 1);
		cvCvtColor(frame,frame2_1C,CV_BGR2GRAY);

		velx = cvCreateMat (rows, cols, CV_32FC1);
		vely = cvCreateMat (rows, cols, CV_32FC1);
		cvSetZero (velx);
		cvSetZero (vely);
		criteria = cvTermCriteria (CV_TERMCRIT_ITER | CV_TERMCRIT_EPS, 20, 0.1);

		cvCalcOpticalFlowHS (frame1_1C, frame2_1C, 0, velx, vely, 100.0, criteria);//计算光流 


		int alpha=1,beta=2; //计算光流能量
		double Ep_temp=0; //个人能量

		double tp=0; //速度值
		for (i = 0; i < cols; i++)//读取每个坐标的光流和速度
		{
			for (j = 0; j < rows; j++)
			{
				dx = (int) cvGetReal2D (velx, j, i);// velx已经为光流场形式
				dy = (int) cvGetReal2D (vely, j, i);
				tp=sqrt(double(dx*dx+dy*dy));
				cvmSet(theta,j,i,atan2(double(dx),double(dy)));//每个点的运动方向角
				if (tp >= 0)
				{
					cvmSet(vel, j, i, tp);
					//printf("tp>0 %d\n", tp);
				}
				else 
					cvmSet(vel,j,i,0);
				//if (dx < 4)
				//	cvmSet(velx,j,i,0);
				//if (dy < 4)
				//	cvmSet(vely,j,i,0);

			}
		}

		double w=30,h=30;
		int dx=0,dy=0;
		int left,right,up,down,p,q;
		int area_num = 0;

		left=i-ceil(w/2);
		right=i+ceil(w/2);
		up=j-ceil(h/2);
		down=j+ceil(h/2);

		if(left<0) left=0;
		if(right>cols) right=cols;
		if(up<0) up=0;
		if(down>rows) down=rows;
		double t = (double)cvGetTickCount();
		
		for (i = 0; i < cols; i++)
		{
			for (j = 0; j < rows; j++) 
			{
				//for (p=left;p<right;p++)
				//{	
				//	for (q=up;q<down;q++)
				//	{
				//		dx += (int) cvGetReal2D (velx,q,p); //从光流场velx中读取q，p位置的光流值，然后与之前的值相加。
				//		dy += (int) cvGetReal2D (vely,q,p);//从光流场vely中读取q，p位置的光流值，然后与之前的值相加。

				//	}
				//}				
				//cvmSet(velmax,j,i,sqrt(double(dx*dx+dy*dy)));//速度最大值？
				//cvmSet(thetamax,j,i,atan2(double(dx),double(dy)));//求方向角最大值
				dx = 0; dy = 0;
				for (iter = Person_List.begin(); iter != Person_List.end();)
				{
					list<Person>::iterator iter_temp;
					iter_temp = iter;
					r = (*iter_temp).History_list.back();
					if (i > r.x && i<(r.x + r.width) && j>r.y && j < (r.y + r.height))
					{
						if ((*iter_temp).vel.flag == 0)
						{
							int v_temp = 0, dx_temp, dy_temp;
							for (p = r.x; p < r.x + r.width; p++)
							{
								for (q = r.y; q < r.y + r.height; q++)
								{
									dx_temp = (int)cvGetReal2D(velx, q, p);
									dy_temp = (int)cvGetReal2D(vely, q, p);
									if (dx_temp*dx_temp + dy_temp*dy_temp > v_temp)
									{
										v_temp = dx_temp*dx_temp + dy_temp*dy_temp;
										dx = dx_temp;
										dy = dy_temp;
									}
								}
							}
							(*iter_temp).vel.dx = dx;
							(*iter_temp).vel.dy = dy;
							(*iter_temp).vel.flag = 1;
						}
						else
						{
							dx = (*iter_temp).vel.dx;
							dy = (*iter_temp).vel.dy;
						}
						break;
					}
					else
					{
						dx = (int)cvGetReal2D(velx, j, i);
						dy = (int)cvGetReal2D(vely, j, i);
					}
					iter++;
				}
				cvmSet(velmax,j,i,sqrt(double(dx*dx+dy*dy)));
				cvmSet(thetamax,j,i,atan2(double(dx),double(dy)));
			}
		}
		

		cvAbsDiff(theta,thetamax,delt_thetamax);//相对于最大方向角的变化值
		if(flag==1) 
		{
			cvAbsDiff(theta,tmpp,tmptheta);
			flag=0;
		}
		cvAbsDiff(theta,tmptheta,delt_theta);//相对于自己前后两帧的变化值
		cvAbsDiff(theta,tmpp,tmptheta);
		for (i = 0; i < cols; i++)
		{
			for (j = 0; j < rows; j++) 
			{
				dx = (int) cvGetReal2D (delt_theta, j, i);//这里x并不是只x轴，是delt_theta矩阵中，（i，j）位置处的方向角差值
				dy = (int) cvGetReal2D (delt_thetamax, j, i);//delt_thetamax矩阵中，（i，j）位置处的方向角差值
				cvmSet(m,j,i,alpha*dx*dx/4/3.14/3.14+beta*dy*dy/4/3.14/3.14);//求每个像素点质量
			}
		}

		
		for (i = 0; i < cols; i += 5)
		{
			for (j = 0; j < rows; j += 5) 
			{
				dx = (int) cvGetReal2D (velx, j, i);
				dy = (int) cvGetReal2D (vely, j, i);
				if ((5<dx<10)&&(5<dy<10))
				cvLine (dst_img2, cvPoint (i, j), cvPoint (i + dx, j + dy), CV_RGB (255, 0, 0), 1, CV_AA, 0);
			}
		}
		

		dx=0;
		dy=0;
		left=i-ceil(w/2);
		right=i+ceil(w/2);
		up=j-ceil(h/2);
		down=j+ceil(h/2);
		// 计算个体能量
		if(left<0) left=0;
		if(right>cols) right=cols;
		if(up<0) up=0;
		if(down>rows) down=rows;
		for(iter=Person_List.begin();iter!=Person_List.end();)
		{		
			list<Person>::iterator iter_temp;
			iter_temp=iter;

			//for (p=left;p<right;p++)
			//{	
			//	for (q=up;q<down;q++)
			//	{
			//		dx = (int) cvGetReal2D (m,q,p);// m是质量矩阵
			//		dy = (int) cvGetReal2D (vel,q,p); //vel是光流矩阵
			//		//printf("dx dy: %d %d\n", dx, dy);
			//		Ep_temp=Ep_temp+dx*dy*dy;//质量*速度*速度*速度
			//	}
			//}
			r = (*iter_temp).History_list.back();
			for (p = r.x; p<r.x + r.width; p++)
			{
				for (q = r.y; q<r.y + r.height; q++)
				{
					dx = (int)cvGetReal2D(m, q, p);
					dy = (int)cvGetReal2D(vel, q, p);
					Ep_temp = Ep_temp + dx*dy*dy;
				}
			}
			iter++;
		}
		Ep[frame_tag]=Ep_temp;


		CvRect Dis_r[20];
		int del_j=0,del_k=0;
		double SB_dis,m_j,m_k;
		dx=0;
		Ep_temp=0;

		// 计算交互能量
		i=0;
		for(iter=Person_List.begin();iter!=Person_List.end();)
		{
			list<Person>::iterator iter_temp;
			iter_temp=iter;
			i=i+1;
			iter++;
		}
		for(j=0;j<i;j++)
		{
			for(k=j+1;k<i;k++)
			{
				del_j=j;del_k=k;
				SB_dis=(Dis_r[j].x-Dis_r[k].x)*(Dis_r[j].x-Dis_r[k].x)+(Dis_r[j].y-Dis_r[k].y)*(Dis_r[j].y-Dis_r[k].y)+1;
				for(iter=Person_List.begin();iter!=Person_List.end();)//j物体质量
				{
					list<Person>::iterator iter_temp;
					iter_temp=iter;
					if(del_j==0)
					{
						r=(*iter_temp).History_list.back();
						for (p=r.x;p<r.x+r.width;p++)
						{	
							for (q=r.y;q<r.y+r.height;q++)
							{
								dx += (int) cvGetReal2D (m,q,p);

							}
						}
						m_j=dx;
						break;
					}
					else
					{
						del_j=del_j-1;
						iter++;
					}
				}

				for(iter=Person_List.begin();iter!=Person_List.end();)//k物体质量
				{
					list<Person>::iterator iter_temp;
					iter_temp=iter;
					if(del_k==0)
					{
						r=(*iter_temp).History_list.back();
						for (p=r.x;p<r.x+r.width;p++)
						{	
							for (q=r.y;q<r.y+r.height;q++)
							{
								dx += (int) cvGetReal2D (m,q,p);
							}
						}
						m_k=dx;
						break;
					}
					else
					{
						del_k=del_k-1;
						iter++;
					}
				}
				Ep_temp=Ep_temp+m_k*m_j/SB_dis/SB_dis; //交互能量
			}
		}
		Ein[frame_tag]=Ep_temp;

		E[frame_tag]=Ep[frame_tag];



		//cvNamedWindow ("ImageHS", 1);
		//cvShowImage ("ImageHS", dst_img2);		
		//cvShowImage("Imask",Imask); 
		cvShowImage("src",Image);
		sprintf(Image_name,"%d.jpg",n++);
		//cvSaveImage(Image_name, dst_img2);
		current_frame=current_frame+1;

		if (current_frame >= number_of_frames - 1) 
			current_frame = number_of_frames - 10;


		frame_tag = frame_tag + 1;

		if ((c == 27) | (frame_tag == 150))
			break;

	}	


	ofstream fout("abc.txt");
	for(int j=0;j<200;j++)
	{
		fout<<E[j];
		fout<<"\n";
	}	
	printf("success.\n");
	//cout<<"\n detection time: 182.59361ms\n";

	return 0;

	/*
	ofstream outfile("abc.txt");
	for(int j=0;j<1000;j++)
	outfile<<E[j]<<endl;
	outfile.close();*/

}


#ifdef _EiC
main(1,"camshiftdemo.c");
#endif



/*
int main(int argc, char* argv[])
{ 

IplImage *img=cvLoadImage("Imback.jpg",1);
CvScalar s;

for(int i=0;i<img->height;i++)
{
for(int j=0;j<img->width;j++)
{
s=cvGet2D(img,i,j);               // get the (i,j) pixel value		
cvSet2D(img,i,j,s);               //set the (i,j) pixel value
}	
}

cvNamedWindow("Image",1); 
//cvShowImage("Image",img);
cvWaitKey(0); //等待按键 

cvDestroyWindow( "Image" );//销毁窗口
cvReleaseImage( &img ); //释放图像 

return 0;
} 
*/


