#include "cv.h"
#include "highgui.h"
#include "cxcore.h"

/**********************************************************************************/
//设置处理的图像通道数,要求小于等于图像本身的通道数
#define CHANNELS 3
//某些颜色的宏定义
#define CV_CVX_WHITE	CV_RGB(0xff,0xff,0xff)
#define CV_CVX_BLACK	CV_RGB(0x00,0x00,0x00)

//For connected components:
int CVCONTOUR_APPROX_LEVEL = 2;     // Approx.threshold - the bigger it is, the simpler is the boundary
int CVCLOSE_ITR = 1;				// How many iterations of erosion and/or dialation there should be
/**********************************************************************************/


/**********************************************************************************/
//下面为码本码元的数据结构
//处理图像时每个像素对应一个码本code_book,每个码本中可有若干个码元code_element
typedef struct ce
{
	uchar learnHigh[CHANNELS];	// High side threshold for learning
								// 此码元各通道的阀值上限(学习界限)
	uchar learnLow[CHANNELS];	// Low side threshold for learning
								// 此码元各通道的阀值下限
								// 学习过程中如果一个新像素各通道值x[i],均有 learnLow[i]<=x[i]<=learnHigh[i],则该像素可合并于此码元
	uchar max[CHANNELS];		// High side of box boundary
								// 属于此码元的像素中各通道的最大值
	uchar min[CHANNELS];		// Low side of box boundary
								// 属于此码元的像素中各通道的最小值
	int	t_last_update;			// This is book keeping to allow us to kill stale entries
								// 此码元最后一次更新的时间,每一帧为一个单位时间,用于计算stale
	int	stale;					// max negative run (biggest period of inactivity)
								// 此码元最长不更新时间,用于删除规定时间不更新的码元,精简码本
} code_element;					// 码元的数据结构

typedef struct code_book
{
	code_element **cb;			// 码元的二维指针,理解为指向码元指针数组的指针,使得添加码元时不需要来回复制码元,只需要简单的指针赋值即可
	int	numEntries;				// 此码本中码元的数目
	int	t;						// count every access
								// 此码本现在的时间,一帧为一个时间单位；记录从开始或最后一次清除操作之间累积的像素点的数目
} codeBook;						// 码本的数据结构
/**********************************************************************************/


