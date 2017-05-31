#-------------------------------------------------
#
# Project created by QtCreator 2016-05-27T16:40:45
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Dome
TEMPLATE = app


SOURCES += main.cpp\
        widget.cpp \
    my_thread.cpp

HEADERS  += widget.h \
    my_thread.h

FORMS    += widget.ui

INCLUDEPATH+=D:\opencv4QT\include\opencv\
                    D:\opencv4QT\include\opencv2\
                    D:\opencv4QT\include

LIBS+=D:\opencv4QT\lib\libopencv_calib3d2410.dll.a\
        D:\opencv4QT\lib\libopencv_contrib2410.dll.a\
        D:\opencv4QT\lib\libopencv_core2410.dll.a\
        D:\opencv4QT\lib\libopencv_features2d2410.dll.a\
        D:\opencv4QT\lib\libopencv_flann2410.dll.a\
        D:\opencv4QT\lib\libopencv_gpu2410.dll.a\
        D:\opencv4QT\lib\libopencv_highgui2410.dll.a\
        D:\opencv4QT\lib\libopencv_imgproc2410.dll.a\
        D:\opencv4QT\lib\libopencv_legacy2410.dll.a\
        D:\opencv4QT\lib\libopencv_ml2410.dll.a\
        D:\opencv4QT\lib\libopencv_objdetect2410.dll.a\
        D:\opencv4QT\lib\libopencv_video2410.dll.a

