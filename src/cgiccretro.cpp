

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <stdio.h>
#include <iostream>
#include <sys/time.h>
#include <unistd.h>
#include <sstream>
#include <stdlib.h>
#include <inttypes.h>
#include <glob.h>
#include <vector>
#include <fcntl.h>
#include <linux/kd.h>
#include <sys/ioctl.h>
#include <boost/algorithm/string.hpp> // include Boost, a C++ library
#include <sys/stat.h>
#include <sys/types.h>
#include <utime.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>


#include <fstream>
#include <iomanip>
#include <unistd.h>
#include <cstdio>
#include <errno.h>
#include <linux/videodev2.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <vector>
#include <stdio.h>



//openCV
#include <opencv2/opencv.hpp>
#include <opencv/cv.hpp>
#include <opencv2/videoio.hpp>
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/core/core.hpp"


#include <iostream>
#include <stdio.h>
#include <stdlib.h>


#include "cgicc/Cgicc.h"
#include "cgicc/HTTPHTMLHeader.h"
#include "cgicc/HTMLClasses.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>

using namespace std;
using namespace cgicc;
using namespace cv;
using boost::property_tree::ptree;
using boost::property_tree::read_json;
using boost::property_tree::write_json;


// DEFINES

#define MAX_JITTER 15000000
#define DEATHMASK_CUTOFF 100
#define MISSINGWORM_THRESHOLD 10

const Scalar SCALAR_BLACK = Scalar(0.0,0.0,0.0);
const Scalar SCALAR_WHITE = Scalar(255.0,255.0,255.0);
const Scalar SCALAR_BLUE = Scalar(255.0,0.0,0.0);
const Scalar SCALAR_GREEN = Scalar(0.0,255.0,0.0);
const Scalar SCALAR_RED = Scalar(0.0,0.0,255.0);

//Globals



int numworms=0; ///total number of worms
int expID=0;    //the experiment ID
int daysold=0;  //the worms' age in days for the start of the observations on wormbot
int lowthresh=0; //hold the lower threshold limit for the edge finding algo
int highthresh=0; //hold the upper threshold limit for the edge finding algo


vector<long> frametimes;  //the unix epoch of the frame array in terms of seconds of worm age...offset by daysold from description.txt


//Functions

long getFileCreationTime(string filename){
	struct stat attr;
	stat(filename.c_str(),&attr);
	long time = attr.st_mtim.tv_sec;
	//cout << "setframetime:" << time << endl;
	return (time);

}//end getFileCreationTime

void getExperimentTimes(string path, int totalframes){
	stringstream ss;
	ss << path << "description.txt" ;
	ifstream ifile(ss.str().c_str());//load the description.txt file to get the experiment parameters
	string inputline;
	long expstarttime=0;
	int daysold=0;

	int i=0;
	while (getline(ifile,inputline)){
		if (i==5){
			expstarttime=atol(inputline.c_str());
		}//end process experiment start time
		if (i==11){
            daysold = atoi(inputline.c_str());  //store the
            cout << "daysold:" << daysold << endl;
            cout << "expstarttime:" << expstarttime << endl;
		}//end process starting age
		i++;
	}//end while lines in the description txt

	for (int j=0; j < totalframes; j++){
			stringstream oss;
			stringstream ss;

			stringstream number;
			number << setfill('0') << setw(6) << j;

			oss << path << "aligned" << number.str() <<".png";

		    ss << path << "frame" << number.str() <<".png";

		    long filetime = getFileCreationTime(ss.str());

		    frametimes.push_back(filetime - (expstarttime - (86400 * daysold)));

	}//end for each frame




}//end getexperimenttimes

long getFrameAge(string frameFileName, long daysold, long expstarttime){
	long filetime = getFileCreationTime(frameFileName);

	return (filetime - (expstarttime - (86400 * daysold)));

}//end getFrameAge



class WormRegion {
public:

	//rectangle ROI variables
	int x; //x coord
	int y; //y coord
	int w;  //width
	int h;	//height
	int n; //the worm's number (:name from JS)

	vector <Mat> frames;  //store the raw image data from the ROI
	vector <Mat> contourframes; //store the contour drawing from the ROI
	vector <long> ageOnFrame; //stores the worm's age at each frame
	vector <int> movements;  //store the per frame movement (area(contour) - deathmask)
	vector <int> mass;		//store the per frame optical mass (area(contour))
	vector <int> frameIndexes; ///basically the original frame number, index from the stack build

	int currframe;
	int endframe;
	int minutesold;
	int daysold;
	long secondsold;




	void getLifespan(long secs){
		secondsold=secs;
		minutesold = secondsold / 60;
		daysold = secondsold /86400;
	}//getLifespan

	void setDeathFrame(int frameofdeath){
		currframe= frameofdeath;
	}//end setDeathFrame