/**********************************************************************************/
// int updateCodeBook( uchar* p, codeBook &c, unsigned* cbBounds, int numChannels )
// Updates the codebook entry with a new data point
//
// p			Pointer to a YUV pixel
// c			Codebook for this pixel
// cbBounds		Learning bounds for codebook (Rule of thumb: 10)
// numChannels	Number of color channels we're learning
//
// NOTES		cbBounds must be of size cbBounds[numChannels]
//
// RETURN		codebook index
/**********************************************************************************/
int cvupdateCodeBook( uchar* p, codeBook &c, unsigned* cbBounds, int numChannels )
{
	if (c.numEntries == 0)					// 码本中码元为零时，在main函数中一开始为零
		c.t = 0;							// 初始化时间为0
	c.t += 1;								// Record learning event
											// 码本时间，记录学习的次数，每调用一次加一，即每一帧图像加一
	/*SET HIGH AND LOW BOUNDS*/
	int n;
	unsigned int high[3], low[3];
	for (n=0; n<numChannels; n++)			//遍历三通道
	{
		high[n] = *(p+n) + *(cbBounds+n);	// *(p+n) 和 p[n] 结果等价，经试验*(p+n)速度更快
		if (high[n]>255)					// high[n] = p[n] + cbBounds[n]，上限阈值
			high[n] = 255;
		low[n] = *(p+n)- *(cbBounds+n);		// low[n] = p[n] - cbBounds[n]，下限阈值
		if (low[n]<0)
			low[n] = 0;						// 用p 所指像素通道数据,加减cbBonds中数值,作为此像素阀值的上下限
	}
	/*SEE IF THIS FITS AN EXISTING CODEWORD*/
	int matchChannel;						// 像素p符合码元的通道数
	int i;									// 码本中码元的序数
	for (i=0; i<c.numEntries; i++)			// 遍历此码本每个码元,测试p像素是否满足其中之一
	{
		matchChannel = 0;
		for (n=0; n<numChannels; n++)		// 遍历每个通道
		{
			if ((c.cb[i]->learnLow[n]<=*(p+n)) && (*(p+n)<=c.cb[i]->learnHigh[n]))	// Found an entry for this channel
																					// 码本c的第i个码元的learnlow[n] <= p[n] <= 码本c的第i个码元的learnhigh[n]
																					// 即如果p像素通道数据在该码元阀值上下限之间
				matchChannel++;														// 如果每个通道都符合，则matchChannel = numChannels
		}
		if (matchChannel == numChannels)	// If an entry was found over all channels
											// 如果p 像素各通道都满足上面条件
		{
			c.cb[i]->t_last_update = c.t;	// 更新该码元时间为码本时间，即当前时间
			for (n=0; n<numChannels; n++)			//对每一通道，调整该码元最大最小值
			{
				if (c.cb[i]->max[n] < *(p+n))		//如果像素p大于码元的max，则码元的max赋值为p
					c.cb[i]->max[n] = *(p+n);
				else if (c.cb[i]->min[n] > *(p+n))	//如果像素p小于码元的min，则码元的min赋值为p
					c.cb[i]->min[n] = *(p+n);
			}
			break;							// 跳出“遍历此码本每个码元”这个循环，即像素p三通道都符合码本中某一码元，则不用遍历以下的码元
		}									// 此时，i<c.numEntries
	}
	/*ENTER A NEW CODE WORD IF NEEDED*/
	if (i==c.numEntries)					// No existing code word found, make a new one
											// p 像素不满足此码本中任何一个码元,下面创建一个新码元
	{
		code_element **foo = new code_element* [c.numEntries+1];	// 为c.numEntries+1个指向码元数组的指针分配空间，比原码本的码元个数多1个
		for (int ii=0; ii<c.numEntries; ii++)
			foo[ii] = c.cb[ii];										// 将原码本的码元赋给新码元，即前c.numEntries个指针指向新分配的每个码元
		foo[c.numEntries] = new code_element;						// 为最后一个新码元申请空间
		if (c.numEntries)
			delete [] c.cb;					// 删除c.cb 指针数组（注意：delete[]与new[]相对应使用）
		c.cb = foo;							// 把foo 头指针赋给c.cb
		for (n=0; n<numChannels; n++)		// 更新新码元各通道数据
		{
			c.cb[c.numEntries]->learnHigh[n] = high[n];		// 新码元的learnhigh为上限阈值
			c.cb[c.numEntries]->learnLow[n] = low[n];		// learnlow为下限阈值
			c.cb[c.numEntries]->max[n] = *(p+n);			// max与min为像素p的值
			c.cb[c.numEntries]->min[n] = *(p+n);
		}
		c.cb[c.numEntries]->t_last_update = c.t;			// 将码元时间设置为码本时间
		c.cb[c.numEntries]->stale = 0;
		c.numEntries += 1;									// 在这里改变码元个数
	}
	/*OVERHEAD TO TRACK POTENTIAL STALE ENTRIES*/
	for (int s=0; s<c.numEntries; s++)
	{
		int negRun = c.t-c.cb[s]->t_last_update;	// This garbage is to track which codebook entries are going stale
													// 计算该码元的不更新时间
		if (c.cb[s]->stale < negRun)
			c.cb[s]->stale = negRun;
	}
	/*SLOWLY ADJUST LEARNING BOUNDS*/
	for (n=0; n<numChannels; n++)					// 如果像素通道数据在高低阀值范围内,但在码元阀值之外,则缓慢调整此码元学习界限
	{
		if (c.cb[i]->learnHigh[n] < high[n])
			c.cb[i]->learnHigh[n] += 1;
		if (c.cb[i]->learnLow[n] > low[n])
			c.cb[i]->learnLow[n] -= 1;
	}

	return(i);
}


/**********************************************************************************/
// uchar cvbackgroundDiff( uchar* p, codeBook &c, int minMod, int maxMod )
// Given a pixel and a code book, determine if the pixel is covered by the codebook
//
// p			pixel pointer (YUV interleaved)
// c			codebook reference
// numChannels  Number of channels we are testing
// maxMod		Add this (possibly negative) number onto max level when code_element determining if new pixel is foreground
// minMod		Subract this (possible negative) number from min level code_element when determining if pixel is foreground
//
// NOTES		minMod and maxMod must have length numChannels, e.g. 3 channels => minMod[3], maxMod[3].
//
// Return		0 => background, 255 => foreground
/**********************************************************************************/
uchar cvbackgroundDiff( uchar* p, codeBook &c, int numChannels, int* minMod, int* maxMod )
{
	int matchChannel;							// 下面步骤和背景学习中查找码元如出一辙
	/*SEE IF THIS FITS AN EXISTING CODEWORD*/
	int i;
	for (i=0; i<c.numEntries; i++)
	{
		matchChannel = 0;
		for (int n=0; n<numChannels; n++)
		{
			if ((c.cb[i]->min[n]-minMod[n]<= *(p+n)) && (*(p+n)<=c.cb[i]->max[n]+maxMod[n]))
				matchChannel++;					// Found an entry for this channel
			else
				break;							// 如果有一通道不符合，则跳出for循环
		}
		if (matchChannel == numChannels)		// 如果第i个码元所有通道都符合(后面的码元不用检测了)，则跳出for循环，此时i<c.numEntries
			break;								// Found an entry that matched all channels
	}
	if (i == c.numEntries)						// 此时没有一个码元符合，即证明是前景，返回255（白色）
		return(255);
	return(0);
}


