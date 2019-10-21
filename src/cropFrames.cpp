//============================================================================
// Name        : cropFrames

// Author      : jason pitt
// Version     : 1.0
// Copyright   : sept 2019
// Description : This application takes a set of alignedimages and removes them from the stack
//============================================================================

//stdlib
#include <iostream>
#include <iomanip>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <glob.h>
#include <utime.h>
#include <fstream>
#include <cstdio>
#include <unistd.h>

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

#include "constants.h"

//namespace
using namespace std;
using namespace cgicc;
using namespace cv;

using boost::property_tree::ptree;
using boost::property_tree::read_json;
using boost::property_tree::write_json;



//Globals
int expID;
string datapath; //hold the path to all the robot data
//read in path from /usr/lib/cgi-bin/data_path
ifstream pathfile("/usr/lib/cgi-bin/data_path");
int numworms=0;


ofstream debugger;










// F U N C T I O N S


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




string swapTimes(string filename, string newfilename){
	stringstream outtime;

	struct stat attr;
	struct utimbuf thetimev;
	stat(filename.c_str(),&attr);

	
	thetimev.actime=attr.st_mtim.tv_sec;
	thetimev.modtime=attr.st_mtim.tv_sec;
	
	outtime << ctime(&thetimev.modtime);

	utime(newfilename.c_str(), &thetimev);

	return (outtime.str());
}




long getFileCreationTime(string filename){
	struct stat attr;
	stat(filename.c_str(),&attr);
	long time = attr.st_mtim.tv_sec;

	return (time);

}//end getFileCreationTime

int setTotalFrames(int numframes, string filepath){
	debugger << "passed numframes: " << numframes <<endl;
	//modify
	string currname = filepath + "description.txt";
	debugger << "file output name: " << currname << endl;
	stringstream oss;
		
	ifstream curr(currname.c_str());
	string line;
	int c=0;
	while(getline(curr,line)){
		//debugger << "c: " << c << " DESC: " << DESC_TOTAL_FRAMES << "line: " << line;
		if (c == DESC_TOTAL_FRAMES){
			 oss << numframes << endl; 
			//debugger << "TRUE" << endl;
		}else{
			 oss << line << endl;
			//debugger << "FALSE" << endl;
		}
		c++;
	}//end while lines in file		
	
	curr.close();
	
	stringstream mod;
	mod << "chmod a+wr " << currname << endl;
	system(mod.str().c_str());
	ofstream ofile(currname.c_str(), ios_base::out);
	ofile << oss.str();
	ofile.close();

return(0);

}//end setTotalFrames



