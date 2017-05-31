#ifndef CODEBOOK_H_INCLUDED
#define CODEBOOK_H_INCLUDED

#include <iostream>
#include "opencv2/opencv.hpp"
using namespace cv;
using namespace std;

#define CHANNELS 3


class CodeWord
{
public:
	uchar learnHigh[CHANNELS];
	uchar learnLow[CHANNELS];
	uchar max[CHANNELS];
	uchar min[CHANNELS];
	int t_last_update;
	int stale;
};

class CodeBook
{
public:
	list<CodeWord> codeElement;
	int numEntries;
	int t;

};

class BackgroundSubtractorCodeBook
{
public:
	BackgroundSubtractorCodeBook();
	~BackgroundSubtractorCodeBook();
	void updateCodeBook(const Mat &inputImage);
	void initialize(const Mat &inputImage, Mat &outputImage);
	void clearStaleEntries();
	void backgroundDiff(const Mat& inputImage, Mat& outputImage);

private:
	Mat yuvImage;
	Mat maskImage;
	CodeBook* codebookVec;
	unsigned int cbBounds[CHANNELS];
	uchar* pColor;
	uchar* pMask;
	int imageSize;
	int nChannels;
	int minMod[CHANNELS];
	int maxMod[CHANNELS];

	uchar maskPixelCodeBook;

	void _updateCodeBookPerpixel(int pixIndex);
	void _clearSraleEntriesPerPixel(int pixIndex);
	uchar _backgroundDiff(int pixIndex);
};


#endif // CODEBOOK_H_INCLUDED