	string printWormList(void){
		int midx,midy;
		//assign worm x,y to center of rectangle ROI
		midx = x + (w/2);
		midy = y + (h/2);

		stringstream ss;

		ss << midx << "," << midy << "," << currframe << "," << n << "," << daysold << "," << minutesold << endl;

		return ss.str();

	}


	string exportLifespan(void){
		int c=0;
		int i=frameIndexes.back();
		int averageMass=0;
		long total=0;
		int goneCounter=0;
		//end goThrough the contourmass in reverse until contour disappears for MISSINGWORM_THRESHOLD  frames
		for(vector<int>::reverse_iterator citer = mass.rbegin(); citer != mass.rend(); citer++) {

			if (c < 100) {  ///get the average mass of the last 100 frames
				total+=(*citer);
			} else if (c==100){
				averageMass = total/100;
			}//end if is 100

			if (c > 100 && (*citer) < averageMass/2) goneCounter++;

			if (goneCounter > MISSINGWORM_THRESHOLD){
				getLifespan(ageOnFrame[i]);
				setDeathFrame(frameIndexes[i]);
				stringstream ss;
				ss << n << "," << daysold << "," << minutesold << "," << secondsold << endl;
				return ss.str();

			}
			c++;
			i--;


		}//end for each mass in reverse

		stringstream ss;
		ss << n << "," << daysold << "," << minutesold << "," << secondsold << endl;
		return ss.str();

	}//end



	void buildStack(Mat& thisImg, long aof, int idx){
		if (idx > endframe) return; //if past time of death ignore frame
		Rect rec(x,y,w,h);
		frames.push_back(thisImg(rec).clone());
		ageOnFrame.push_back(aof);
		frameIndexes.push_back(idx);
	}//end buildStack


	long getDiff(Mat img1, Mat img2,string path, bool doStore){
		long imageSum=0;
		Mat diffImage;
		diffImage = img1 - img2; //absdiff
		if (doStore)imwrite(path,diffImage);
		imageSum = countNonZero(diffImage);
		return imageSum;
	}//end getdiffs

	long getDiff(int checkframe){
		long imageSum=0;
		Mat diffImage;
		absdiff(frames[checkframe], frames[checkframe+1], diffImage);



		return countNonZero(diffImage);

	}//end get diff


	void dumpStack(bool storeFiles){
		//function dumps the ROI stack to disk and builds the
		int filenum=0;

		//create the subdirectory
		stringstream mkdircommand;
		stringstream path;
		path << "/disk1/robot_data/" << expID << "/worm_num" << n ;
		mkdircommand << "mkdir " << path.str() << endl;
		system(mkdircommand.str().c_str());

		for (vector<Mat>::iterator citer = frames.begin(); citer != frames.end(); citer++){
		   			stringstream filename;
		   			filename << path.str() << "/framenum" << filenum <<".png";
					if (storeFiles) imwrite(filename.str().c_str(), (*citer));

					filenum++;
		   		 }//end for each rectangle







					stringstream contmasspath;
					contmasspath << "/disk1/robot_data/" << expID << "/worm_num" << n << "/contourmass";
					ofstream cmfile(contmasspath.str().c_str());


					//setup the deathmask for movement subtraction
					Mat deathmask = (Mat::zeros( frames[0].size(), CV_8UC1 ));

					int i=0;
					for (vector<Mat>::iterator citer = frames.begin(); citer != frames.end(); citer++){
						Mat canny_output;
						vector<vector<Point> > contours;
						vector<Vec4i> hierarchy;
						blur( (*citer), (*citer), Size(3,3) );
						Canny( (*citer), canny_output, lowthresh, highthresh, 3 );
						findContours( canny_output, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE, Point(0, 0) );
						Mat drawing = (Mat::zeros( canny_output.size(), CV_8UC1 ));

						for( size_t j = 0; j< contours.size(); j++ )
							 {
							  double length=arcLength(contours[j], true);
							  Rect boundRect=boundingRect(contours[j]);

							   Scalar color = SCALAR_WHITE;
							   drawContours( drawing, contours, (int)j, color, 2, 8, hierarchy, 0, Point() );


							 }
						stringstream filename;
					 	filename << path.str() << "/contour" << i <<".png";

					 	//store the contour mass
					 	cmfile << "frame num, " << i << "," <<  (long) countNonZero(drawing) << endl;
					 	mass.push_back((long) countNonZero(drawing));

					 	if (storeFiles) imwrite(filename.str().c_str(), drawing);
					 	contourframes.push_back(drawing.clone());
					 	i++;

					}//end for each frame


					//go through contour stack in reverse and build the deathmask



					 i=0;
					 int c=endframe;

					for(vector<Mat>::reverse_iterator citer = contourframes.rbegin(); citer != contourframes.rend(); citer++) {


							//build contour death jitter mask
							if (i < DEATHMASK_CUTOFF){
								deathmask = deathmask + (*citer);
							}//end if in dead phase
							else if (i == DEATHMASK_CUTOFF){
								stringstream maskname;
								maskname << path.str() << "/deathmask.png";
								if (storeFiles) imwrite(maskname.str().c_str(),deathmask);
							}//end
							i++;


					}




					cmfile.close();

					stringstream contpath;
					contpath << "/disk1/robot_data/" << expID << "/worm_num" << n << "/diffsum";
					ofstream cfile(contpath.str().c_str());

					//calculate the interframe diffs on contours


					for (int i=0; i < (filenum); i++){
						stringstream filename; //empty filename
						filename << path.str() << "/diffs" << i <<".png";
						cout << "start:" << i ;
						long mov=0;
						mov = getDiff(contourframes[i],deathmask, filename.str(), storeFiles);
						movements.push_back(mov);
						cfile << "frame number: " << i << "," << mov << endl;
						cout << "end" << endl;
					}
					cfile.close();






	}//end dump stack

