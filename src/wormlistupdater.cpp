//============================================================================
// Name        : wormlistupdater.cpp

// Author      : jason pitt
// Version     : 1.0
// Copyright   : june 2018
// Description : This application take a wormlist built/modified in the retrograde
// javascript app and finds the unix timestamps and saves the modfied wormlist.csv
//============================================================================

//stdlib
#include <iostream>
#include <iomanip>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

//cgicc
#include "cgicc/Cgicc.h"
#include "cgicc/HTTPHTMLHeader.h"
#include "cgicc/HTMLClasses.h"

//boost
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>

//openCV
#include <opencv2/opencv.hpp>
#include <opencv/cv.hpp>
#include <opencv2/videoio.hpp>
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/core/core.hpp"



//namespace
using namespace std;
using namespace cgicc;
using namespace cv;

using boost::property_tree::ptree;
using boost::property_tree::read_json;
using boost::property_tree::write_json;


const Scalar SCALAR_BLACK = Scalar(0.0,0.0,0.0);
const Scalar SCALAR_WHITE = Scalar(255.0,255.0,255.0);
const Scalar SCALAR_BLUE = Scalar(255.0,0.0,0.0);
const Scalar SCALAR_GREEN = Scalar(0.0,255.0,0.0);
const Scalar SCALAR_RED = Scalar(0.0,0.0,255.0);


//Globals
int expID;
string datapath; //hold the path to all the robot data
//read in path from /usr/lib/cgi-bin/data_path
ifstream pathfile("/usr/lib/cgi-bin/data_path");


ofstream debugger;
int numworms=0;



// C L A S S E S

class Experiment { //adapted from wormbot.cpp Well class
public:
		string email;
		string title;
		string investigator;
		string description;
		struct timeval starttime;
		long currentframe;
		int active;
		int status;
		string directory;
		int startingN;	   //number of worms put onto plate
		int startingAge;   //in days;
		int busy;
		int finished;
		long thisexpID;
		int xval;
		int yval;
		int plate;
		int rank;
		bool transActive;
		bool gfpActive;
		bool timelapseActive;
		bool dailymonitorActive;
		string wellname;
		string strain;

		 Experiment(int expnum){







		}//end constructor


		void printDescriptionFile(void){
				string filename;
				filename = directory + string("description.txt");
				//cout<<filename << endl;
				ofstream ofile(filename.c_str());

						ofile << "****************************************************************\n";
						ofile << title << endl;
						ofile << email<< endl;
						ofile << investigator<< endl;
						ofile << description<< endl;
						ofile << starttime.tv_sec<< endl;
						ofile << currentframe<< endl;
						ofile << strain << endl;
						ofile << active<< endl;
						ofile << directory<< endl;
						ofile << startingN<< endl;	   //number of worms put onto plate
						ofile << startingAge<< endl;   //in days;
						ofile << "expID:" << expID<< endl;
						ofile << xval<< endl;
						ofile << yval<< endl;
						ofile << plate<< endl;
						ofile << wellname<< endl;
						ofile << "****************************************************************\n";
						ofile << "::br::" << endl;


						ofile.close();



			}//end print description file

};

class Worm {
public:
	int x;
	int y;
	int currf;
	int number;
	float daysold;
	float minutesold;
	long secondsold;

	Worm(int sx, int sy, int curr, int wormnumber,long secs){

		x=sx;
		y=sy;
		currf=curr;
		number=wormnumber;
		daysold=0;
		minutesold=0;
		secondsold=secs;
		minutesold = ((float)secondsold) / 60.0f;
		daysold = ((float)secondsold) /86400.0f;


	}//end constructor

	Worm(string fileline){
        stringstream ss(fileline);
        string token;
        //cout << "<br>debug:" << fileline<<endl;
        getline(ss,token,',');
        x=atoi(token.c_str());
        getline(ss,token,',');
        y=atoi(token.c_str());
        getline(ss,token,',');
        currf=atoi(token.c_str());
        getline(ss,token,',');
        number=atoi(token.c_str());
        getline(ss,token,',');
        daysold=atof(token.c_str());
        getline(ss,token,',');
        minutesold=atof(token.c_str());
        //cout << "<br>debug:" << drawDiv();

		}//end constructor

	string printData(void){

		stringstream ss;
		ss << setprecision(5);
		ss << x << "," << y << "," << currf << "," << number << "," << daysold << "," << minutesold << endl;
		return (ss.str());


	}//end print data

	string drawDiv(int now){
		stringstream oss;
	      if (now >= currf) oss << ".w" << number <<" { position:absolute; opacity:0.75; font-size:20px; font-family:Impact, Charcoal, sans-serif; color:white; text-shadow: 2px 2px 4px #000000; background:transparent; left:" << (x-25)  << "px; top:" << (y-25) << "px; z-index:2;}" << endl;


		return ( oss.str());

	}//end drawDiv

