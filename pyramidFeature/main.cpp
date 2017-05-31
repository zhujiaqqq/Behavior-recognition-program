#include <opencv2/opencv.hpp>
#include <iostream>
#include <fstream>
using namespace cv;
using namespace std;

void countBlock(Mat& img, int* p, const int value);
void one2four(Mat& img, Mat& ROI1, Mat& ROI2, Mat& ROI3, Mat& ROI4);

int main(void)
{
    ofstream fs("d:\\QtTest\\pyramidFeature\\abc.txt");
    if(!fs)
    {
        cout << "File open error!" << endl;
        return 0;
    }
    char *cstr=new char[100];
    for(int i = 1; i <=4; i++)
    {
        //int p[16]={0};
        int p4[21][16]={0};
        Mat ROI[4];
        Mat ROII[4][4];
        sprintf(cstr,"%s%d%s","d:\\QtTest\\pyramidFeature\\",i,".jpg");
        Mat img = imread(cstr);
        threshold(img, img, 50, 255, THRESH_BINARY);
//*********************
//归一化并保存
//*********************
        resize(img, img, Size(48,100));
        imwrite(cstr,img);
//*********************
threshold(img, img, 50, 255, THRESH_BINARY);
        //********计算原图块*********
        countBlock(img, *p4, 1);
        //************一分为四****************
        one2four(img, ROI[0], ROI[1], ROI[2], ROI[3]);
        for(int i = 0; i < 4; i++)
        {
            //************计算四分之一图块*********
            countBlock(ROI[i], *p4+16+16*i, 4);
            //************一分为四****************
            one2four(ROI[i], ROII[i][0], ROII[i][1], ROII[i][2], ROII[i][3]);

            for(int j = 0; j < 4; j++)
            {
                //**********计算十六分之一图块********
                countBlock(ROII[i][j], *p4+80+16*(i*4+j),16);
            }
        }

//*********************
//输出块个数数组
//*********************

        for(int m = 0; m < 21; m++)
        {
            for(int n = 0; n < 16; n++)
            {
                cout << p4[m][n] << " ";
                fs << p4[m][n] << " ";
            }
            cout << endl;
            fs <<  endl;
        }

//*********************
        imshow("aa",img);
        fs << "\n" << endl;
        waitKey(0);
    }
    fs.close();
    return 0;
}

void countBlock(Mat& img, int* p, const int value)
{
    for(int i = 0; i < img.rows - 1; i++)
    {
        int ii = i+1;
        uchar* data = img.ptr<uchar>(i);
        uchar* nextdata = img.ptr<uchar>(ii);
        for(int j = 0; j < img.cols - 1; j++)
        {
            unsigned int pp =  0;
            pp = pp | (int)data[j]<< 24;
            pp = pp | (int)data[j+1]<< 16;
            pp = pp | (int)nextdata[j]<< 8;
            pp = pp | (int)nextdata[j+1];
            //cout << p << endl;
            switch (pp)
            {
            case 0x00000000:
                p[0]+=value;                break;
            case 0x000000ff:
                p[1]+=value;                break;
            case 0x0000ff00:
                p[2]+=value;                break;
            case 0x0000ffff:
                p[3]+=value;                break;
            case 0x00ff0000:
                p[4]+=value;                break;
            case 0x00ff00ff:
                p[5]+=value;                break;
            case 0x00ffff00:
                p[6]+=value;                break;
            case 0x00ffffff:
                p[7]+=value;                break;
            case 0xff000000:
                p[8]+=value;                break;
            case 0xff0000ff:
                p[9]+=value;                break;
            case 0xff00ff00:
                p[10]+=value;                break;
            case 0xff00ffff:
                p[11]+=value;                break;
            case 0xffff0000:
                p[12]+=value;                break;
            case 0xffff00ff:
                p[13]+=value;                break;
            case 0xffffff00:
                p[14]+=value;                break;
            case 0xffffffff:
                p[15]+=value;                break;
            default:
                break;
            }
        }
    }
//    for(int n = 0; n < 16; n++)
//    {
//        cout << p[n] << " ";
//    }
//    cout << endl;
}

void one2four(Mat& img, Mat& ROI1, Mat& ROI2, Mat& ROI3, Mat& ROI4)
{
    ROI1 = img(Rect(0, 0, img.cols/2, img.rows/2));

    ROI2 = img(Rect(img.cols/2, 0, img.cols/2, img.rows/2));

    ROI3 = img(Rect(0, img.rows/2, img.cols/2, img.rows/2));

    ROI4 = img(Rect(img.cols/2, img.rows/2, img.cols/2, img.rows/2));

}