//UTILITES//////////////////////////////////////////////////////////////////////////
/**********************************************************************************/
// int clearStaleEntries( codeBook &c )
// After you've learned for some period of time, periodically call this to clear
// out stale codebook entries
//
// c			Codebook to clean up
//
// Return		number of entries cleared
/**********************************************************************************/
int cvclearStaleEntries( codeBook &c )
{
	int staleThresh = c.t >> 1;				// 设定刷新时间
	int* keep = new int [c.numEntries];		// 申请一个标记数组，数组元素数目为码本中码元的个数
	int keepCnt = 0;						// 记录不删除码元数目
											// SEE WHICH CODEBOOK ENTRIES ARE TOO STALE
	for (int i=0; i<c.numEntries; i++)		// 遍历码本中每个码元
	{
		if (c.cb[i]->stale > staleThresh)	// 如码元中的不更新时间大于设定的刷新时间,则标记为删除
			keep[i] = 0;					// Mark for destruction，标记
		else
		{
			keep[i] = 1;					// Mark to keep
			keepCnt += 1;					// 记录不删除码元数目
		}
	}
	/*KEEP ONLY THE GOOD*/
	c.t = 0;								// Full reset on stale tracking
											// 码本时间清零
	code_element **foo = new code_element* [keepCnt];	// 申请大小为keepCnt 的码元指针数组
	int k=0;
	for (int ii=0; ii<c.numEntries; ii++)
	{
		if (keep[ii])						// 如果keep[ii] = 0则不进入，对应要删除的码元
		{
			foo[k] = c.cb[ii];
			foo[k]->stale = 0;				// We have to refresh these entries for next clearStale
			foo[k]->t_last_update = 0;
			k++;
		}
	}
	/*CLEAN UP*/
	delete [] keep;
	delete [] c.cb;
	c.cb = foo;								// 把foo 头指针地址赋给c.cb
	int numCleared = c.numEntries - keepCnt;// 被清理的码元个数
	c.numEntries = keepCnt;					// 剩余的码元个数

	return(numCleared);						// 返回被清理的码元个数
}


