#include <iostream>
#include <iomanip>
#include "opencv2/core/core.hpp"
#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/nonfree/nonfree.hpp"
#include "opencv2/imgproc/imgproc_c.h"
#include "opencv2/legacy/legacy.hpp"
#include "opencv2/legacy/compat.hpp"

using namespace cv;
using namespace std;

int main()
{
    Mat leftImg=imread("e://left.jpg");
    Mat rightImg=imread("e://right.jpg");
    if(leftImg.data==NULL||rightImg.data==NULL)
        return 0;
    //ת���ɻҶ�ͼ
    Mat leftGray;
    Mat rightGray;

    cvtColor(leftImg,leftGray,CV_BGR2GRAY);
    cvtColor(rightImg,rightGray,CV_BGR2GRAY);

    //��ȡ����ͼ���Ĺ�ͬ������
    int minHessian=400;
    SurfFeatureDetector detector(minHessian);
    vector<KeyPoint> leftKeyPoints,rightKeyPoints;
    detector.detect(leftGray,leftKeyPoints);
    detector.detect(rightGray,rightKeyPoints);
    SurfDescriptorExtractor extractor;

    Mat leftDescriptor,rightDescriptor;
    extractor.compute(leftGray,leftKeyPoints,leftDescriptor);
    extractor.compute(rightGray,rightKeyPoints,rightDescriptor);
    FlannBasedMatcher matcher;
    vector<DMatch> matches;
    matcher.match(leftDescriptor,rightDescriptor,matches);

    int matchCount=leftDescriptor.rows;
    if(matchCount>15)
    {
        matchCount=15;
        //sort(matches.begin(),matches.begin()+leftDescriptor.rows,DistanceLessThan);
        sort(matches.begin(),matches.begin()+leftDescriptor.rows);
    }

    vector<Point2f> leftPoints;
    vector<Point2f> rightPoints;

    for(int i=0; i<matchCount; i++)
    {
        leftPoints.push_back(leftKeyPoints[matches[i].queryIdx].pt);
        rightPoints.push_back(rightKeyPoints[matches[i].trainIdx].pt);
    }
    //��ȡ����ͼ�����ұ�ͼ����ͶӰӳ����ϵ
    Mat homo=findHomography(leftPoints,rightPoints);
    Mat shftMat=(Mat_<double>(3,3)<<1.0,0,leftImg.cols, 0,1.0,0, 0,0,1.0);
    //ƴ��ͼ��
    Mat tiledImg;
    warpPerspective(leftImg,tiledImg,shftMat*homo,Size(leftImg.cols+rightImg.cols,rightImg.rows));
    rightImg.copyTo(Mat(tiledImg,Rect(leftImg.cols,0,rightImg.cols,rightImg.rows)));
    //����ͼ��
    imwrite("e://tiled.jpg",tiledImg);

    //��ʾƴ�ӵ�ͼ��
    imshow("tiled image",tiledImg);
    waitKey(0);
    return 0;

}
