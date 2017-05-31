#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QString>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <QStringList>
#include <QFileDialog>
#include <string>
#include <QDebug>
#include "my_thread.h"
#include "widget.h"
using namespace std;
using namespace cv;

namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = 0);
    ~Widget();
    void detectAndDraw( Mat& img, CascadeClassifier& cascade,
                        CascadeClassifier& nestedCascade,
                        double scale, bool tryflip );
    QImage cvMat2QImage(const cv::Mat& mat);
    //void test();
    //VideoCapture cap;
private slots:
    void on_pushButton_4_clicked();

    void on_radioButton_clicked();

    void on_close_pushButton_clicked();

    void on_neutral_clicked();

    void on_happy_clicked();

    void on_anger_clicked();

    void on_sadness_clicked();

    void on_add_pushButton_clicked();

    void on_train_pushButton_clicked();

    void on_save_pushButton_clicked();

    void on_load_pushButton_clicked();

    void on_test_pushButton_clicked();

private:
    Ui::Widget *ui;
//    QString m_sFace;
    int m_iFace;
    vector<Mat> images;
    vector<int> labels;
//    Ptr<FaceRecognizer> model = createLBPHFaceRecognizer();
//    QImage m_qImage;

    bool stop = false;
    My_thread *threadA = new My_thread();
};

#endif // WIDGET_H
