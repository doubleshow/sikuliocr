#include <iostream>
#include <vector>
#include "cv.h"
#include "highgui.h"
#include "word-finder.h"

#include "TimingBlock.h"

using namespace cv;
using namespace std;
enum{
	ALL,
	LOWER_ONLY,
	UPPER_ONLY
};

bool filter_lineRect(Rect& line, const char word[]);
bool filter_wordRect(WordRect& wordRect, const char word[]);
	

Rect strip_top_bottom(Mat input);

void draw_rectangle(Mat& img, Rect& rect, Scalar color = Scalar(0,0,200)){
	Rect& c = rect;
	rectangle(img, 
			  Point( c.x, c.y), 
			  Point( c.x + c.width, c.y + c.height),
			  color, 0.5, 0, 0 );	
}

void add_margin(Rect& r, int margin, Size bound){;
	
	int x1 = r.x;
	int y1 = r.y;
	int x2 = r.x + r.width;
	int y2 = r.y + r.height;
	
	x1 = max(x1-margin,0);
	y1 = max(y1-margin,0);
	x2 = min(x2+margin,bound.width-margin);
	y2 = min(y2+margin,bound.height-margin);
	
	r.x = x1;
	r.y = y1;
	r.width = x2 - x1 + 1;
	r.height = y2 - y1 + 1;
}


void merge(Rect& r1, Rect& r2){
	int x1 = min(r1.x, r2.x);
	int y1 = min(r1.y, r2.y);
	int x2 = max(r1.x+r1.width-1, r2.x+r2.width-1);
	int y2 = max(r1.y+r1.height-1, r2.y+r2.height-1);	
	r1.x = x1;
	r1.y = y1;
	r1.width = x2 - x1 + 1;
	r1.height = y2 - y1 + 1;
//	return Rect(x1, y1, x2 - x1 + 1, y2 - y1 + 1);
}

void print_rect(Rect& r){
	
	cout << "(" << r.x << "," << r.y << ") " << r.width << "x" << r.height << endl; 
}


void print_matrix(Mat& m){
	typedef uchar T;	
	for( int i = 0; i < m.rows; i +=1 )
    {
		T* ptr = m.ptr<T>(i);		
        cout << i << "\t:";
		for( int j = 0; j < m.cols; j += 1 )
        {			
			cout << (int) ptr[j] << '\t';
		}
		cout << endl;
	}
}


struct tBlock{
	
	tBlock(int _x1, int _y1, int _x2, int _y2) 
	: x1(_x1), y1(_y1), x2(_x2), y2(_y2) {};
	
	int x1;
	int y1;
	int x2;
	int y2;
};

struct tLine : public tBlock {
	
	tLine(int _x1, int _y1, int _x2, int _y2) 
	: tBlock(_x1,_y1,_x2,_y2) {};
	
};

void imshowCompare(const Mat& m1, const Mat& m2, bool pause = true){
	Size canvasSize(max(m1.cols, m2.cols), m1.rows + m2.rows);
	Mat canvas(canvasSize, m1.type());
	
	Mat d1(canvas,Rect(0,0,m1.cols,m1.rows));
	Mat d2(canvas,Rect(0,m1.rows,m2.cols,m2.rows));
	m1.copyTo(d1);
	m2.copyTo(d2);
	
	namedWindow("compare",CV_WINDOW_AUTOSIZE);
	imshow("compare",canvas);
	if (pause)
		waitKey();
}

void imshowCompareHorizontal(const Mat& m1, const Mat& m2, bool pause = true){
	Mat canvas(max(m1.rows, m2.rows),m1.cols + m2.cols, m1.type());
	
	Mat d1(canvas,Rect(0,0,m1.cols,m1.rows));
	Mat d2(canvas,Rect(m1.cols,0,m2.cols,m2.rows));
	m1.copyTo(d1);
	m2.copyTo(d2);
	
	namedWindow("compare",CV_WINDOW_AUTOSIZE);
	imshow("compare",canvas);
	if (pause)
		waitKey();
}


void imshowCompareZoom(const Mat& m1, const Mat& m2, bool pause = true){
	
	Mat large1;
	Mat large2;	
	resize(m1,large1, Size(m1.cols*3,m1.rows*3));
	resize(m2,large2, Size(m2.cols*3,m2.rows*3));
	imshowCompare(large1, large2, pause);
}

void imshowDebug(const char* name, const Mat& m, bool pause = true){
    imshow(name,m);
	if (pause)
		waitKey();	
}

void imshowDebugZoom(const char* name, Mat& m, bool  pause = true){
	
	Mat large;
	resize(m,large, Size(m.cols*3,m.rows*3));
    imshow(name,large);
	if (pause)
		waitKey();	
}


int L2dist(Vec3b p1, Vec3b p2){
	Vec3i p1i(p1);
	Vec3i p2i(p2);
	return norm(p1i - p2i);
}
	
int L2dist(uchar p1, uchar p2){
	return abs(p1 - p2);
}

