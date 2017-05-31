#include<iostream>
#include<opencv2/opencv.hpp>
#include<fstream>
using namespace std;
using namespace cv;

int main(){
    Mat img = imread("10.bmp");
    cvtColor(img, img, CV_BGR2GRAY);
//    threshold(img, img,50, 255, THRESH_BINARY);
//    Canny(img, img, 150, 100, 3);
    ofstream SaveFile("img.txt");

    int nRows = img.rows;
    int nCols = img.cols;

    int sum_x = 0, sum_y = 0, area = 0,n=0;
    uchar x[200];
    uchar y[200];
    CvPoint midPoint;
    CvPoint endPoint[200];

    uchar* p;

    for(int i = 0; i < nRows; i++){
        p = img.ptr<uchar>(i);

        for(int j = 0; j < nCols; j++){
            if(p[j] == 255){
                sum_x += i;
                sum_y += j;
                area += 1;
                if(area%5==0){
                    SaveFile << i << "\t" << j << endl;
                    endPoint[n].x=j;
                    endPoint[n].y=i;
                    n++;

                }
//                cout << "x:" << i << "\t" << "y:" << j << endl;

            }
        }
    }



    midPoint.x = sum_y / area;
    midPoint.y =sum_x / area ;
    SaveFile << midPoint.x << "\t" << midPoint.y << endl;
    img.at<uchar>(sum_x / area, sum_y / area) = 255;
//    cout << area << endl;
    for(int m=0; m < n;m++){
        line(img, midPoint, endPoint[m], cvScalar(255));
    }
    SaveFile.close();
    imshow("img", img);
    imwrite("10.jpg",img);
    waitKey(0);
    return 0;
}