void cropFrames(int startCrop, int endCrop, string filepath){

	int totframes=0;
	//get the total number of frames
	stringstream totalfilename;
	totalfilename << filepath << "description.txt";	
	ifstream curr(totalfilename.str().c_str());
	string framesstring;
	int c=0;	
	while (getline(curr,framesstring)){
		if (c==DESC_TOTAL_FRAMES) totframes=atoi(framesstring.c_str());
		c++;
	}//end while lines in description
	
	debugger << "total frames: " << totframes << endl;

	for (int i=startCrop; i <= endCrop; i++){
		stringstream rm,number;
		number << setfill('0') << setw(6) << i;
		//mv the aligned files
		rm << "mv " << filepath << "aligned" << number.str() << ".png " << filepath << "Acensored" << number.str() << ".png" << endl;
		system(rm.str().c_str());
		
		rm.str("");
		//mv the original frames....mv seems to preserve the timestamps otherwise this will need a workaround
		rm << "mv " << filepath << "frame" << number.str() << ".png " << filepath << "Fcensored" << number.str() << ".png" << endl;
		system(rm.str().c_str());
		
		
 
	}//rm each file in the range

	//rename files and copy timestamps from original frames
	bool framepresent = true;	
	int subject =endCrop+1; //start on the file right after the end of the crop
	int target = startCrop;	//start filling at start of Crop
	while(framepresent){
		stringstream mv, subjectnumber, targetnumber, stampsource,targetfilename;
		subjectnumber << setfill('0') << setw(6) << subject;
		targetnumber << setfill('0') << setw(6) << target;
	 	
		mv << "mv " << filepath << "aligned" << subjectnumber.str() << ".png " << filepath << "aligned" << targetnumber.str() << ".png" << endl;
		system (mv.str().c_str());
				
		mv.str("");
		mv << "mv " << filepath << "frame" << subjectnumber.str() << ".png " << filepath << "frame" << targetnumber.str() << ".png" << endl;
		system (mv.str().c_str());
		
		
		//take timestamp from Frame"subject".png to aligned"target".png...actually mv seems to maintain the timestamp so this isn't needed
		//stampsource << filepath << "frame" << subjectnumber.str() << ".png";
		//targetfilename << filepath << "aligned" << targetnumber.str() << ".png";
		//debugger << "stamp:" << swapTimes(stampsource.str(), targetfilename.str());

		subject++;
		target++;
		debugger << "subject:" << subject << " totalframes:" << totframes << endl;
		if (subject >= totframes){ //-(subject-target)){ ///should add actual error checking for out of boundery crops
			 framepresent=false;
			 setTotalFrames((totframes-(endCrop+1-startCrop)), filepath);
		}//end if reach end
	}//end while frames

}//end cropframes

void cropWorms(int startCrop, int endCrop, string wormfile){
	stringstream croppedwormlist;
	ifstream curr(wormfile.c_str());
	string aline;
	
	int c=0;
	while (getline(curr,aline)){
		stringstream tokens(aline);
		string value,remainder;
		int x,y,deathpoint;
		
		getline(tokens,value,','); //x	
		x = atoi(value.c_str());		
		getline(tokens,value,','); //y	
		y = atoi(value.c_str());
		getline(tokens,value,','); //frame
	
		deathpoint = atoi(value.c_str());

		getline(tokens,remainder);
		
		if (deathpoint >= startCrop && deathpoint <= endCrop){ //remove worms inside the crop
			c++; //increment the crop counter
		}else if (deathpoint < startCrop){ //before crop point no need to change
			croppedwormlist << aline << endl; 
		}else if (deathpoint > endCrop) { //if after crop, need to shift
			croppedwormlist << x << "," << y << "," << deathpoint - (endCrop+1-startCrop) << "," << remainder << endl; 

		}//end if past crop
				
	}//end while worms in list 
	curr.close();

	ofstream ofile(wormfile.c_str());
	ofile << croppedwormlist.rdbuf();
	ofile.close();

cout << "removed " << c << " worms" << endl;

}//end cropframes





//////////// M A I N
int main(int argc,char **argv){

	cout << setprecision(5);

	getline(pathfile,datapath);
	pathfile.close();

	string updatepath = datapath + "cropdebug";
	debugger.open(updatepath.c_str());

	stringstream wormfilename;
	 Cgicc cgi;
	 int cropStart,cropEnd=0;
	 
	

	try {


	      
	      expID = atoi(string(cgi("expID")).c_str());
	      cropStart = atoi(string(cgi("cropStart")).c_str());
	      cropEnd = atoi(string(cgi("cropEnd")).c_str());
	     

	     	


              //remove desired frames
	      wormfilename << datapath << expID << "/";
	      cropFrames(cropStart,cropEnd,wormfilename.str());
	   

	      //remove any worms in the cropped segment
	      wormfilename << "wormlist.csv";
	      cropWorms(cropStart,cropEnd,wormfilename.str());	

	     //build the wormlist
	
	//loadWorms(wormfilename.str());	
	     
	     



	} catch (exception& e){

	}//end exception caught


	








	
	//output something so data calls success
		cout << HTTPHTMLHeader() << endl;
		cout << "blah" << endl;
		debugger.close();


	return 0;
}
