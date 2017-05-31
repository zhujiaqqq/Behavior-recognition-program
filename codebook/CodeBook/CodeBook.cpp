#include "CodeBook.h"

BackgroundSubtractorCodeBook::BackgroundSubtractorCodeBook(){
    nChannels = CHANNELS;
}

void BackgroundSubtractorCodeBook::initialize(const Mat &inputRGBImage, Mat &outputImage){
    if(inputRGBImage.empty()){
        return;
    }
    if(yuvImage.empty()){
        yuvImage.create(inputRGBImage.size(),inputRGBImage.type());
    }
    if(maskImage.empty()){
        maskImage.create(inputRGBImage.size(),CV_8UC1);
        Mat temp(inputRGBImage.rows,inputRGBImage.cols,CV_8UC1,Scalar::all(255));
        maskImage = temp;
    }

    imageSize = inputRGBImage.cols*inputRGBImage.rows;

    codebookVec = new CodeBook[imageSize];
    for(int i=0; i < imageSize; ++i){
        codebookVec[i].numEntries = 0;
    }
    for(int i=0; i<nChannels; i++){
        cbBounds[i] = 10;
        minMod[i] = 20;
        maxMod[i] = 20;
    }
    outputImage = maskImage;
}

void BackgroundSubtractorCodeBook::updateCodeBook(const Mat &inputImage){
    cvtColor(inputImage, yuvImage, CV_RGB2YCrCb);//CV_BGR2YCrCb
    pColor = yuvImage.data;

    for(int c = 0; c < imageSize; ++c){
        _updateCodeBookPerpixel(c);
        pColor+=3;
    }
}

void BackgroundSubtractorCodeBook::_updateCodeBookPerpixel(int pixelIndex){
    if(codebookVec[pixelIndex].numEntries == 0){
        codebookVec[pixelIndex].t = 0;
    }
    codebookVec[pixelIndex].t+=1;

    int n;
    unsigned int high[3],low[3];
    for(n=0; n<nChannels; n++){
        high[n] = *(pColor+n)+*(cbBounds+n);
        if(high[n] > 255){
            high[n] = 255;
        }
        low[n] = *(pColor+n)-*(cbBounds+n);
        if(low[n] < 0){
            low[n] = 0;
        }
    }

    int matchChannel;
    list<CodeWord>::iterator jList;
    list<CodeWord>::iterator jListAfterPush;

    for(jList=codebookVec[pixelIndex].codeElement.begin(); jList!=codebookVec[pixelIndex].codeElement.end(); ++jList){
        matchChannel = 0;
        for(n=0; n<nChannels; n++){
            if( ((*jList).learnLow[n]<=*(pColor+n))&&((*(pColor+n)<=(*jList).learnHigh[n])) ){
                matchChannel++;
            }
        }

        if(matchChannel == nChannels){
            (*jList).t_last_update = codebookVec[pixelIndex].t;
            for(n=0; n<nChannels; n++){
                if((*jList).max[n] < *(pColor+n)){
                    (*jList).max[n] = *(pColor+n);
                }
                else if((*jList).min[n] > *(pColor+n)){
                    (*jList).min[n] = *(pColor+n);
                }
            }
            break;
        }
    }

    if(jList == codebookVec[pixelIndex].codeElement.end()){
        CodeWord newElement;
        for(n=0; n<nChannels; n++){
            newElement.learnHigh[n] = high[n];
            newElement.learnLow[n] = low[n];
            newElement.max[n] = *(pColor+n);
            newElement.min[n] = *(pColor+n);
        }

        newElement.t_last_update = codebookVec[pixelIndex].t;
        newElement.stale = 0;
        codebookVec[pixelIndex].numEntries+=1;
        codebookVec[pixelIndex].codeElement.push_back(newElement);
    }

    for(jListAfterPush=codebookVec[pixelIndex].codeElement.begin(); jListAfterPush!=codebookVec[pixelIndex].codeElement.end(); ++jListAfterPush){
        int negRun = codebookVec[pixelIndex].t - (*jListAfterPush).t_last_update;
        if((*jListAfterPush).stale < negRun){
            (*jListAfterPush).stale = negRun;
        }
    }

    for(n=0; n<nChannels; n++){
        if((*jList).learnHigh[n] < high[n]){
            (*jList).learnHigh[n]+=1;
        }
        if((*jList).learnLow[n] > low[n]){
            (*jList).learnLow[n] -= 1;
        }
    }

    return;
}

void BackgroundSubtractorCodeBook::clearStaleEntries(){
    for(int i=0; i<imageSize; i++){
        _clearSraleEntriesPerPixel(i);
    }
}

void BackgroundSubtractorCodeBook::_clearSraleEntriesPerPixel(int pixelIndex){
    int staleThresh = codebookVec[pixelIndex].t;
    for(list<CodeWord>::iterator itor=codebookVec[pixelIndex].codeElement.begin(); itor!=codebookVec[pixelIndex].codeElement.end();){
        if((*itor).stale > staleThresh){
            itor = codebookVec[pixelIndex].codeElement.erase(itor);
        }
        else
        {
            (*itor).stale = 0;
            (*itor).t_last_update = 0;
            itor++;
        }
    }
    codebookVec[pixelIndex].t = 0;
    codebookVec[pixelIndex].numEntries = (int)codebookVec[pixelIndex].codeElement.size();
    return;
}

void BackgroundSubtractorCodeBook::backgroundDiff(const Mat &inputImage, Mat &outputImage){
    cvtColor(inputImage, yuvImage, CV_RGB2YCrCb);
    pColor = yuvImage.data;

    pMask = maskImage.data;
    for(int c=0; c<imageSize; c++){
        maskPixelCodeBook = _backgroundDiff(c);
        *pMask++ = maskPixelCodeBook;
        pColor += 3;
    }
    outputImage = maskImage.clone();
}

uchar BackgroundSubtractorCodeBook::_backgroundDiff(int pixelIndex){
    int matchChannels;
    list<CodeWord>::iterator itor;
    for(itor = codebookVec[pixelIndex].codeElement.begin(); itor != codebookVec[pixelIndex].codeElement.end(); ++itor){
        matchChannels = 0;
        for(int n=0; n<nChannels; ++n){
            if(((*itor).min[n]-minMod[n]<=*(pColor+n)) && (*(pColor+n) <= (*itor).max[n]+maxMod[n])){
                matchChannels++;
            }
            else{
                break;
            }
        }

        if(matchChannels == nChannels){
            break;
        }
    }

    if(itor == codebookVec[pixelIndex].codeElement.end()){
        return 255;
    }
    return 0;
}

BackgroundSubtractorCodeBook::~BackgroundSubtractorCodeBook(){
    delete [] codebookVec;
}