int L1dist(Vec3b p1, Vec3b p2){
	return max(p1[0],p2[0])-min(p1[0],p2[0])+	
		   max(p1[1],p2[1])-min(p1[1],p2[1])+
		   max(p1[2],p2[2])-min(p1[2],p2[2]);	
}


vector<Rect> segment_image(Mat color){
	TimingBlock tb("segment_image");
	Mat gray;
	cvtColor(color, gray, CV_RGB2GRAY);	
	
#if DISPLAY_SEGMENT_IMAGE
	Mat resultImage(color.size(), color.type());
#endif
	
	typedef uchar T;
	//typedef Vec3b T;
	
	Mat im1 = gray;
	//Mat im2 = im1.clone();	
	
	//Mat im1 = color;
	Mat im2 = Mat::ones(im1.rows,im1.cols,CV_8UC1);
	
	{
		TimingBlock  tb("Scanning");

	Size size = im1.size();
	for( int i = 0; i < size.height; i +=1 )
    {
		//cout << i << endl;
		T* ptr1 = im1.ptr<T>(i);
		uchar* ptr2 = im2.ptr<uchar>(i);	
		
		bool has_previous_baseline = false;
		int previous_baseline_endpoint = 0;
		int current_baseline_startpoint;
		
		current_baseline_startpoint = 0;
		
        for( int j = 1; j < size.width; j += 1 )
        {			
			
			// if a contrast transition is encountered or 
			// at the right-most edge of the image

			//float diff = norm(ptr1[j]-ptr1[j-1]);//1,p2);//L2dist(p1,p2);//(p1 - p2);//tr1[j] - ptr1[j-1]);
			//float diff = L1dist(ptr1[j],ptr1[j-1]);
			float diff = abs(ptr1[j]-ptr1[j-1]);
			if (diff > 50 || j == size.width - 1){
				
				// check for the condition of a baseline hypothesis
				// the length of the baseline must be > 15
				if ((j - current_baseline_startpoint) > 10 || j == size.width - 1){
					
					// if there's a previous baseline hypothesis 
					// and close to the current one, 
					//int closeness_threshold = 12;
					
					//set small for find task because lines usually
					// have plentiy of spacing vertically
					int closeness_threshold = 1; 
					
					if (has_previous_baseline && 
						L2dist(ptr1[current_baseline_startpoint], 
							   ptr1[previous_baseline_endpoint]) < 150 &&
						(current_baseline_startpoint - previous_baseline_endpoint) 
						 <= closeness_threshold){
						
						
						// merge the current baseline with the previously baseline
						for (int k=previous_baseline_endpoint; 
							 k < current_baseline_startpoint; k += 1){
							ptr2[k] = 0;
						}
					}
					
					has_previous_baseline = true;
					previous_baseline_endpoint = j;
					
					for (int k=current_baseline_startpoint; k < j; ++k){
						ptr2[k] = 0;
					}	
					
				}
				
				// forming a new baseline hypothesis
				current_baseline_startpoint = j+1;
			}
        }
    }	
	
	}

//	{
//		TimingBlock tb("athre");
//	Mat im3;
//	adaptiveThreshold(gray,im3,255,ADAPTIVE_THRESH_MEAN_C,THRESH_BINARY_INV,3,1);
//	namedWindow("segment:binary", CV_WINDOW_AUTOSIZE);			
//	imshow("segment:binary",im3);			
//	waitKey();
//	}
	
	//adaptiveThreshold(im1,im2,0,255,CV_THRESH_BINARY);
	
//	imshow("segment:binary",im2);			
//	waitKey();
		
	
	CvMemStorage* storage = cvCreateMemStorage();
	CvSeq* first_contour = NULL;
	
	CvMat mat = (CvMat) im2;	
	{
	TimingBlock  tb("FindContour");

	cvFindContours(
				   &mat,
				   storage,
				   &first_contour,
				   sizeof(CvContour),
				   CV_RETR_EXTERNAL);
	
	}
	CvSeq* c = first_contour;
 	
	vector<Rect> rects;	
	{
		TimingBlock tb("while loop");
	while (c!= NULL){
		
		// find bounding boxes
		int x1=im2.cols;
		int x2=0;
		int y1=im2.rows;
		int y2=0;		
		
		
		
		for( int i=0; i < c->total; ++i ){
			
			
			
			CvPoint* p = CV_GET_SEQ_ELEM( CvPoint, c, i );
			if (p->x > x2)
				x2 = p->x;
			if (p->x < x1)
				x1 = p->x;
			if (p->y > y2)
				y2 = p->y;
			if (p->y < y1)
				y1 = p->y;		 		
		}
		
		// add a 2-pixel margin around each line block
		x1 = max(x1-2,0);
		y1 = max(y1-2,0);
		x2 = min(x2+2,im1.cols-1);
		y2 = min(y2+2,im1.rows-1);
		
		int h = y2-y1+1;
		int w = x2-x1+1;
		
		Rect rect(x1,y1,w,h);
		
		if (h<8 || w<8 || h/w > 5){
			
		}else{
			
#if DISPLAY_SEGMENT_IMAGE
			
			rectangle(im2,
					  Point( x1, y1), 
					  Point( x2, y2),
					  Scalar(200), 0.5, 0, 0 ); 
			
			//			rectangle(color, 
			//					  Point( x1, y1), 
			//					  Point( x2, y2),
			//					  Scalar(0,200,0), 0.5, 0, 0 ); 
			
			Mat src(color,rect);
			Mat dest(resultImage,rect);
			src.copyTo(dest);
			
			//			waitKey();
			
#endif			
			
			rects.push_back(rect);	
			
		}
		
		
		c = c->h_next;
		
	}
	}
#if DISPLAY_SEGMENT_IMAGE
	
	namedWindow("segment:gray", CV_WINDOW_AUTOSIZE);			
	imshow("segment:gray",resultImage);			
	
	namedWindow("segment:color", CV_WINDOW_AUTOSIZE);			
	imshow("segment:color",color);			
	
	waitKey();
#endif		
	
	cvReleaseMemStorage(&storage);
	return rects;
	
}


