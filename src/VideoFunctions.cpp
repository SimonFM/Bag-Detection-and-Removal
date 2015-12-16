#include "Headers/VideoFunctions.h"
#include "Headers/Defines.h"
#include "Headers/Utilities.h"
#include "opencv2/core.hpp"
#include "opencv2/highgui.hpp"
#include <iostream>
#include "MedianBackground.h"
 
// load the images in from a files
void loadVideosFromFile(char * fileLocation, char ** imageFiles, int size, VideoCapture * &videos){
    std::cout << "Loading some Videos..." << std::endl;
    for(int i = 0; i < size; i++){
        string filename(fileLocation);
        filename.append(imageFiles[i]);
        VideoCapture temp(filename);
        videos[i] = temp;
        if (!videos[i].isOpened())
            std::cout << "!!! Failed to open file: " << filename << std::endl;
        else std::cout << "Loaded: " << filename << std::endl;
    }
 
}
 
void applyMask(Mat toProcess, Mat mask, Mat &result){
    toProcess.copyTo(result,mask);
}
 
//  Gray the current frame
void grayFrame(Mat currentFrame, Mat &result){
    cvtColor(currentFrame, result, CV_BGR2GRAY);
    //imshow("Gray ", gray);
}
 
// Apply Edge Detection
void cannyFrame(Mat toBeGrayed, Mat &result){
    Mat Cannyed;
    Canny(toBeGrayed, result, 50, 150, 3);
    //imshow("Canny",Cannyed);
}

// this tutorial helped me understand how to implement the background
// models in OpenCV:
// http://docs.opencv.org/master/d1/dc5/tutorial_background_subtraction.html#gsc.tab=0
void getMaskFromBackGroundFrame(Mat currentFrame,Ptr<BackgroundSubtractorMOG2> MOG2, Mat &result){
    RNG rng(12345);
    Mat foreGround, canny, drawing;
    vector<vector<Point> > contours;
    vector<Vec4i> hierarchy;
    MOG2 -> apply(currentFrame, foreGround,0);
    morphologyEx(foreGround, result, MORPH_ERODE, Mat(), Point(-1,-1),NUMBER_OF_ERODES);
    morphologyEx(result, result, MORPH_DILATE, Mat(), Point(-1,-1),NUMBER_OF_DILATIONS);
}
 
void applyContours(Mat input, Mat original, Mat &output){
    RNG rng(12345);
    Mat foreGround, canny;
    vector<vector<Point> > contours;
    vector<Vec4i> hierarchy;
	cvtColor(input, foreGround, CV_BGR2GRAY);
	//threshold(foreGround, foreGround, 30, 255,  CV_THRESH_BINARY );
	imshow("foreGround1",foreGround);
	morphologyEx(foreGround, foreGround, MORPH_ERODE, Mat(), Point(-1,-1),NUMBER_OF_ERODES);
	//erode();
	imshow("foreGround2",foreGround);
    findContours( foreGround, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );

	vector<RotatedRect> minRect( contours.size() );
	vector<RotatedRect> minEllipse( contours.size() );
      // Draw contours
    Mat drawing = Mat::zeros( original.size(), CV_8UC3 );
    for( int i = 0; i< contours.size(); i++ ){

		minRect[i] = minAreaRect( Mat(contours[i]) );
        if( contours[i].size() > 5 ) minEllipse[i] = fitEllipse( Mat(contours[i]) ); 

       Scalar color = Scalar( rng.uniform(0, 255), rng.uniform(0,255), rng.uniform(0,255) );
       drawContours( original, contours, i, color, 2, 8, hierarchy, 0, Point() );

	   ellipse( original, minEllipse[i], color, 2, 8 );
       // rotated rectangle
       Point2f rect_points[4]; minRect[i].points( rect_points );
       for( int j = 0; j < 4; j++ )
          line( original, rect_points[j], rect_points[(j+1)%4], color, 1, 8 );
    }
 // output = input;
}

Mat getMedianDifferenceMedianModels(MedianBackground &background1,MedianBackground &background2,Mat &frameToDisplay){
	Mat diff1, diff2, diff3;
	Mat median_foreground_image_1, median_foreground_image_2;
	background1.UpdateBackground( frameToDisplay );
	Mat background1Image = background1.GetBackgroundImage();

	Mat median_difference;
	absdiff(background1Image, frameToDisplay, diff1);
	cvtColor(diff1, diff1, CV_BGR2GRAY);
	threshold(diff1,diff1,30,255,THRESH_BINARY);
	median_foreground_image_1.setTo(Scalar(0,0,0));
	frameToDisplay.copyTo(median_foreground_image_1, diff1);

	background2.UpdateBackground( frameToDisplay );
	Mat background2Image2 = background2.GetBackgroundImage();

	absdiff(background2Image2, frameToDisplay, diff2);
	cvtColor(diff2, diff2, CV_BGR2GRAY);
	threshold(diff2,diff2,30,255,THRESH_BINARY);
	median_foreground_image_2.setTo(Scalar(0,0,0));
	frameToDisplay.copyTo(median_foreground_image_2, diff2);

	absdiff(median_foreground_image_1, median_foreground_image_2, diff3);
	//absdiff(diff1, diff2, diff3);


	imshow("diff3",diff3);
	//waitKey(0);
	return diff3;
}


// Process the Videos
void processVideos(int size, VideoCapture * &videos){
    std::cout << "Processing the Frames..." << std::endl;
    Mat frameToDisplay, current, diff3;
	Mat contouredImage, blurredImage;
	Mat output;
	VideoCapture currentVideo;
    Ptr<BackgroundSubtractorMOG2> MOG2 = createBackgroundSubtractorMOG2();
	int frameNum = 0;
    for(int i = 0; i < 1 ; i++){
		//videos[i].set(CV_CAP_PROP_POS_FRAMES, 0);
		videos[i] >> current;
		currentVideo = videos[i];
		MedianBackground background1(current, (float) 1.005, 1 );
		MedianBackground background2(current, (float) 1.05, 1 );
		
		frameNum = 0;
		frameToDisplay = current;
		int currentFrame = videos[i].get(CV_CAP_PROP_POS_FRAMES);
		while(!frameToDisplay.empty()){ // Process the image's frame
			if(frameNum % FRAME == 0){
				
				int currentFrame = videos[i].get(CV_CAP_PROP_POS_FRAMES);
				std::cout <<"Processing Frame: " << currentFrame << std::endl;
				blur(frameToDisplay,blurredImage, cv::Size(3,3));
				diff3 = getMedianDifferenceMedianModels(background1,background2,blurredImage);
				imshow("diff3",diff3);
				applyContours(diff3,frameToDisplay,contouredImage);
				
				//imshow("diff between 2",contouredImage);
				imshow("Original",frameToDisplay);
				//show("median_foreground_image2",median_foreground_image);
				waitKey(SPEED);
				// DO NOT DELETE, THIS IS THE OLD WAY
				//getMaskFromBackGroundFrame(frameToDisplay,MOG2,mask);
				//grayFrame(frameToDisplay,gray);
				//applyMask(frameToDisplay,mask,resultOfMask);
				//applyContours(mask,contours);
				////cannyFrame(gray);
				//Mat display1 = JoinImagesHorizontally(frameToDisplay,"Original", contours , "Contours");
				//imshow("Bag ",display1);
				//waitKey(SPEED);
				//std::cout << " Frame : " << frame << std::endl;
				
			}
			frameNum++;
		    videos[i] >> frameToDisplay;
		}   

	}
}