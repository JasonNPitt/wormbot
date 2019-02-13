//============================================================================
// Name        : alignerd.cpp
// Author      : Jason N Pitt
// Version     :
// Copyright   : MIT LICENSE
// Description : aligns image files in a folder
//============================================================================

#include <iostream>
#include <sys/time.h>
#include <stdlib.h>
#include <glob.h>
#include <sys/stat.h>
#include <utime.h>
#include <fstream>
#include <cstdio>
#include <unistd.h>

#include <boost/algorithm/string.hpp>

#include <opencv/cv.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/imgcodecs.hpp>

#define DELAY_TIME 5

const int MAX_FEATURES = 500;
const float GOOD_MATCH_PERCENT = 0.15f;


using namespace std;

using namespace cv;
using namespace boost;



string directory("");
stringstream root_dir("/var/www/html/wormbot/");

string currfilename;
string fn = root_dir.str() + "alignerd.log";
ofstream logfile(fn.c_str(), ofstream::app);
//streambuf *coutbuf = std::cout.rdbuf(); //save old buf



void readDirectory(string fulldirectory);


class Timer {
public:
	long systemstarttime,seconds,delay;
	double msdelay;
	struct timeval start;
	bool ms;


	Timer(long sseconds){

		gettimeofday(&start, NULL);
		delay = sseconds;
		ms=0;
		msdelay=0;
	}//end consntructor

	Timer(void){
		gettimeofday(&start, NULL);
		delay = 0;
		ms=0;
		msdelay=0;

	}

	Timer(double msec,bool mms){
		gettimeofday(&start,NULL);
		msdelay=msec;
		ms=1;


	}

	void startTimer(long time){

		gettimeofday(&start, NULL);
		if (ms){
			msdelay=time;
		}else{
			delay=time;
		}
		//cout << "stmstimer" << delay << "." << msdelay << "\n";
	}//end start timer
	void startTimer(double time){

			gettimeofday(&start, NULL);
			if (ms){
				msdelay=time;
			}else{
				delay=time;
			}
			//cout << "stmstimer" << msdelay << "\n";
		}//end start timer
	double getTimeElapsed(void){
		struct timeval currtime;
		gettimeofday(&currtime, NULL);
		stringstream ss;
		ss << currtime.tv_sec;

		string seconds(ss.str());
		//cout << "currsec" << seconds << "\t";
		ss.str("");
		ss.clear();
		ss << (double)currtime.tv_usec/(double)1000000;
		string micros(ss.str());

		micros.erase(0,1);
		//cout << "currmicros" << micros << "\t";
		seconds.append(micros);
		//cout << "appended" << seconds << "\t";
		double curr = atof(seconds.c_str());
		//cout << "currtime" << curr << "\t";
		ss.str("");
		ss.clear();

			ss << start.tv_sec;

				string sseconds(ss.str());
				ss.str("");
				ss.clear();
				ss << (double)start.tv_usec/(double)1000000;
				string smicros(ss.str());
				smicros.erase(0,1);
				sseconds.append(smicros);
				double scurr = atof(sseconds.c_str());
				//cout << "starttime" << scurr << "\n";
		return curr-scurr;

	}

	int getSeconds(void){
		struct timeval currtime;
				gettimeofday(&currtime,NULL);

		return (int)(currtime.tv_sec-start.tv_sec);
	}

	void printTimer(void){
		struct timeval currtime;
		gettimeofday(&currtime,NULL);
		cout << (currtime.tv_sec-start.tv_sec);
	}

	bool checkTimer(void){
		struct timeval currtime;
		gettimeofday(&currtime, NULL);
		if(ms){
			//cout << getTimeElapsed() << "\n";
			if (getTimeElapsed() >= msdelay)return true; else return false;
			//cout << "check" << (currtime.tv_sec-start.tv_sec) << "." << (currtime.tv_usec-start.tv_usec) << "\n";
			//if ((currtime.tv_sec-start.tv_sec) >= delay && (currtime.tv_usec-start.tv_usec) >= msdelay) return true; else return false;
		}else{
			if (currtime.tv_sec-start.tv_sec >= delay) return true; else return false;
		}
	}//end checkTimer

};