vector<Rect> segment_lineImage_old(Mat m){
	
	
	vector<Rect> rects;
	
	typedef uchar T;
	
	Mat mt = m.clone().t();
    //imshowDebug("segment_line:mt",mt);
	
	int bg = mt.at<T>(0,0);
	
	int p=0;
	bool spacing=true;
	for( int i = 0; i < mt.rows; i +=1 )
    {
		//cout << i << endl;
		T* ptr1 = mt.ptr<T>(i);
		
		int cnt = 0;
        for( int j = 1; j < mt.cols; j += 1 )
        {			
			
			if (abs(ptr1[j] - bg) < 150 && j){
				cnt++;
			}else{
				cnt = 0;
			}
		}
		
		
		if (!spacing && (cnt >= mt.cols-1 || i == mt.rows - 1)){
			
			spacing = true;
			
			int y1=p;
			int y2=i;
			
			int x1=0;
			int x2=mt.cols-2;
			
			//
			//Rect rect(x1,y1,x2-x1+1,y2-y1+1);
			Rect rect(y1,x1,y2-y1+1,x2-x1+1);
			rects.push_back(rect);
			
			//#if DISPLAY_SEGMENT_LINE
			//			
			//			rectangle(mt, 
			//					  Point( x1, y1), 
			//					  Point( x2, y2),
			//					  Scalar(0,0,200), 1, 0, 0 );
			//			imshowDebug("char",mt);
			//#end			
		}else if (spacing && cnt < mt.cols-1){
			
			spacing = false;
			
			p = i;
		}
		
	}		
	return rects;
}


bool sort_by_x (Rect a, Rect b){ 
	return (a.x < b.x); 
}


void subtract_background(Mat& input){
	/*	
	 uchar bg = im1.at<uchar>(1,1);
	 
	 if (bg>150){
	 threshold(im1,im1,55,255,THRESH_BINARY_INV);
	 }else{
	 threshold(im1,im1,200,255,THRESH_BINARY);		
	 }
	 
	 */	
}

vector<Rect> segment_lineImage(Mat& input){
	
	Mat im1;
	Mat& imrgb = input;
	cvtColor(imrgb, im1, CV_RGB2GRAY);	
	
	uchar bg = im1.at<uchar>(1,1);

	// ToDO: a more reliable background pixel 
//	if (bg>150){
		//threshold(im1,im1,55,255,THRESH_BINARY_INV);
		adaptiveThreshold(im1,im1,255,ADAPTIVE_THRESH_MEAN_C,THRESH_BINARY_INV,3,1);
//	}else{
		//threshold(im1,im1,200,255,THRESH_BINARY);		
//		adaptiveThreshold(im1,im1,255,ADAPTIVE_THRESH_MEAN_C,THRESH_BINARY,3,1);	
//	}
	
	//erode(im1,im1,Mat());	
	//Mat im1copy = im1.clone();
	//imshowDebugZoom("t",im1);
	
	
	CvMemStorage* storage = cvCreateMemStorage();
	CvSeq* first_contour = NULL;
	
	
	//CvMat mat = (CvMat) im1.clone();
	Mat m2 = im1.clone();
	CvMat mat = (CvMat) m2;
	
	cvFindContours(
				   &mat,
				   storage,
				   &first_contour,
				   sizeof(CvContour),
				   CV_RETR_EXTERNAL);
	
	CvSeq* c = first_contour;
 	//cvReleaseMat(&mat);
	
	vector<Rect> rects;	
	while (c!= NULL){
		
		// find bounding boxes
		int x1=im1.cols;
		int x2=0;
		int y1=im1.rows;
		int y2=0;		
		
		
		
		for( int i=0; i < c->total; ++i ){
			
			
			
			CvPoint* p = CV_GET_SEQ_ELEM( CvPoint, c, i );
			if (p->x > x2)
				x2 = p->x;
			if (p->x < x1)
				x1 = p->x;
			if (p->y > y2)
				y2 = p->y;
			if (p->y < y1)
				y1 = p->y;				
		}
		
		int h = y2-y1+1;
		int w = x2-x1+1;
		
		
		Rect rect(x1,y1,w,h);
		
		if (h>4){
			
			
#if DISPLAY_SEGMENT_LINEIMAGE_STEP
			rectangle(imrgb, 
					  Point( rect.x, rect.y), 
					  Point( rect.x + rect.width, rect.y + rect.height),
					  Scalar(0,0,200), 1, 0, 0 ); 			 
			imshowDebugZoom("t",imrgb);
			//waitKey();
#endif
			rects.push_back(rect);	
			
		}
		c = c->h_next;	
	}
	cvReleaseMemStorage(&storage);
	
	sort(rects.begin(), rects.end(), sort_by_x);	
	
#if 0
	if (rects.size()>1){
		// use the spacing between the first two letters as an estimate of the letter spacing
		Rect& r1 = rects[0];
		Rect& r2 = rects[1];
		
		int sp = r2.x - r1.x - r1.width;
		
		// expand each rect by the amount of letter spacing
		for (int i=0; i<rects.size();++i){
			
			Rect& rect = rects[i];
			int x1 = max(rect.x-sp,0);
			int x2 = min(rect.x+rect.width+sp, input.cols);
			
			rect.x = x1; 
			rect.width = x2 - x1 + 1;
		}
	}
#endif
	
	
#if DISPLAY_SEGMENT_LINEIMAGE_RESULT	
	for (int i=0; i<rects.size();++i){		
		Rect& rect = rects[i];
		rectangle(imrgb, 
				  Point( rect.x, rect.y), 
				  Point( rect.x + rect.width, rect.y + rect.height),
				  Scalar(0,0,200), 1, 0, 0 ); 
	}
	imshowDebugZoom("t",imrgb);
#endif
	
	return rects;
}