/**********************************************************************************/
// void cvconnectedComponents( IplImage* mask, int poly1_hull0, float perimScale, int* num, CvRect* bbs, CvPoint* centers )
// This cleans up the forground segmentation mask derived from calls to cvbackgroundDiff
//
// mask			Is a grayscale (8 bit depth) "raw" mask image which will be cleaned up
//
// OPTIONAL PARAMETERS:
// poly1_hull0	If set, approximate connected component by (DEFAULT) polygon, or else convex hull (0)
// perimScale 	Len = image (width+height)/perimScale.  If contour len < this, delete that contour (DEFAULT: 4)
// num			Maximum number of rectangles and/or centers to return, on return, will contain number filled (DEFAULT: NULL)
// bbs			Pointer to bounding box rectangle vector of length num.  (DEFAULT SETTING: NULL)
// centers		Pointer to contour centers vectore of length num (DEFULT: NULL)
/**********************************************************************************/
void cvconnectedComponents( IplImage* mask, int poly1_hull0, float perimScale, int* num, CvRect* bbs, CvPoint* centers )
{
	static CvMemStorage* mem_storage = NULL;
	static CvSeq* contours = NULL;
	/*CLEAN UP RAW MASK*/
	cvMorphologyEx( mask, mask, NULL, NULL, CV_MOP_OPEN, CVCLOSE_ITR );	// 对mask进行开运算（消除高亮的孤立点）
	cvMorphologyEx( mask, mask, NULL, NULL, CV_MOP_CLOSE, CVCLOSE_ITR );// 对mask进行闭运算（消除低亮的孤立点）
	/*FIND CONTOURS AROUND ONLY BIGGER REGIONS*/
	if (mem_storage==NULL)
		mem_storage = cvCreateMemStorage(0);
    else
		cvClearMemStorage(mem_storage);
	CvContourScanner scanner = cvStartFindContours( mask,
													mem_storage,
													sizeof(CvContour),
													CV_RETR_EXTERNAL,
													CV_CHAIN_APPROX_SIMPLE );	// 该函数每次返回一个轮廓
	CvSeq* c;
	int numCont = 0;
	while ((c=cvFindNextContour(scanner)) != NULL)						// 查找剩余轮廓，一直循环，直至为空
	{
		double len = cvContourPerimeter(c);								// 返回轮廓的周长
		double q = (mask->height + mask->width)/perimScale;				// calculate perimeter len threshold
																		// 计算轮廓周长的阈值
		if (len<q)														// Get rid of blob if it's perimeter is too small
			cvSubstituteContour( scanner, NULL );						// 舍弃轮廓周长过小的轮廓
		else															// Smooth it's edges if it's large enough
		{
			CvSeq* c_new;
			if( poly1_hull0 )											// Polygonal approximation of the segmentation
	            c_new = cvApproxPoly( c,								// 若poly1_hull0为1，则进行多边形逼近
									  sizeof(CvContour),
									  mem_storage,
									  CV_POLY_APPROX_DP,
									  CVCONTOUR_APPROX_LEVEL,			// 计算多边形逼近的精度
									  0 );
			else														// Convex Hull of the segmentation
				c_new = cvConvexHull2(c,mem_storage,CV_CLOCKWISE,1);	// 若为0，则进行hull矩操作
            cvSubstituteContour( scanner, c_new );						// 新处理后的序列取代原序列
			numCont++;
        }
	}
	contours = cvEndFindContours( &scanner );
	/*PAINT THE FOUND REGIONS BACK INTO THE IMAGE*/
	cvZero(mask);
	IplImage* maskTemp;
	/*CALC CENTER OF MASS AND OR BOUNDING RECTANGLES，如果num非空就计算某些参数*/
	if (num!=NULL)
	{
		int N =* num, numFilled=0, i=0;
		CvMoments moments;
		double M00, M01, M10;
		maskTemp = cvCloneImage(mask);
		for (i=0,c=contours; c!=NULL; c=c->h_next,i++)
		{
			if (i<N)													// Only process up to *num of them
			{
				cvDrawContours( maskTemp, c, CV_CVX_WHITE, CV_CVX_WHITE, -1, CV_FILLED, 8 );
				/*Find the center of each contour，如果center非空就计算图像重心*/
				if (centers!=NULL)
				{
					cvMoments( maskTemp, &moments, 1 );
					M00 = cvGetSpatialMoment( &moments, 0, 0 );
					M10 = cvGetSpatialMoment( &moments, 1, 0 );
					M01 = cvGetSpatialMoment( &moments, 0, 1 );
					centers[i].x = (int)(M10/M00);						// 通过中心矩计算图像的重心
					centers[i].y = (int)(M01/M00);
				}
				/*Bounding rectangles around blobs，如果bbs非空就计算轮廓的边界框*/
				if (bbs!=NULL)
				{
					bbs[i] = cvBoundingRect(c);							// 计算边界框
				}
				cvZero(maskTemp);
				numFilled++;
			}
			/*Draw filled contours into mask*/
			cvDrawContours( mask, c, CV_CVX_WHITE, CV_CVX_WHITE, -1, CV_FILLED, 8 );
			//draw to central mask
		}	//end looping over contours
		*num = numFilled;
		cvReleaseImage( &maskTemp );
	}
	/*ELSE JUST DRAW PROCESSED CONTOURS INTO THE MASK，如果num为空则只画轮廓就可以了*/
	else
	{
		for (c=contours; c!=NULL; c=c->h_next)
		{
			cvDrawContours( mask, c, CV_CVX_WHITE, CV_CVX_BLACK, -1, CV_FILLED, 8 );
		}
	}
}