string swapTimes(string filename, string newfilename){
	stringstream outtime;

	struct stat attr;
	struct utimbuf thetimev;
	stat(filename.c_str(),&attr);

	//cout << "setframetime:" << time << endl;
	thetimev.actime=attr.st_mtim.tv_sec;
	thetimev.modtime=attr.st_mtim.tv_sec;
	//TIMESPEC_TO_TIMEVAL(&thetimev, &attr.st_mtim);
	outtime << ctime(&thetimev.modtime);

	utime(newfilename.c_str(), &thetimev);

	return (outtime.str());
}


long getFileCreationTime(string filename){
	struct stat attr;
	stat(filename.c_str(),&attr);
	long time = attr.st_mtim.tv_sec;
	cout << "setframetime:" << time << endl;
	return (time);

}


void alignDirectory(string dirname, vector<string> filelist, int numframes, int numaligned){
	vector<int> compression_params;
	 compression_params.push_back(CV_IMWRITE_PNG_COMPRESSION);//(CV_IMWRITE_PXM_BINARY);
	 compression_params.push_back(0);


     //save image0
	 cout << "\n Directory: " << dirname <<endl;
	 if (numaligned ==0){
		 try{
		 Mat frame=imread(filelist[0]);
		 Mat frame_gray;

		//convert the first frame to gray and write it out to disk as aligned

		 cvtColor(frame, frame_gray, CV_BGR2GRAY); //make it gray
		 replace_all(filelist[0],"frame","aligned");
		 imwrite(filelist[0], frame_gray, compression_params );
		 numaligned=1;
		 cout <<"Set frame 0." <<endl;
		 } catch (cv::Exception ex){
			cout << "first frame was bad! try copying a later frame to frame 0...I don't feel like writing the code to deal with this rare possibility" << endl;
		 }//end caught exception trying to load

	 }//end if numaligned is zero


	for (int i=numaligned; i < numframes; i++){

		




					cout << "." << endl; //keep webserver from timing out
					Mat frame, frame_gray, im2_aligned, im1_gray,im1, h;

				try{
				 		frame=imread(filelist[i]);

				  		cout << "read filelist ";
						cvtColor(frame, frame_gray, CV_BGR2GRAY); //make it gray

						im1= imread(filelist[0]);//filelist[i-1]);//try warping all to original

						cvtColor(im1, im1_gray, CV_BGR2GRAY);
						cout << "convert color ";
				} catch (cv::Exception ex){
						//frame is bad...overwrite it with the previous frame
		    				 stringstream outss;
		    				 cout << "caught exception:" << ex.code << endl;
		    				 outss << "cp " << filelist[i-1];
		    				 replace_all(filelist[i],"frame","aligned");
		    				 outss << " " << filelist[i] << endl;
		    				 system(outss.str().c_str());
		    				// imwrite(filelist[i], frame_gray, compression_params );
		    				 continue;

				}//end frame was bad
				
/*
//use keypoints

				// Convert images to grayscale
				 
				  
				   
				  // Variables to store keypoints and descriptors
				  std::vector<KeyPoint> keypoints1, keypoints2;
				  Mat descriptors1, descriptors2;
				  cout << "1"; 
				  // Detect ORB features and compute descriptors.
				  Ptr<Feature2D> orb = ORB::create(MAX_FEATURES);
				  orb->detectAndCompute(frame_gray, Mat(), keypoints1, descriptors1);
				  orb->detectAndCompute(im1_gray, Mat(), keypoints2, descriptors2);
				   cout << "2";
				  // Match features.
				  std::vector<DMatch> matches;
				  Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create("BruteForce-Hamming");
				  matcher->match(descriptors1, descriptors2, matches, Mat());
				   cout << "3";
				  // Sort matches by score
				  std::sort(matches.begin(), matches.end());
				   cout << "4";
				  // Remove not so good matches
				  const int numGoodMatches = matches.size() * GOOD_MATCH_PERCENT;
				  matches.erase(matches.begin()+numGoodMatches, matches.end());
				   			   
				   cout << "5";
				  // Extract location of good matches
				  std::vector<Point2f> points1, points2;
				   
				  for( size_t i = 0; i < matches.size(); i++ )
				  {
				    points1.push_back( keypoints1[ matches[i].queryIdx ].pt );
				    points2.push_back( keypoints2[ matches[i].trainIdx ].pt );
				  }
				  cout << "6";
				  // Find homography
				  
					 h = estimateRigidTransform(points1, points2,false);
				   cout << "7";
					  // Use homography to warp image
					 try { warpAffine(frame, im2_aligned, h, im1.size(), INTER_LINEAR + WARP_INVERSE_MAP);} catch (cv::Exception ex) {im2_aligned = frame.clone();}
cout << "8";
					
					
				

				//end use keypoints
				//use ECC


	*/				


					// Define the motion model
					const int warp_mode = MOTION_TRANSLATION;

					// Set a 2x3 warp matrix
					Mat warp_matrix;
	    			warp_matrix = Mat::eye(2, 3, CV_32F);
	    			 // Specify the number of iterations.
	    			int number_of_iterations = 50;

	    			 // Specify the threshold of the increment
	    			 // in the correlation coefficient between two iterations
	    			 double termination_eps = 1e-8;
	    			 cout << "set warp matrix ";

	    			 // Define termination criteria
	    			 TermCriteria criteria (TermCriteria::COUNT+TermCriteria::EPS, number_of_iterations, termination_eps);

	    			 // Run the ECC algorithm. The results are stored in warp_matrix.
	    			 try{
	    				 findTransformECC(im1_gray,frame_gray,warp_matrix,warp_mode,criteria);
	    			 }
	    			 catch(cv::Exception e){
	    				 //frame is bad...overwrite it with the previous frame
	    				 stringstream outss;
	    				 cout << "caught exception:" << e.code << endl;
	    				 outss << "cp " << filelist[i-1];
	    				 replace_all(filelist[i],"frame","aligned");
	    				 outss << " " << filelist[i] << endl;
	    				 system(outss.str().c_str());
	    				// imwrite(filelist[i], frame_gray, compression_params );
	    				 continue;
	    			 }
	    			 cout << "ECC ";
	    			 warpAffine(frame, im2_aligned, warp_matrix, im1.size(), INTER_LINEAR + WARP_INVERSE_MAP);
	    			 cout << "warp ";
	    			

				//end warp ECC







				 Mat im2_aligned_gray;
	    			 cvtColor(im2_aligned, im2_aligned_gray, CV_BGR2GRAY);
	    			 cout << "convert color 2";
	    			 string oldname = filelist[i];
	    			 replace_all(filelist[i],"frame","aligned");
	    			 cout << "writing: "<<filelist[i] << endl;
				
				//Write timestamps with openCV
				
	    				stringstream textadd;
	    				struct utimbuf atime;
	    				atime.modtime = getFileCreationTime(oldname);
	    				atime.actime = atime.modtime;

	    				stringstream filetime;


	    				filetime << ctime(&atime.modtime);
	    				string formattedtime(filetime.str().substr(0,filetime.str().size()-1)); //get the ctime string
	    				string fileframenumber;
	    				replace_all(formattedtime,":",".");  //strip out : for FFMPEG compat
					int spot = filelist[i].find("aligned");  //find index of the framenumber
	    				fileframenumber = filelist[i].substr(spot+7);
	    				replace_all(fileframenumber,".png",""); //remove ".png"
					textadd << fileframenumber << " " << formattedtime << " " << dirname; //put framenumber and timestamp into text
					Point lowerleft(10,im2_aligned_gray.size().height-10);

					putText(im2_aligned_gray, textadd.str(), lowerleft,FONT_HERSHEY_COMPLEX_SMALL, 0.8,cvScalar(255, 255, 255), 1, CV_AA);



	    			 imwrite(filelist[i], im2_aligned_gray, compression_params );
	    			 cout << "writefile  " << endl;
	    			 cout << "timestamp" <<  swapTimes(oldname,filelist[i]) << endl;

	    			 
	    			
	}//end for each frame
}//end alignDirectory