	string drawIcon(int now){
		stringstream oss;
		 if (now >= currf){
			 oss << "<img src=\"/dead.png\" class=\"w" << number << "\">";
			 oss << "<div class=\"w" << number << "\" >" << number << "</div>" << endl;
		 }//end if should be visible
        return ( oss.str());

	}//end drawIcon

	string drawFormField(int now){
	     stringstream oss;
	     if (now >= currf) oss << "<input type=\"checkbox\" name=\"w" << number <<"\"> Worm "<< number <<": days old:" << daysold << "<br>" << endl;
	     return (oss.str());
	}//end draw formField

};

class Day{
public:
	int numdead;
	int numalive;

	Day(void){
		numdead=0;
		numalive=0;
	}
};

class Lifespan{
public:
	vector <Day> days;
	float maxlifespan;
	int n;
	double mean;
	double median;
	vector <float> formedian; //death events


	Lifespan(vector<Worm>  myworms){
		//getmaxlifespan
		maxlifespan=0;
		n=0;
		double total=0;
		

		for (int i=0; i < myworms.size(); i++){
			if (myworms[i].daysold > maxlifespan) maxlifespan = myworms[i].daysold;
			formedian.push_back(myworms[i].daysold);
			n++;
		}//end for each worm

		for (int i=0; i < myworms.size(); i++){
					total += myworms[i].daysold;
		}//end for each worm
		if (n>0) mean = total/((double)n); else mean=0;

		if (n >0){
			sort (formedian.begin(), formedian.end());
			if (n%2==1){
				median = formedian[n/2];
			}else { //end if odd
				median = (formedian[n/2] + formedian[(n+1)/2])/2;

			}//else even
		} else median=0;






		for (int i=0; i <= (int)maxlifespan; i++){
		     Day today = Day();

		     for(int j=0; j < myworms.size(); j++){
		    	 if (i==(int)myworms[j].daysold) {
		    		 today.numdead++;
		    	 }//if died this day
		    	 if (i < (int)myworms[j].daysold){
		    		 today.numalive++;
		    	 }//end if not dead yet

		     }//end for each worm
		     days.push_back(today);

		}//end for each day

	}//end constructor

	string getOasisListHTML(void){
		stringstream oss;
		for(int i =0; i <= maxlifespan; i++){
			oss << i << "," << days[i].numdead << endl;

		}//end for each day
		return (oss.str());
	}//end get oasis list

	string getOasisListFloat(void){
		stringstream oss;
		for(int i =0; i < formedian.size(); i++){
			oss << formedian[i] << "," << 1 << endl;

		}//end for each day
		return (oss.str());
	}//end get oasis list



};


vector <Worm> wormlist;








// F U N C T I O N S




//make an updated current_contour.png
void Update_Contours(string filename, int lowthresh, int highthresh){


							Mat canny_output;
							Mat inputImg;
							inputImg = imread(filename.c_str());

							if (lowthresh < 0) lowthresh =0;
							if (highthresh < 0) highthresh =0;
							if (lowthresh > 255) lowthresh =255;
							if (highthresh > 255) highthresh =255;

							vector<vector<Point> > contours;
							vector<Vec4i> hierarchy;
							blur( inputImg, inputImg, Size(3,3) );
							Canny( inputImg, canny_output, lowthresh, highthresh, 3 );
							findContours( canny_output, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE, Point(0, 0) );
							Mat drawing = (Mat::zeros( canny_output.size(), CV_8UC1 ));

							for( size_t j = 0; j< contours.size(); j++ )
								 {
								  double length=arcLength(contours[j], true);
								  Rect boundRect=boundingRect(contours[j]);

								   Scalar color = SCALAR_WHITE;
								   drawContours( drawing, contours, (int)j, color, 2, 8, hierarchy, 0, Point() );


								 }
							stringstream outfilename;
						 	outfilename << datapath << expID << "/current_contrast.png";

						 	imwrite(outfilename.str().c_str(), drawing);


}//end update contours





//gets the lifespan based on description.txt and the frame time
long getLifespan(string filename, long frametime){
	ifstream ifile(filename.c_str());
	string inputline;
	long expstarttime=0;
	int daysold=0;

	int i=0;
	while (getline(ifile,inputline)){
		if (i==5){
			expstarttime=atol(inputline.c_str());
		}//end process experiment start time
		if (i==11){
            daysold = atoi(inputline.c_str());
            cout << "daysold," << daysold << endl;
            cout << "expstarttime," << expstarttime << endl;
            cout << "frametime," << frametime << endl;
             return (frametime-(expstarttime -(86400 * daysold)));

		}//end process starting age
		i++;
	}


}//end getexperimentstarttime


