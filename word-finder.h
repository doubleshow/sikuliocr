#ifndef _WORD_FINDER_H_
#define _WORD_FINDER_H_

#include "cv.h"
using namespace cv;


struct Match{
	Rect rect;
	float score;
};


struct WordRect : public Rect{	
	WordRect(Rect& r) : Rect(r) {};
	vector<Rect> charRects;
};

class WordFinder{
	
public:
	WordFinder();
	void train(Mat& trainingImage);
	vector<Match> find(const Mat& inputImage, const char* word);
	
	
	void test_find(const Mat& inputImage, const vector<string>& testWords);
};



#define DISPLAY_SEGMENT_IMAGE 0
#define DISPLAY_SEGMENT_LINEIMAGE_STEP 0
#define DISPLAY_SEGMENT_LINEIMAGE_RESULT 0 
#define DISPLAY_RECOGNIZE 0
#define DISPLAY_WORD_FIND_RESULT 1
#define DISPLAY_TEST_SEGMENT 0

#define DISPLAY_MATCH_WORD 0
#define DISPLAY_MATCH_CHAR 0

#define DISPLAY_FIND_WORD_STEP 0

#define DISPLAY_NUM_TOP_MATCHES 1

#define MIN_CHAR_MATCH_THRESHOLD 0.4


#endif