	WormRegion(int sx,int sy, int sw, int sh, int frame, int rectnum){

		//extract standard rect coordinate
		if (sw <0 || sh < 0){
			if (sw < 0){
				x=sx+sw;
				w = -sw;
				y=sy;
				h=sh;
			}//width less than zero
			if (sh < 0){
				y=sy+sh;
				h=-sh;
			}//height less than zero
		} else {
			x=sx;
			y=sy;
			h=sh;
			w=sw;

		}//else not less than zero
		endframe = frame;
		currframe = frame;
		n=rectnum;
		daysold=-1;
		minutesold=-1;
		secondsold=-1;




	}//end WormRegion constructor


};//end class wormregion


int getNumFrames(string directory){

	//returns the number of aligned frames in a directory
	glob_t aligned_result;
	int numaligned=0;

	stringstream globpattern;

	globpattern  << directory.c_str() << string("aligned*.png");
	glob(globpattern.str().c_str(),GLOB_TILDE,NULL,&aligned_result);

	for (unsigned int i=0; i < aligned_result.gl_pathc; i++){
			numaligned++;
		}//end for each glob

return numaligned;

}//end readDirectory


//Globals

vector <WormRegion> worms;  //vector to hold the worm objects




//export wormlist.csv
void ExportWormlist(string filename){
	ofstream wormfile(filename.c_str());

	for (vector<WormRegion>::iterator citer = worms.begin(); citer != worms.end(); citer++){

		if ((*citer).daysold >0){
			wormfile << (*citer).printWormList();

		}//if a deathtime was found

	}//end for each worm region
	wormfile.close();
}//end exportwormlist