vector<Rect> extract_characters(Mat& imrgb){
	
	vector<Rect> ret;
	
	Mat im1;
	cvtColor(imrgb, im1, CV_RGB2GRAY);	
	//imshowDebug("1",imrgb);
	
	
	Mat im = imrgb.clone();
	
	vector<Rect> lines = segment_image(imrgb);
	
	cout << "found " << lines.size() << " lines" << endl;
	
	//for (int j=0; j < min(30,(int)lines.size()) ; ++j){
	for (int j=0; j < lines.size() ; ++j){
		
		cout << "segmenting line: " << j << " ...";
		Rect o = lines[j];
		
		cout << o.x << " " << o.y << " " << o.width << " " << o.height;
		
		Mat m(imrgb, o);
		vector<Rect> chs = segment_lineImage(m);
		
		cout << " found " << chs.size() << " blocks" << endl;
		for (int i=0; i < chs.size() ; ++i){
			Rect c = chs[i];		
			
			c.x += o.x;
			//c.y += o.y;
			c.y += o.y;
			
			/*		
			 c.y = o.y;
			 c.height = o.height;
			 
			 Rect r = strip_top_bottom(Mat(im1,c));
			 c.y += r.y;
			 c.height = r.height;
			 
			 */		
			ret.push_back(c);
			
			rectangle(im, 
					  Point( c.x, c.y), 
					  Point( c.x + c.width, c.y + c.height),
					  Scalar(0,0,200), 1, 0, 0 ); 	
		}
		
		//		rectangle(im, 
		//				  Point( o.x, o.y), 
		//				  Point( o.x + o.width, o.y + o.height),
		//				  Scalar(0,200,0), 1, 0, 0 ); 		
		
	}	
	//imshowDebug("1",im);
	
	return ret;
	
}


vector<Mat> letterImages;
char recognize_character(Mat& charImageGray, int mode = 0){
	
	float bestScore=0;
	char bestChar=' ';
	
	for (int j=0; j < letterImages.size() ; ++j){
		//Rect lt = letters[j];
		Mat imlt = letterImages[j];
		
		
		Size ch = charImageGray.size();
		Size lt = imlt.size();
		float ar1 = 1.0*ch.height / ch.width;
		float ar2 = 1.0*lt.height / lt.width;
		
		char c;
		if (mode == LOWER_ONLY && j < 26)
			continue;
		
		if (mode == UPPER_ONLY && j >= 26)
			continue;
		
		if (j<26){
			c = 'A' + j;
		}
		else {
			c = 'a' + j - 26;	
		}
		
		//cout << c << ":" <<  min(ar1,ar2)/max(ar1,ar2) << " : ";
		float aspect_ratio_measure = min(ar1,ar2)/max(ar1,ar2);
		if (aspect_ratio_measure>0.5){
			
			
			Size nsize;
			nsize.width = max(ch.width, lt.width);
			nsize.height = max(ch.height, lt.height);
			
			//Mat imch(imrgb,ch);
			Mat nch;
			Mat nlt;
			resize(charImageGray,nch,nsize);
			resize(imlt,nlt,nsize);
			
			/*
			 Size nsize;			
			 nsize.width = lt.width;
			 nsize.height = ch.height * (1.0*lt.width/ch.width);
			 
			 
			 Mat nch;// = imch;
			 Mat nlt = imlt;
			 resize(imch,nch,nsize);
			 */
			
			Mat result;
			matchTemplate(nch,nlt,result,CV_TM_CCOEFF_NORMED);
			double minValue, maxValue;
			Point minLoc, maxLoc;
			minMaxLoc(result, &minValue, &maxValue, &minLoc, &maxLoc);
			//cout << maxValue << endl;
			
			if (maxValue > bestScore){
				bestScore = maxValue;
				bestChar = c;
			}
		}
	}
	
	cout << bestChar << " " << bestScore << endl;
	
	return bestChar;
	
}


