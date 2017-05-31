#ifndef MY_THREAD_H
#define MY_THREAD_H

#include <QThread>
#include <iostream>
//#include "widget.h"
#include <opencv2/opencv.hpp>
#include <QImage>
#include <iostream>
using namespace std;
using namespace cv;
class My_thread: public QThread
{
    Q_OBJECT
public:
    My_thread();
    //void setMessage(QString message);
    void stop();
    void test(VideoCapture cap);
    QImage cvMat2QImage(const cv::Mat& mat);
    QImage detectAndDraw( Mat& img, CascadeClassifier& cascade,
                        CascadeClassifier& nestedCascade,
                        double scale, bool tryflip,int *count );
    Ptr<FaceRecognizer> model = createLBPHFaceRecognizer();
protected:
    void run();
    //void printMessage();

private:
    //QString messageStr;
    volatile bool stopped;
    VideoCapture cap;
    QImage m_qImage;
    QString m_sFace;

};

#endif // MY_THREAD_H
