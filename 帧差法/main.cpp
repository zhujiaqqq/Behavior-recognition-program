#include <iostream>
#include <string>
#include <stdio.h>
#include <opencv2/opencv.hpp>
using namespace cv;
using namespace std;

int main(){
	VideoCapture capture("lena_run1.avi");
	if(!capture.isOpened())
		return -1;
	double rate = capture.get(CV_CAP_PROP_FPS);
	int delay = 1000/rate;
	Mat framePro,frame,dframe;
	bool flag = false;

	char *cstr = new char[100];
	int i = 0;
	namedWindow("image",CV_WINDOW_AUTOSIZE);
	namedWindow("test",CV_WINDOW_AUTOSIZE);

	while(capture.read(frame)){
        cvtColor(frame,frame,CV_BGR2GRAY);
		if(false == flag){
			framePro = frame.clone();

			flag = true;
		}
		else{
			absdiff(frame,framePro,dframe);
			framePro = frame.clone();
			threshold(dframe,dframe,80,255,CV_THRESH_BINARY);
			imshow("image",frame);
			imshow("test",dframe);
			sprintf(cstr,"%s%d%s","e://tt//src",i++,".jpg");
			imwrite(cstr,frame);
			waitKey(delay);
		}
	}
	return 0;
}