Rect strip_top_bottom(Mat input){
	
	typedef uchar T;
	
	Mat copy = input.clone();
  	
	int bg = input.at<T>(0,0);
	
	if (bg>150){
		threshold(input,copy,bg-135,255,THRESH_BINARY_INV);
	}else{
		threshold(input,copy,bg+135,255,THRESH_BINARY);
	}
	
	//print_matrix(copy);
	
	bool blank;
	int i,j;
	
	blank = true;
	i=0;
	while (blank && i < copy.rows){		
		T* ptr = copy.ptr<T>(i);		
		j=0;
		while (blank && j < copy.cols){
			if (ptr[j]>0)
				blank = false;
			j++;
		}	
		i++;
	}
	
	int y1=i-1;
	
	blank = true;
	i=copy.rows-1;
	while (blank && i >= 0){		
		T* ptr = copy.ptr<T>(i);		
		j=copy.cols-1;
		while (blank && j >= 0){
			if (ptr[j]>0)
				blank = false;
			j--;
		}	
		i--;
	}
	
	int y2=i+1;
	
	//cout << input.rows << ":" << y1 << " " << y2 << endl;
	
	return Rect(0,y1,input.cols,y2-y1+1);	
}

bool isUpper(char ch){
	return ch >= 'A' && ch <= 'Z';
}

void recognize(Mat& image){
	
	
	Mat gray;
	cvtColor(image, gray, CV_RGB2GRAY);	
	
	Mat resultImage = Mat(image.size(), image.type());
	
	//::zeros(image.rows,image.cols,image.type());
	//image.clone();
	
	
	
	
	
	vector<Rect> lines = segment_image(image);
	
	cout << "found " << lines.size() << " lines" << endl;		
	for (int j=0; j < min(20,(int)lines.size()) ; ++j){
		//for (int j=0; j < lines.size() ; ++j){
		
		cout << "segmenting line: " << j << " ...";
		Rect o = lines[j];
		Rect lineRect = lines[j];
		
		cout << o.x << " " << o.y << " " << o.width << " " << o.height;
		
		Mat m(image, o);
		vector<Rect> chs = segment_lineImage(m);
		
		// 1: all lower
		
		/*	
		 int cnt=0;
		 for (int i=0; i < chs.size() ; ++i){
		 Rect c = chs[i];	
		 c.x += o.x;
		 c.y += o.y;
		 if ((c.y + c.height) > (o.y + (0.6)*o.height))
		 cnt++;
		 }
		 
		 // all lower cases	
		 if ((1.0*cnt / chs.size()) > 0.8)
		 mode = 1;
		 */
		uchar lineBgColor = m.at<uchar>(0,0);
		
		
		char prevChar;
		Rect prevCharRect;
		
		cout << " found " << chs.size() << " blocks" << endl;
		for (int i=0; i < chs.size() ; ++i){
			Rect c = chs[i];	
			
			
			c.x += o.x;
			c.y += o.y;
			Rect charRect = c;
			
			Mat charImageGray(gray,c);
			
			if (lineBgColor<50){
				
				//Mat::ones(gray.rows, gray.cols, CV_8U)*255;
				Mat white = Mat::ones(charImageGray.rows, 
									  charImageGray.cols, CV_8U)*255 - charImageGray;
				subtract(white, charImageGray, charImageGray); 
			}	
			
			
			int mode;
			
			if (i==0){
				
				
			}else{
				
				if (abs(prevCharRect.y + prevCharRect.height) - 
					(charRect.y + charRect.height) <= 2){
					
					
				}
				
				
			}
			
			
			
			//imshowDebugZoom("1", charImageGray);
			
			
			//				c.y = o.y;
			//				c.height = o.height;
			
			//				Rect r = strip_top_bottom(Mat(gray,c));
			//				c.y += r.y;
			//				c.height = r.height;
			//				
			
			if ((charRect.y + charRect.height) < (lineRect.y + 0.9*lineRect.height)){
				mode = LOWER_ONLY;
			}else {
				mode = UPPER_ONLY;
			}
			
			
			char bestChar = recognize_character(charImageGray, mode);
			
#if DISPLAY_RECOGNIZE
			
			Scalar textColor;
			
			if (lineBgColor > 50)
				textColor = Scalar(0,0,255);
			else
				textColor = Scalar(0,0,255);
			
			//				rectangle(resultImage, 
			//						  Point( c.x, c.y), 
			//						  Point( c.x + c.width, c.y + c.height),
			//						  Scalar(0,0,200), 1, 0, 0 ); 	
			
			
			char buf[10];
			Point center;
			center.x = c.x;// + ch.width/2;
			center.y = o.y;// + ch.height;
			
			sprintf(buf,"%c",bestChar);
			putText(resultImage,buf,center, FONT_HERSHEY_SIMPLEX, 0.4, textColor);
			
			//		imshowDebug("1",copy);	
			
#endif				
			prevChar = bestChar;
			prevCharRect = charRect;
			
			
		}
		
		//		rectangle(im, 
		//				  Point( o.x, o.y), 
		//				  Point( o.x + o.width, o.y + o.height),
		//				  Scalar(0,200,0), 1, 0, 0 ); 		
		
	}	
	imshowDebug("1",resultImage);
	
	
}