void scanRobotDir(string robotdir){
	stringstream ss;

	//read in the current expID
	ss << robotdir << "currexpid.dat";
	string curridfilename = ss.str();
	ifstream ifile(curridfilename.c_str());
	string currstring;
	getline(ifile,currstring);
	long totalexperiments = atol(currstring.c_str());

	for (int i=0; i <= totalexperiments; i++){
		ss.str("");
		ss << robotdir << i << "/";
		cout << "reading expID:" << ss.str() << endl;
		readDirectory(ss.str());

	}//end for each ID in the directory


}//end scanrobotdir

void readDirectory(string fulldirectory){
	glob_t glob_result;
	glob_t aligned_result;
	vector<string> filelist;
	vector<string> alignedlist;
	long numframes =0;
	long numaligned=0;

	stringstream globpattern;
	globpattern  << fulldirectory.c_str() << string("frame*.png");
     //cout << "directory:" << directory << "<br>" << endl;
	cout <<"globpat:" << globpattern.str() << endl;

	glob(globpattern.str().c_str(),GLOB_TILDE,NULL,&glob_result);
	
	for (unsigned int i=0; i < glob_result.gl_pathc; i++){
		filelist.push_back(string(glob_result.gl_pathv[i]));
		numframes++;
	}//end for each glob

	//scan for aligned files
	globpattern.str("");
	globpattern  << fulldirectory.c_str() << string("aligned*.png");
	glob(globpattern.str().c_str(),GLOB_TILDE,NULL,&aligned_result);

	for (unsigned int i=0; i < aligned_result.gl_pathc; i++){
            filelist[i]=aligned_result.gl_pathv[i];
			numaligned++;
		}//end for each glob





     if (numaligned != numframes){
    	 alignDirectory(fulldirectory,filelist,numframes,numaligned);
     }


}//end readDirectory