int main()
{
	/*需要使用的变量*/
	CvCapture*	capture;
	IplImage*	rawImage;
	IplImage*	yuvImage;
	IplImage*	ImaskCodeBook;
	codeBook*	cB;
	unsigned	cbBounds[CHANNELS];
	uchar*		pColor;					//YUV pointer
	int			imageLen;
	int			nChannels = CHANNELS;
	int			minMod[CHANNELS];
	int			maxMod[CHANNELS];

	/*初始化变量，从摄像头载入影像*/
	cvNamedWindow( "Raw" );
	cvNamedWindow( "CodeBook" );
	capture = cvCreateFileCapture("person01_walking_d1_uncomp.avi");
	if (!capture)
	{
		printf("Couldn't open the capture!");
		return -1;
	}
	rawImage = cvQueryFrame(capture);										// 从影像中获取每一帧的图像
	yuvImage = cvCreateImage( cvGetSize(rawImage), 8, 3 );					// 给yuvImage 分配一个和rawImage 尺寸相同,8位3通道图像
	ImaskCodeBook = cvCreateImage( cvGetSize(rawImage), IPL_DEPTH_8U, 1 );	// 为ImaskCodeBook 分配一个和rawImage 尺寸相同,8位单通道图像
	cvSet( ImaskCodeBook, cvScalar(255));									// 设置单通道数组所有元素为255,即初始化为白色图像
	imageLen = rawImage->width * rawImage->height;							// 源图像的面积，亦即像素个数
	cB = new codeBook[imageLen];											// 得到与图像像素数目长度一样的一组码本,以便对每个像素进行处理
	for (int i=0; i<imageLen; i++)
		cB[i].numEntries = 0;			// 初始化每个码本的码元数目为0，共imageLen个码本，每一个像素对应一个码本
	for (int i=0; i<nChannels; i++)
	{
		cbBounds[i] = 10;				// 用于确定码元各通道的阀值
		minMod[i]	= 20;				// 用于背景差分函数中
		maxMod[i]	= 20;				// 调整其值以达到最好的分割
	}

	/*开始处理视频每一帧图像*/
	for (int i=0; ; i++)								// 没有跳出循环条件，死循环
	{
		cvCvtColor( rawImage, yuvImage, CV_BGR2YCrCb );	// 色彩空间转换,将rawImage 转换到YUV色彩空间,输出到yuvImage
														// 即使不转换效果依然很好
		//yuvImage = cvCloneImage(rawImage);
		if (i<=30)										// 30帧内进行背景学习
		{
			pColor = (uchar*)(yuvImage->imageData);		// pColor指向指向yuvImage图像首地址
			for (int c=0; c<imageLen; c++)
			{
				cvupdateCodeBook( pColor, cB[c], cbBounds, nChannels );	// 对图像的每个像素,调用此函数,捕捉背景中相关变化图像
																		// 对每一像素pColor，设置对应的码本cB[c]
				pColor += 3;											// 3通道图像, 指向下一个像素的第一通道数据，在函数中对n通道进行处理
			}
			if (i==30)												// 到30帧时调用下面函数,删除码本中陈旧的码元
			{
				for (int c=0; c<imageLen; c++)
					cvclearStaleEntries(cB[c]);						// 遍历所有码本，删除每一个码本中陈旧的码元
			}
		}
		else
		{
			uchar maskPixelCodeBook;								// 30帧过后
			pColor = (uchar*)((yuvImage)->imageData);				// 3 channel yuv image
			uchar* pMask = (uchar*)((ImaskCodeBook)->imageData);	// 1 channel image
																	// pMask指向ImaskCodeBook图像的首地址
			for (int c=0; c<imageLen; c++)
			{
				maskPixelCodeBook = cvbackgroundDiff( pColor, cB[c], nChannels, minMod, maxMod );
																	// 背景处理，对每一个像素判断是否为前景（白色）、背景（黑色）
				*pMask++ = maskPixelCodeBook;						// pMask指针指向的元素，先自加，再赋值
																	// 即将maskPixelCodeBook的值赋给ImaskCodeBook图像（单通道）
				pColor += 3;										// pColor 指向的是3通道图像
			}
		}
		if (!(rawImage = cvQueryFrame(capture)))					// 影像播放完毕，跳出for循环
			break;
		cvconnectedComponents( ImaskCodeBook, 1, 4, NULL, NULL, NULL );
																	// 连通域法消除噪声
		cvShowImage( "Raw", rawImage );								// 循环显示图片，即播放影像
		cvShowImage( "CodeBook", ImaskCodeBook );

		if (cvWaitKey(30) == 27)									// 按ESC键退出
			break;
	}

	/*释放内存，销毁窗口*/
	cvReleaseCapture( &capture );
	if (yuvImage)
		cvReleaseImage( &yuvImage );
	if(ImaskCodeBook)
		cvReleaseImage( &ImaskCodeBook );
	cvDestroyAllWindows();
	delete [] cB;

	return 0;
}