Mat& get_letter_image(char ch){
	
	int i=0;
	if (ch >= 'a' && ch <= 'z')
		i = ch - 'a' + 26;
	else
		i = ch - 'A';
	
	return letterImages[i];
}

Mat& get_char_image(char ch){
	return get_letter_image(ch);
}

Mat generate_word_image(const char word[]){
	
    int n = strlen(word);
	
	
	int x=0;
	int total_width=0;
	int max_height=0;
	int margin=2;
	int spacing=6;
	int offset=3;
	int above=0;
	int below=0;
	for (int i=0; i < n; ++i){		
		char ch = word[i];
		
		if (ch == ' '){
			total_width += margin + spacing;		
		}
		else{
			
			Mat& letterImage = get_letter_image(ch);
			
			total_width += letterImage.cols;
			total_width += margin;	
			
			if (ch == 'p' || ch == 'y' || ch == 'g' || ch == 'q' || ch == 'j'){
				below = offset;
				above = max(above, letterImage.rows - offset);
			}
			else{				
				above = max(above, letterImage.rows);				
			}
				
		}
	}
	total_width -= margin;
	max_height = below + above+1;
	
	
	Mat img = Mat::ones(max_height,total_width,CV_8U)*255;	
	
	for (int i=0; i < n; ++i){		
		char ch = word[i];
		
		if (ch == ' '){
			x += spacing;		
		}
		else{
			
			
			Mat& letterImage = get_letter_image(ch);
			
			if (ch == 'p' || ch == 'y' || ch == 'g' || ch == 'q' || ch == 'j'){
			
			//Mat dest(img, Rect(x,0, letterImage.cols, letterImage.rows));
			Mat dest(img, Rect(x,max_height-letterImage.rows, letterImage.cols, letterImage.rows));
			letterImage.copyTo(dest);
			
			}else{
	
				Mat dest(img, Rect(x,above-letterImage.rows, letterImage.cols, letterImage.rows));
				letterImage.copyTo(dest);
				
			}
			
			x += letterImage.cols;
			x += margin;
		}
	}
	
	//imshowDebugZoom("wordimage", img);
	return img;
}

void find_character(Mat source, char target){
	
	Mat targetImage = letterImages['U'-'A'];
	
	
}






vector<WordRect> characterRects_to_wordRects(const vector<Rect>& characterRects){
	
	vector<WordRect> wordRects;

	Rect charRect = characterRects[0];	
	
	WordRect wordRect(charRect);
	//wordRect.charRects.push_back(charRect);	
	wordRects.push_back(wordRect);
	

	//int spacing_threshold = 3;
	for (int j=0; j < characterRects.size() ; ++j){
		Rect charRect = characterRects[j];		
		WordRect& currentWordRect = wordRects.back();
		
		int spacing = charRect.x - (currentWordRect.x + currentWordRect.width);
		int spacing_threshold = 3 + (charRect.height / 12) ;
		if (spacing <= spacing_threshold || j== 0){
			// merge the current character into the current word
			merge(currentWordRect, charRect);
			currentWordRect.charRects.push_back(charRect);
		}else{
			
			WordRect newWordRect(charRect);			
			newWordRect.charRects.push_back(charRect);	
			wordRects.push_back(newWordRect);
				
		}
	}
	
	return wordRects;
		
}

