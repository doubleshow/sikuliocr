#include <iostream>
#include <vector>
#include "cv.h"
#include "highgui.h"

#include "word-finder.h"

using namespace cv;
using namespace std;


string inputImageName;
vector<string> testWords;

void testcase_network(){
//	inputImageName = "network_small.png";		
//	inputImageName = "network_medium.png";	
	inputImageName = "network_large.png";	

	testWords.push_back("Apply");
	testWords.push_back("Connect");
	testWords.push_back("Assist");
	testWords.push_back("Revert");
	testWords.push_back("Show");
	testWords.push_back("Advanced");	
	testWords.push_back("UMIACS");

}	

void testcase_trash(){
	inputImageName = "trash_large.png";	
	
	testWords.push_back("Cancel");	
	testWords.push_back("Empty");	
	testWords.push_back("Trash");
	testWords.push_back("View");	
	testWords.push_back("Compose");	
	testWords.push_back("Calendar");		
	testWords.push_back("Applications");		
	testWords.push_back("Dropbox");		
}

void testcase_keyboard(){
	inputImageName = "keyboard.png";	
	
	testWords.push_back("Keyboard");	
	testWords.push_back("Restore");		
	testWords.push_back("Spotlight");	
	testWords.push_back("Services");		
	testWords.push_back("Bluetooth");	
	testWords.push_back("Shortcuts");		
	testWords.push_back("Dashboard");
	testWords.push_back("double");
	testWords.push_back("Application");
	testWords.push_back("Replacing");
	testWords.push_back("Batteries");

}


void testcase_access(){
	inputImageName = "access.png";	
	
	testWords.push_back("Enhance");	
	testWords.push_back("Contrast");		
	testWords.push_back("VoiceOver");	
	testWords.push_back("Display");	
	testWords.push_back("Zoom");	
	testWords.push_back("Options");	
	
}


void testcase_gmail(){
	inputImageName = "gmail.png";	
	
	testWords.push_back("Compose");
	testWords.push_back("Archive");
	testWords.push_back("Delete");
	testWords.push_back("All");
	testWords.push_back("None");
	testWords.push_back("Read");
	testWords.push_back("Unread");
	testWords.push_back("Calendar");
	testWords.push_back("Document");

}

void testcase_gmail_zoom(){
	inputImageName = "gmail_zoom.png";	
	//inputImageName = "drafts.png";	
	
	testWords.push_back("Compose");
	testWords.push_back("Inbox");
	testWords.push_back("Buzz");
	testWords.push_back("Starred");	
	testWords.push_back("Drafts");
	testWords.push_back("Sent");
	testWords.push_back("Mail");
	
}

int main (int argc, char * const argv[]) {

	Mat trainingImage = imread("fonts.png",1);
		
	//char inputImageName[] = "sikulibwg.png";
	//char inputImageName[] = "sikuli_black_background.png";
	//char inputImageName[] = "ui.png";
	
	testcase_network();
	//testcase_trash();
	//testcase_keyboard();
	//testcase_access();
	//testcase_gmail_zoom();

	
	Mat inputImage    = imread(inputImageName,1);
	
	WordFinder wf;
	wf.train(trainingImage);
	wf.test_find(inputImage,testWords);
	
    return 0;
}