long getFileCreationTime(string filename){
	struct stat attr;
	stat(filename.c_str(),&attr);
	long time = attr.st_mtim.tv_sec;

	return (time);

}//end getFileCreationTime


float getAgeinDays(int framenum){

	stringstream oss;
	string fulldirectory;
	long frametime;
	oss << datapath << expID << "/";

		fulldirectory=oss.str();
		stringstream number;
		stringstream ss;
		number << setfill('0') << setw(6) << framenum;
		ss << fulldirectory << "frame" << number.str() <<".png";
		frametime = getFileCreationTime(ss.str());
		ss.str("");
		ss << fulldirectory << "description.txt";
	return(((float)(getLifespan(ss.str(),frametime)))/86400.0f);
}//end getageinDays

int getAgeinMinutes(int framenum){

	stringstream oss;
	string fulldirectory;
	long frametime;
	oss << datapath << expID << "/";

		fulldirectory=oss.str();
		stringstream number;
		stringstream ss;
		number << setfill('0') << setw(6) << framenum;
		ss << fulldirectory << "frame" << number.str() <<".png";
		//debugger << ss.str() << ",";
		frametime = getFileCreationTime(ss.str());
		//debugger << "frametime: " << frametime;
		ss.str("");
		ss << fulldirectory << "description.txt";
	return(((float)(getLifespan(ss.str(),frametime)))/60);
}//end getageinDays


void loadWorms(string filename){
     ifstream ifile(filename.c_str());
     string inputline;
     numworms=0;

     while(getline(ifile,inputline)){
    	 Worm myworm(inputline);
    	 wormlist.push_back(myworm);
    	 numworms++;
     }//end while inputlines
}//end load worms


string buildMovie(string filename, int startframe, int endframe){
	stringstream oss;
	stringstream ffmpeg;
	stringstream lastcomp;


	ffmpeg << "./ffmpeg -y -f image2 -start_number " << startframe
		<<" -i "<< filename << "aligned%06d.png ";

	if (wormlist.size() != 0) {

		for (int i=0; i < wormlist.size(); i++){
			ffmpeg << " -i /var/www/html/wormbot/img/dead.png "; //load the worm circle for each dead worm
		}
		//load the base timelapse movie
		ffmpeg << " -filter_complex \" [0:v] setpts=PTS-STARTPTS [base]; ";

		int counter=1;
		for (int i=0; i < wormlist.size(); i++){
			ffmpeg << " [" << counter+i << ":v] setpts=PTS-STARTPTS [dead" << counter+i << "]; ";
		}

		lastcomp << "[base]";

		for (int i=0; i < wormlist.size(); i++){

			int start =wormlist[i].currf-startframe;
			int end = endframe - startframe;

			ffmpeg << lastcomp.str() << "[dead" << counter +i << "] overlay="
			   << wormlist[i].x -25 << ":" << wormlist[i].y -25<< ":";
			ffmpeg << "enable='between(n," << start << "," << end << ")' ";

			lastcomp.str(""); //erase the last composite title

			if (i + 1 < wormlist.size()) {
				lastcomp << "[tmp" << i <<"] ";
				ffmpeg << lastcomp.str() << ";";
			}

		}

		ffmpeg << " \"";
	}

	ffmpeg << " -q:v 1 -vframes " << (endframe+1)-startframe << " " << filename << expID <<".avi 2>&1 | tee /var/www/robot_data/ffmpegstdout.txt" << endl;

	system(ffmpeg.str().c_str());

	string fn = filename + "/tempffmpegcomand";
	ofstream ofile(fn.c_str());
	ofile << ffmpeg.str() <<endl;
	ofile.close();

	oss << "worked";

	return (oss.str());
}//end buildmovie

string printWormLifespan(string title){
	stringstream oss;
	if (wormlist.empty()) return oss.str();



	Lifespan ls(wormlist);

	oss << "N," << ls.n <<  endl;
		oss << "Mean, " << ls.mean <<  endl;
		oss << "Median," << ls.median << endl;
		oss << "Max, " << ls.maxlifespan  << endl <<endl;

		oss << "%" <<  title << endl;
	oss << ls.getOasisListHTML();
	oss << "\n\n\n";
	oss << "%" <<  title << endl;
	oss << ls.getOasisListFloat();




	return (oss.str());
}//end printwormlifespan