int
main(int argc,
     char **argv)
{




	ofstream serverlock("/disk1/robot_data/serverlock.lock");
	serverlock.close();



	  std::streambuf *psbuf, *backup;
	  std::ofstream filestr;
	  filestr.open ("/var/www/robot_data/cgiechooutput");

	  backup = std::cerr.rdbuf();     // back up cout's streambuf

	  psbuf = filestr.rdbuf();        // get file's streambuf
	  std::cerr.rdbuf(psbuf);         // assign streambuf to cout
	  std::cout.rdbuf(psbuf);





	string boostfilename("/var/www/robot_data/boostoutput");
	ofstream boostfile(boostfilename.c_str());
	boostfile << "boostopen" << endl;

	 int max_frame=0;


   try {
      Cgicc cgi;



      string foo;


      foo = cgi("name");
      expID = atoi(string(cgi("expID")).c_str());
      lowthresh = atoi(string(cgi("lowthresh")).c_str());
      highthresh = atoi(string(cgi("highthresh")).c_str());
      cout << lowthresh << "lowthresh\n";
      cout << highthresh << "highthresh\n";


      stringstream readcgi;
      readcgi << foo;

      ptree pt;
      read_json (readcgi, pt);

      write_json(boostfile,pt);


      boostfile << "ExpID" << expID << endl;
      BOOST_FOREACH(const ptree::value_type &v, pt.get_child("")) {

    	  boostfile << "x:" <<  v.second.get<int>("x") << ",";
    	  boostfile << "y:" <<  v.second.get<int>("y") << ",";
    	  boostfile << "w:" <<  v.second.get<int>("w") << ",";
    	  boostfile << "h:" <<  v.second.get<int>("h") << ",";
    	  boostfile << "f:" <<  v.second.get<int>("f") << endl;

    	  //end find the maximum frame number
          boostfile << "4" << endl;
    	  if (v.second.get<int>("f") > max_frame) max_frame= v.second.get<int>("f");
    	  WormRegion thisworm(v.second.get<int>("x"),v.second.get<int>("y"),v.second.get<int>("w"),v.second.get<int>("h"),v.second.get<int>("f"),v.second.get<int>("name"));
    	  worms.push_back(thisworm);
      }//end boostforeach






   }
   catch(exception& e) {
      // handle any errors
	   cout << "exception caught from cgicc!!!" << endl;
	   filestr.close();
	   remove("/disk1/robot_data/serverlock.lock");
	   boostfile.close();
	   return(0);

   }
   boostfile.close();







   //read in the path to the experiment
   	stringstream experimentpath;
   	vector<string> filelist;


   	experimentpath  << "/disk1/robot_data/" << expID << "/";




   	//get total number of frames
   	//returns the number of aligned frames in a directory
   	//build filelist

   		glob_t aligned_result;
   		int numaligned=0;

   		long expstarttime=0;
		int daysold=0;
   		//extract daysold and experiment start from the description.txt file
   		stringstream ss;
		ss << experimentpath.str() << "description.txt";
		ifstream ifile(ss.str().c_str());//load the description.txt file to get the experiment parameters
		string inputline;


		int q=0;
		while (getline(ifile,inputline)){
			if (q==5){
				expstarttime=atol(inputline.c_str());
			}//end process experiment start time
			if (q==11){
				daysold = atoi(inputline.c_str());  //store the
				cout << "daysold:" << daysold << endl;
				cout << "expstarttime:" << expstarttime << endl;
			}//end process starting age
			q++;
		}//end while lines in the description txt






   		stringstream globpattern;

   		globpattern  << experimentpath.str().c_str() << string("aligned*.png");
   		glob(globpattern.str().c_str(),GLOB_TILDE,NULL,&aligned_result);

   		for (unsigned int i=0; i < aligned_result.gl_pathc; i++){
   			filelist.push_back(string(aligned_result.gl_pathv[i]));
   			frametimes.push_back(getFrameAge(string(aligned_result.gl_pathv[i]),daysold,expstarttime));
   				numaligned++;
   			}//end for each glob

   		cout << numaligned-- << endl;


   Mat imgFrame1;
   Mat imgFrame2;
   Mat imgFrame3;

   imgFrame1=imread(filelist[0]);
   imgFrame2=imread(filelist[1]);
   imgFrame3=imread(filelist[2]);








   for (int i=0; i < numaligned-2; i++){

   	bool bumped=false;


   		if (i < 3){  //first time in loop
   			imgFrame1=imread(filelist[i]);
   			imgFrame2=imread(filelist[i+1]);
   			imgFrame3=imread(filelist[i+2]);
   		} else {
   			imgFrame1 = imgFrame2.clone();
   			imgFrame2 = imgFrame3.clone();
   			imgFrame3 = imread(filelist[i+2]);
   		}//end if not first time through loop

   		Mat diffImage;
   		absdiff(imgFrame1, imgFrame2, diffImage);

   		long imageSum=0;

   		MatIterator_<uchar> it, end;
   		            for( it = diffImage.begin<uchar>(), end = diffImage.end<uchar>(); it != end; ++it)
   		                imageSum+=*it;
   		            string diffquant = boost::lexical_cast<std::string>(imageSum);
   		            //looks like at 1080p the cutoff for jitter should be around 15,000,000

   		            if (imageSum > MAX_JITTER) {
   		            	imgFrame1 = imgFrame2.clone();
   		            	bumped=true;

   		            }//end if exceeds maxJitter


   		 for (vector<WormRegion>::iterator citer = worms.begin(); citer != worms.end(); citer++){
   			 (*citer).buildStack(imgFrame1,frametimes[i],i);
   		 }//end for each rectangle






   }//end for each image


//timefile.close();
ss.str("");
ss << experimentpath.str() << "lifespanFile";
cout << "exppath " << ss.str() << endl;
ofstream lifespanFile(ss.str().c_str());



   int wormcounter=0;

   for (vector<WormRegion>::iterator citer = worms.begin(); citer != worms.end(); citer++){


      			 (*citer).dumpStack(true);
      			 lifespanFile << (*citer).exportLifespan();
   }//end for each rectangle



   vector<int> lifespan;

   for(int i=0; i < 200; i++) {
	   lifespan.push_back(0);


	   for (vector<WormRegion>::iterator citer = worms.begin(); citer != worms.end(); citer++){
		   if((*citer).daysold == i) lifespan[i]++;
	   }//end each worm
	   lifespanFile << i << "\t" << lifespan[i] << endl;
   }//end for each day


   lifespanFile.close();

   ss.str("");
   ss << experimentpath.str() << "wormlist.csv";
   cout << "WLpath " << ss.str() << endl;
   ExportWormlist(ss.str());


   filestr.close();
   remove("/disk1/robot_data/serverlock.lock");





}//end main
