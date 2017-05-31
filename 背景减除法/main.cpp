#include "opencv2/opencv.hpp"
#include <iostream>

using namespace std;
using namespace cv;

int main(){
    int i;
    VideoCapture cap;
    cap.open("daria_jack.avi");

    if(!cap.isOpened()){
        return -1;
    }

    Mat frame, gray;
    Mat background = imread("background.jpg");
    long frameLong = cap.get(CV_CAP_PROP_FRAME_COUNT);

    for(i=0; i<frameLong; i++){
        cap >> frame;
        imshow("frame",frame);
        frame = background - frame;
        cvtColor(frame, gray, CV_BGR2GRAY);
        threshold( gray, gray, 50, 255,0 );
        imshow("  gray",gray);
        waitKey(0);
    }
    return 0;
}