//////////// M A I N
int main(int argc,char **argv){

	cout << setprecision(5);

	getline(pathfile,datapath);
	pathfile.close();

	string updatepath = datapath + "updatedebug";
	debugger.open(updatepath.c_str());

	stringstream wormfilename;
	 Cgicc cgi;
	 int moviestart,moviestop=0;
	 int highthresh,lowthresh =0;
	 int currframe;

	try {


	      string foo = cgi("deadworms");
	      expID = atoi(string(cgi("expID")).c_str());
	      moviestart = atoi(string(cgi("startmovie")).c_str());
	      moviestop = atoi(string(cgi("stopmovie")).c_str());
	      highthresh = atoi(string(cgi("highthresh")).c_str());
    	  lowthresh = atoi(string(cgi("lowthresh")).c_str());
    	  currframe = atoi(string(cgi("currframe")).c_str());



	      wormfilename << datapath << expID << "/wormlist.csv";
	      ofstream wormfile(wormfilename.str().c_str());
	      stringstream readcgi;
		  readcgi << foo;

		  ptree pt;
		  read_json (readcgi, pt);

		  string tboost(datapath + "updateboost");
		  ofstream testboost(tboost.c_str());
		  testboost << "movstart:" << moviestart << ",moviestop:" << moviestop << endl;
		  write_json(testboost,pt);

		  testboost.close();



		  BOOST_FOREACH(const ptree::value_type &v, pt.get_child("")) {

		      	  wormfile  <<  v.second.get<int>("x") << ",";
		      	  wormfile <<  v.second.get<int>("y") << ",";
		      	  wormfile  <<  v.second.get<int>("deathframe") << ",";
		      	  wormfile  <<  v.second.get<int>("number") << ",";
		      	  if (v.second.get<float>("daysold") < 0 || v.second.get<float>("minutesold") < 0){
		      	  //get the age of the deathframe
		      		  wormfile <<   getAgeinDays(v.second.get<int>("deathframe")) << ",";
		      		  wormfile <<   getAgeinMinutes(v.second.get<int>("deathframe")) << endl;
		      	  }else {
		      		wormfile << v.second.get<float>("daysold") << "," ;
		      		wormfile << v.second.get<float>("minutesold") << endl;

		      	  }//end else already found time of death


		   }//end boostforeach

		  wormfile.close();



	} catch (exception& e){

	}//end exception caught


	//build the wormlist
	stringstream wormpath;
	wormpath << datapath << expID << "/";
	loadWorms(wormfilename.str());


	if (cgi.queryCheckbox("buildMovie")){

		buildMovie(wormpath.str(),moviestart,moviestop);

	}//end if want to build a new movie

	if (cgi.queryCheckbox("checkboxUpdateContours")){
		stringstream currimgfilename;
		stringstream number;
		number << setfill('0') << setw(6) << currframe;
		currimgfilename << datapath << expID << "/aligned" << number.str() << ".png";
		Update_Contours(currimgfilename.str(),lowthresh, highthresh);

	}//end if update contours



	//Dump data to lifespanoutput_expID.csv


	//dump description into lifespan output
	stringstream filename,ofilename;
	filename << datapath << expID << "/description.txt";
	ofilename << datapath << expID << "/lifespanoutput_" << expID << ".csv";
	ifstream ifile(filename.str().c_str());
	ofstream ofile(ofilename.str().c_str());
	string aline;
	int c;
	string dtitle;
	while (getline(ifile,aline)){
		switch(c){
		case 0:
			break;
		case 1:
			ofile << "title," << aline << endl;
			dtitle=aline;
			break;
		case 2:
			ofile << "email," << aline << endl;
			break;
		case 3:
			ofile << "investigator," << aline << endl;
			break;
		case 4:
			ofile << "description," << aline << endl;
			break;
		case 5:
			ofile << "unix epoch start time," << aline << endl;
			break;
		case 6:
			ofile << "last experiment frame number,"  << aline << endl;
			break;
		case 7:
			ofile << "strain,"  << aline << endl;
			break;
		case 9:
			ofile << "experiment directory,"  << aline << endl;
			break;
		case 10:
			ofile << "starting N," << aline << endl;
			break;
		case 11:
			ofile << "starting Age in days," << aline << endl;
			break;
		case 12:
			ofile << "expID," << expID << endl;
			break;
		case 15:
			ofile << "plate number," << aline << endl;
			break;
		case 16:
			ofile << "well," << aline << endl;
			break;




		}//end switch


		c++;
	}//end while lines in description
	ifile.close();

	ofile << printWormLifespan(dtitle);


	ofile.close();

	debugger.close();

	//output something so data calls success
		cout << HTTPHTMLHeader() << endl;
		cout << html() << head(title("WormBot Response")) << endl;
		cout << "</body></html>" << endl;


	return 0;
}