void test_segment(const Mat& inputImage, const char word[]){
	
	//Mat resultImage = inputImage.clone();
	Mat resultImage1;
	Mat resultImage2;
	
	
	resultImage1 = inputImage.clone();	
	resultImage2 = Mat(inputImage.size(), inputImage.type(), Scalar(0,0,0));
	
	vector<Rect> linesRects = segment_image(inputImage);
	for (int i=0; i < linesRects.size(); ++i){
		
		Rect lineRect = linesRects[i];
		Mat lineImage = Mat(inputImage, lineRect);
		
		vector<Rect> characterRects = segment_lineImage(lineImage);
		if (characterRects.empty())
			continue;
		
		
		for (int j=0; j < characterRects.size() ; ++j){
			Rect characterRect = characterRects[j];		
			characterRect.x += lineRect.x;
			characterRect.y += lineRect.y;
		
			
			//add_margin(characterRect, 1, inputImage.size());
			
			Rect& c = characterRect;
			rectangle(resultImage1, 
					  Point( c.x, c.y), 
					  Point( c.x + c.width, c.y + c.height),
					  Scalar(0,0,200), 0.5, 0, 0 ); 
			
			rectangle(resultImage2, 
					  Point( c.x, c.y), 
					  Point( c.x + c.width, c.y + c.height),
					  Scalar(0,255,0), 0.5, 0, 0 ); 	
			
						
			Mat src(inputImage, characterRect);
			Mat dest(resultImage2, characterRect);
			src.copyTo(dest);			
		}
		
		
		
		Rect& c = lineRect;
		rectangle(resultImage1, 
				  Point( c.x, c.y), 
				  Point( c.x + c.width, c.y + c.height),
				  Scalar(0,200,0), 0.5, 0, 0 ); 	

		rectangle(resultImage2, 
				  Point( c.x, c.y), 
				  Point( c.x + c.width, c.y + c.height),
				  Scalar(255,0,0), 0.5, 0, 0 ); 	
		
		
		vector<WordRect> wordRects = characterRects_to_wordRects(characterRects);
		
		for (int j=0; j < wordRects.size() ; ++j){
			WordRect& wordRect = wordRects[j];
			
			
			if (filter_wordRect(wordRect, word))
				continue;

			
			wordRect.x += lineRect.x;
			wordRect.y += lineRect.y;	
			draw_rectangle(resultImage2, wordRect);
			
			char buf[50];
			sprintf(buf, "%d:%s", wordRect.charRects.size(),word);
			Point loc(wordRect.x, wordRect.y);
			Scalar textColor(255,255,255);
			putText(resultImage2,buf, loc, FONT_HERSHEY_SIMPLEX, 0.4, textColor);
		}
	}
	
			
	
	imshowDebug("resultImage1", resultImage1,false);	
	imshowDebug("resultImage2", resultImage2,false);
	waitKey();
	
	//resultImage2.release();
	
}

bool sort_by_score(Match& m1, Match& m2){
	return m1.score > m2.score;
}


float match_char(Mat& input, Mat& target){

	Mat inputG;
	if (input.type() == CV_8UC3){
		cvtColor(input, inputG, CV_RGB2GRAY);
	}else{
		input = inputG;
	}
	
	Mat targetG = target;
	//resize(target, targetG, inputG.size());
	
	Mat inputGN;
	resize(inputG, inputGN, targetG.size());
	
	
	Mat result;
	matchTemplate(inputGN, targetG, result, CV_TM_CCOEFF_NORMED); 	
	
	double minValue, maxValue;
	Point minLoc, maxLoc;
	minMaxLoc(result, &minValue, &maxValue, &minLoc, &maxLoc);	
	
#if DISPLAY_MATCH_CHAR
	cout << maxValue << endl;
	imshowCompareZoom(inputGN, targetG);
#endif
	
	return maxValue;
}

float match_word(const Mat& inputImage, const WordRect& wordRect, const char targetWord[]){
	float score = 0;

#if DISPLAY_MATCH_WORD	
	cout << wordRect.charRects.size() << endl;		
	Mat m1(inputImage, wordRect);
	Mat m1g;
	cvtColor(m1, m1g, CV_RGB2GRAY);
	Mat m2 = generate_word_image(targetWord);
	imshowCompareZoom(m1g, m2);
#endif	

	
	for (int i=0; i < min(wordRect.charRects.size(), strlen(targetWord)); ++i){

		Rect charRect = wordRect.charRects[i];
		Mat inputCharImage(inputImage, charRect);
		Mat targetCharImage = get_char_image(targetWord[i]);
	
		float score_per_char = match_char(inputCharImage, targetCharImage);
		
		if (score_per_char > MIN_CHAR_MATCH_THRESHOLD){
			score = score + 1 + (0.1*score_per_char);
		}
	}	
	
#if DISPLAY_MATCH_WORD		
	cout << "total score:" << score << endl; 
#endif
	
	return score;	
}

bool filter_lineRect(Rect& line, const char word[]){
	return line.width < strlen(word)*5;
}

bool filter_wordRect(WordRect& wordRect, const char word[]){
	int inputWordLength = wordRect.charRects.size();
	int targetWordLength = strlen(word);

	
	if (inputWordLength > targetWordLength + 2 ){
		return true; // too long
	}	
	
	if (inputWordLength < targetWordLength - 1){
		return true; // too short
	}	
	
	if (wordRect.width < strlen(word) * 5){
		return true; // too narrow
	}	
	
	return false;
}