int main(int argc, char **argv) {
	ifstream t("var/root_dir");
	root_dir << t.rdbuf();

	bool stopdaemon = true;
	int setExp=-1;

	// setup a timer and get the robot data directory from the commandline
	Timer mytimer;
	string standardfile;
	cout << "argc=" << argc << endl;
	if (argc < 1) {
		standardfile = "/wormbot/";
	} else if (argc ==2) {
		standardfile = argv[1];
		
	} else if (argc >=3){
		standardfile = argv[1];
		string flags(argv[2]);
		if (flags.find("-v") != string::npos){
			 stopdaemon=false;
			 cout << "-verbose mode" << endl;
		} 
		if (flags.find("-e") != string::npos) {
			cout << "targetted directory mode"<< endl;
			setExp = atoi(argv[3]);
			stringstream targetDir;
			targetDir << standardfile << setExp << "/" ;
			readDirectory(targetDir.str());
			exit(EXIT_SUCCESS);
			return 0;
		}//end if "-e"
	}

	if (stopdaemon){
		cout.rdbuf(logfile.rdbuf()); //redirect std::cout to out.txt!
		daemon(0,1);
	}//end if daemon

	//while (true) { //removed endless loop to force master_control to make alignerd restart to deal with OPENCV memory leak
		//mytimer.startTimer((long)DELAY_TIME);
	    scanRobotDir(standardfile);
	   // while(!mytimer.checkTimer());  ///wait
	//}

    exit(EXIT_SUCCESS);

	return 0;
}
