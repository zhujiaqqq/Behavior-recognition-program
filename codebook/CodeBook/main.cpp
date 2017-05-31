#include "CodeBook.h"
#include "opencv2/opencv.hpp"
using namespace cv;
int main()
{
    VideoCapture cap;
    cap.open("person01_walking_d1_uncomp.avi");

    if(!cap.isOpened()){
        cout << "can not open this video!!" << endl;
        return -1;
    }

    BackgroundSubtractorCodeBook bgcbModel;
    Mat inputImage,outputMaskCodebook;

    for(int i = 0; ; i++){
        cap >>inputImage;

        if(inputImage.empty()){
            break;
        }
        if(i == 0){
            bgcbModel.initialize(inputImage, outputMaskCodebook);
        }
        else if(i <= 20 && i > 0){
            bgcbModel.updateCodeBook(inputImage);
            if(i == 30){
                bgcbModel.clearStaleEntries();
            }
        }
        else{
            bgcbModel.backgroundDiff(inputImage, outputMaskCodebook);
        }

        imshow("image", inputImage);
        imshow("foreground mask", outputMaskCodebook);

        waitKey(30);

    }
    return 0;
}