vector<Match> find_word_by_image(Mat inputImage, const char word[]){
	
	Mat targetWordImage = generate_word_image(word);
	
	
	Rect bestRect;
	float bestScore=0;
	
	vector<Match> candidateMatches;
	
	int total_lines=0;
	int seen_lines=0;
	int total_words=0;
	int total_chars=0;
	int seen_words=0;
	
	vector<Rect> linesRects = segment_image(inputImage);
	total_lines = linesRects.size();
	
	for (int i=0; i < linesRects.size(); ++i){
		
		Rect lineRect = linesRects[i];
		Mat lineImage = Mat(inputImage, lineRect);
		
		
		// line is too short
		if (filter_lineRect(lineRect, word)){
			continue;
		}
		
		// break the line down into characters
		vector<Rect> charRects;
		charRects = segment_lineImage(lineImage);
	
		// skip if too few characters
		if (charRects.size() < strlen(word) - 2){
			continue;
		}
				
		for (int j=0; j < charRects.size(); ++j){
			Rect& charRect = charRects[j];
			charRect.x += lineRect.x;
			charRect.y += lineRect.y;
		}
		total_chars += charRects.size();
		
		
		seen_lines++;
		vector<WordRect> wordRects = characterRects_to_wordRects(charRects);
		total_words += wordRects.size();
		
		for (int j=0; j < wordRects.size(); ++j){
			WordRect wordRect = wordRects[j];
			
			if (filter_wordRect(wordRect, word))
				continue;

			seen_words++;
			
			float score = 0;
			
#if MATCH_BY_ENTIRE_IMAGE			
			// Match by whole image
			Mat inputWordImage = Mat(inputImage, wordRect);
			
			Mat inputWordImageGray;
			cvtColor(inputWordImage, inputWordImageGray, CV_RGB2GRAY);	
		

			Mat targetWordImageN;
			resize(targetWordImage, targetWordImageN, inputWordImageGray.size());

			Mat result;
			matchTemplate(inputWordImageGray, targetWordImageN, result, CV_TM_CCOEFF_NORMED); 
			
			double minValue, maxValue;
			Point minLoc, maxLoc;
			minMaxLoc(result, &minValue, &maxValue, &minLoc, &maxLoc);
			score = maxValue;
			//cout << maxValue << endl;
			
#else			
			
			score = match_word(inputImage, wordRect, word);
#endif			
			Match m;
			m.score = score;
			m.rect = wordRect;
			candidateMatches.push_back(m);
			
		}
		
		//}
	
	}
	//

	cout << "Lines: " << total_lines << endl;	
	cout << "Seen lines: " << seen_lines << endl;
	cout << "Chars: " << total_chars << endl;
	cout << "Words: " << total_words << endl;
	cout << "Seen Words: " << seen_words << endl;

	
	
	sort(candidateMatches, sort_by_score);									   
	return candidateMatches;								   
}




WordFinder::WordFinder(){};

void
WordFinder::train(Mat& trainingImage){
	
	Mat trainingImageGray;
	cvtColor(trainingImage, trainingImageGray, CV_RGB2GRAY);	
	
	vector<Rect> letters = extract_characters(trainingImage);
	for (int i=0; i < letters.size(); ++i)
		letterImages.push_back(Mat(trainingImageGray, letters[i]));
}

vector<Match>
WordFinder::find(const Mat& inputImage, const char* word){
	TimingBlock tb("WordFinder::find");
	vector<Match> ms = find_word_by_image(inputImage, word);
	return ms;
}

void paste(const Mat& src, Mat& dest, int x, int y){
	
	int x1 = x;
	int y1 = y;
	int x2 = min(x+src.cols-1, dest.cols-1);
	int y2 = min(y+src.rows-1, dest.rows-1);
	int h = y2-y1+1;
	int w = x2-x1+1;
	Mat destRectImage(dest, Rect(x1,y1,w,h));
	Mat srcRectImage(src, Rect(0,0,w,h));
	
	srcRectImage.copyTo(destRectImage);
}

void
WordFinder::test_find(const Mat& inputImage, const vector<string>& testWords){

	Mat resultImage = inputImage.clone();
	
	
  for (int i=0; i < testWords.size(); ++i){
	  string testWord(testWords[i]);

	  test_segment(inputImage,testWord.c_str());
	  	
	  vector<Match> ms = find(inputImage, testWord.c_str());
	  
	
	  // draw each match on the result image for visualization
	  for (int j=0; j < min(DISPLAY_NUM_TOP_MATCHES, (int) ms.size()); ++j){
	  
		  Rect r = ms[j].rect;
		  
		  draw_rectangle(resultImage, r);
		  
		  char buf[50];
		  sprintf(buf, "%d:%s:%0.2f", j+1, testWord.c_str(), ms[j].score);
 
		  int baseline = 0;
		  Size textSize = getTextSize(buf, 
									  FONT_HERSHEY_SIMPLEX,
									  0.5, 1, &baseline);

		  Point loc(r.x,r.y+25);
 		  rectangle(resultImage, 
					loc+Point(0,baseline), 
					loc+Point(textSize.width, -textSize.height),
					Scalar(0,0,0), CV_FILLED);
		  	  
		  Scalar textColor(255,255,255);
		  putText(resultImage,buf, loc, FONT_HERSHEY_SIMPLEX, 0.5, textColor);
		  //imshowDebug("test_find:resultImage",resultImage);
 
	  }
  }
	
	imshowDebug("test_find:resultImage",resultImage);
	
}