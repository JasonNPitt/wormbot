//============================================================================
// Name        : marker.cpp
// Author      : Jason N Pitt and Nolan Strait
// Version     :
// Copyright   : MIT LICENSE
// Description : Hello World in C++, Ansi-style
//============================================================================


#include <sys/stat.h>
#include <glob.h>
#include <boost/algorithm/string.hpp> // include Boost, a C++ library
#include <boost/lexical_cast.hpp>
#include <vector>
#include <stdio.h>

#include <cgicc/CgiDefs.h>
#include <cgicc/Cgicc.h>
#include <cgicc/HTTPHTMLHeader.h>
#include <cgicc/HTMLClasses.h>

#include <opencv2/opencv.hpp>


using namespace std;
using namespace cgicc;
using namespace cv;
using namespace boost;

//globals
Cgicc cgi;
string directory("");
string fulldirectory("");
string currfilename;
string expTitle;
vector<string> filelist;
vector<string> alignedlist;
long expID;
long frametime;
int currframe=0;
int numframes=0;
int day;
double hour;
int doalign=0;
int numaligned=0;
int numworms=0;
int skipframes=1;

stringstream root_dir;
stringstream datapath;




class Worm {
public:
	int x;
	int y;
	int currf;
	int number;
	int daysold;
	int minutesold;
	long secondsold;

	Worm(int sx, int sy, int curr, int wormnumber,long secs){

		x=sx;
		y=sy;
		currf=curr;
		number=wormnumber;
		daysold=0;
		minutesold=0;
		secondsold=secs;
		minutesold = secondsold / 60;
		daysold = secondsold /86400;


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
        daysold=atoi(token.c_str());
        getline(ss,token,',');
        minutesold=atoi(token.c_str());
        //cout << "<br>debug:" << drawDiv();

		}//end constructor

	string printData(void){

		stringstream ss;
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
long getLifespan(string filename);
string buildMovie(string filename, int startframe, int endframe);



vector<Worm> wormlist;

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
	int maxlifespan;
	int n;
	double mean;
	double median;


	Lifespan(vector<Worm>  myworms){
		//getmaxlifespan
		maxlifespan=0;
		n=0;
		double total=0;
		vector<int> formedian;

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






		for (int i=0; i <= maxlifespan; i++){
		     Day today = Day();

		     for(int j=0; j < myworms.size(); j++){
		    	 if (i==myworms[j].daysold) {
		    		 today.numdead++;
		    	 }//if died this day
		    	 if (i < myworms[j].daysold){
		    		 today.numalive++;
		    	 }//end if not dead yet

		     }//end for each worm
		     days.push_back(today);

		}//end for each day

	}//end constructor

	string getOasisListHTML(void){
		stringstream oss;
		for(int i =0; i <= maxlifespan; i++){
			oss << i << "\t" << days[i].numdead << endl;

		}//end for each day
		return (oss.str());
	}//end get oasis list



};

void alignDirectory(void){
	vector<int> compression_params;
	 compression_params.push_back(CV_IMWRITE_PNG_COMPRESSION);//(CV_IMWRITE_PXM_BINARY);
	 compression_params.push_back(0);



     //save image0
	 if (numaligned ==0){
		 Mat frame=imread(filelist[0]);
		 Mat frame_gray;
		 cvtColor(frame, frame_gray, CV_BGR2GRAY); //make it gray
		 replace_all(filelist[0],"frame","aligned");
		 imwrite(filelist[0], frame_gray, compression_params );
		 numaligned=1;
	 }//end if numaligned is zero


	for (int i=numaligned; i < numframes; i++){
					cout << "." << endl; //keep webserver from timing out
		            Mat frame=imread(filelist[i]);
		            Mat frame_gray;
		            Mat im2_aligned;

					cvtColor(frame, frame_gray, CV_BGR2GRAY); //make it gray

					Mat im1= imread(filelist[i-1]);
					Mat im1_gray;
					cvtColor(im1, im1_gray, CV_BGR2GRAY);

					// Define the motion model
					const int warp_mode = MOTION_TRANSLATION;

					// Set a 2x3 warp matrix
					Mat warp_matrix;
	    			warp_matrix = Mat::eye(2, 3, CV_32F);
	    			 // Specify the number of iterations.
	    			int number_of_iterations = 5000;

	    			 // Specify the threshold of the increment
	    			 // in the correlation coefficient between two iterations
	    			 double termination_eps = 1e-10;

	    			 // Define termination criteria
	    			 TermCriteria criteria (TermCriteria::COUNT+TermCriteria::EPS, number_of_iterations, termination_eps);

	    			 // Run the ECC algorithm. The results are stored in warp_matrix.
	    			 findTransformECC(im1_gray,frame_gray,warp_matrix,warp_mode,criteria);
	    			 warpAffine(frame, im2_aligned, warp_matrix, im1.size(), INTER_LINEAR + WARP_INVERSE_MAP);
	    			 Mat im2_aligned_gray;
	    			 cvtColor(im2_aligned, im2_aligned_gray, CV_BGR2GRAY);
	    			 replace_all(filelist[i],"frame","aligned");
	    			 imwrite(filelist[i], im2_aligned_gray, compression_params );
	}//end for each frame
}//end alignDirectory


void readDirectory(void){
	glob_t glob_result;
	glob_t aligned_result;

	stringstream globpattern;
	globpattern  << fulldirectory.c_str() << string("frame*.png");
     cout << "fulldirectory:" << fulldirectory << "<br>" << endl;
	cout <<"globpat:" << globpattern.str() << "<br>" << endl;

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


}//end readDirectory

void deleteWorms(string filename){
	ifstream ifile(filename.c_str());
	string inputline;
	vector<Worm> dworms;

	while(getline(ifile,inputline)){
	    	 Worm myworm(inputline);

	    	 //check to see if this worm needs deleting
	    	 stringstream ss;

	    	 ss << "w" << myworm.number;
	    	 if (cgi.queryCheckbox(ss.str())){
	    		 //do nothing
	    	 } else{
	    		 dworms.push_back(myworm);
	    	 }//end if not deleted

	     }//end while inputlines
	ifile.close();



	ofstream ofile(filename.c_str());
	for (vector<Worm>::iterator citer = dworms.begin(); citer != dworms.end(); citer++){
		    	ofile << (*citer).printData();

	 }//end for all worms
	ofile.close();

}//end deleteWorms

void addWorm(int x, int y, int curr, string filename){
	//saves a worm to the wormfile
	ifstream ifile(filename.c_str());
	int wormcounter=0;
	string wormline;

	int lastnum=0;


	while (getline(ifile,wormline)){
		Worm myworm(wormline);
		lastnum=myworm.number;
		wormcounter++;
	}
	stringstream ss;
	ss << fulldirectory << "description.txt";
	Worm myworm(x,y,curr,++lastnum,getLifespan(ss.str()));
	ifile.close();
	ofstream ofile(filename.c_str(), std::ios::app);
	ofile << myworm.printData();
	ofile.close();
}//end addWorm

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

void printWormDivs(void){
     for (vector<Worm>::iterator citer = wormlist.begin(); citer != wormlist.end(); citer++){
    	 cout << (*citer).drawDiv(currframe) <<endl;


     }//end for each worm

}//end printwormdivs

void printWormImgs(void){
     for (vector<Worm>::iterator citer = wormlist.begin(); citer != wormlist.end(); citer++){
    	 cout << (*citer).drawIcon(currframe) <<endl;


     }//end for each worm

}//end printwormdivs

string printWormLifespan(void){
	stringstream oss;
	if (wormlist.empty()) return oss.str();
    oss << "<pre>" << endl;
	oss << "%" << expTitle << endl;

	Lifespan ls(wormlist);

	oss << ls.getOasisListHTML();
	oss << "</pre>" << endl;
	oss << "<h3>" <<endl;
	oss << "N:" << ls.n << "<br>" << endl;
	oss << "Mean: " << ls.mean << "<br>" << endl;
	oss << "Median:" << ls.median << "<br>" << endl;
	oss << "Max: " << ls.maxlifespan << "<br>" << endl;


	return (oss.str());
}//end printwormlifespan


void printWormForm(void){
	cout << "<div class=\"wormlist\"> <h3>" << endl;
	cout << "<input type=\"submit\" value=\"remove selected worm(s)\"> <br>" << endl;
     for (vector<Worm>::iterator citer = wormlist.begin(); citer != wormlist.end(); citer++){
    	 cout << (*citer).drawFormField(currframe) <<endl;


     }//end for each worm
     cout << "</h3> " << endl;

     cout << printWormLifespan() << endl;
     cout << "<a href=\"http://sbi.postech.ac.kr/oasis2/\" target=\"_blank\"> Oasis 2 </a><br>" << endl;

     cout << "</div>" << endl;

}//end printwormdivs

long getLifespan(string filename){
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
            cout << "daysold:" << daysold << endl;
            cout << "expstarttime:" << expstarttime << endl;
            cout << "frametime:" << frametime << endl;
             return (frametime-(expstarttime -(86400 * daysold)));

		}//end process starting age
		i++;
	}


}//end getexperimentstarttime



string getDescription(string filename){
	ifstream ifile(filename.c_str());
	string inputline;
	stringstream oss;


	int i=0;
	while (getline(ifile,inputline)){

          switch(i){
          case 1:
        	  oss << "Title:" << inputline << "<br>" << endl;
        	  expTitle = inputline;
        	  break;
          case 3:
        	  oss << "Investigator:" << inputline << "<br>" << endl;
        	  break;
          case 4:
        	  oss << "Description:" << inputline << "<br>" << endl;
        	  break;
          case 7:
        	  oss << "Strain:" << inputline << "<br>" << endl;
        	  break;
          case 10:
        	  oss << "N:" << inputline << "<br>" << endl;
        	  break;
          case 11:
        	  oss << "Starting Age (days):" << inputline << "<br>" << endl;
        	  break;
          case 15:
        	  oss << "Well:" << inputline;
        	  break;
          case 16:
        	  oss << inputline << "<br>" << endl;
        	  break;


          }//end switch


		i++;
	}//end while

return (oss.str());
}//end getdescription


long getFileCreationTime(string filename){
	struct stat attr;
	stat(filename.c_str(),&attr);
	long time = attr.st_mtim.tv_sec;
	cout << "setframetime:" << time << endl;
	return (time);

}//end getFileCreationTime

void processStartForm(void){
	stringstream oss;
	string hiddenexpID;
	hiddenexpID = cgi("loadedexpID");
	if(!hiddenexpID.empty() ){
		expID=atol(cgi("loadedexpID").c_str());
	}else{
		expID=atol(cgi("expID").c_str());
		//doalign=1;
	}
	oss << datapath.str() << expID << "/";

	fulldirectory=oss.str();

	oss.str(""); //nnull

	oss << "/wormbot/" << expID << "/";
	directory = oss.str();

	//cout << "processdir:" << directory << "<br>" << endl;

    //load the currentframe

	string readcurr;
	currfilename="curr.dat";
	currfilename = fulldirectory+currfilename;
	ifstream currfile(currfilename.c_str());
	getline(currfile,readcurr);
	currframe=atoi(readcurr.c_str());

	stringstream number;
	stringstream ss;
		number << setfill('0') << setw(6) << currframe;



	    ss << fulldirectory << "frame" << number.str() <<".png";

	    frametime = getFileCreationTime(ss.str());



}//end processstartform


void processControlPanel(void){
	int clickx=0;
	int clicky=0;

	clickx=atoi(cgi("cpan.x").c_str());
	clicky=atoi(cgi("cpan.y").c_str());


	skipframes= atoi(cgi("frameskip").c_str());

	int moviestartframe = atoi(cgi("moviestartframe").c_str());
	int movieendframe = atoi(cgi("movieendframe").c_str());


	//check build movie button
	if (clickx >= 95 && clickx <= 202 && clicky >=538 && clicky <=595) {
		string moviefilename;


		buildMovie(fulldirectory, moviestartframe,movieendframe);


	}//end if click movie button


	//check forward button
	if (clickx >= 180 && clickx <= 278 && clicky >=346 && clicky <=384) {
		currframe+=skipframes;
		if (currframe >= numframes) currframe=numframes-1;

	}
	//check reverse button
	if (clickx >= 22 && clickx <= 122 && clicky >=346 && clicky <=384) {
		currframe-=skipframes;

			if (currframe < 0) currframe=0;

		}
	//check start button
		if (clickx >= 22 && clickx <= 118 && clicky >=494 && clicky <=526) {
			currframe=0;

		}
		//check end button
				if (clickx >= 182 && clickx <= 278 && clicky >=494 && clicky <=526) {
					currframe=numframes-1;

				}

				ofstream ofile(currfilename.c_str());
				ofile << currframe << endl;
				ofile.close();



}//end processControlPanel

string buildMovie(string filename, int startframe, int endframe){
	stringstream oss;
	stringstream ffmpeg;
	stringstream lastcomp;


	ffmpeg << "./ffmpeg -y -f image2 -start_number " << startframe
		<<" -i "<< filename << "aligned%06d.png ";

	if (wormlist.size() != 0) {

		for (int i=0; i < wormlist.size(); i++){
			ffmpeg << " -i /var/www/dead.png "; //load the worm circle for each dead worm
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

	ffmpeg << " -q:v 1 -vframes " << (endframe+1)-startframe << " " << filename << expID <<".avi 2>&1 | tee /tmp/ffmpegstdout.txt" << endl;

	system(ffmpeg.str().c_str());

	string fn = datapath.str() + "/tempffmpegcomand";
	ofstream ofile(fn.c_str());
	ofile << ffmpeg.str() <<endl;
	ofile.close();

	oss << "worked";

	return (oss.str());
}//end buildmovie

string getCurrframe(void){
	stringstream oss;
	stringstream ss;

	stringstream number;
	number << setfill('0') << setw(6) << currframe;

	oss << directory << "aligned" << number.str() <<".png";

    ss << directory << "frame" << number.str() <<".png";

    frametime = getFileCreationTime(ss.str());

	return (oss.str());

}//end getCurrframe





int main(int argc, char **argv) {

	ifstream t("var/root_dir");
	root_dir << t.rdbuf();

	string fn = "/usr/lib/cgi-bin/data_path";
	ifstream t2(fn.c_str());
	string apath;
	getline(t2,apath);
	datapath.str(apath);

	int mapx;
	int mapy;

	mapx=atoi(cgi("mappy.x").c_str());
	mapy=atoi(cgi("mappy.y").c_str());
	cout << HTTPHTMLHeader() << endl;
 	cout << html() << head(title("WormBot Lifespan Editor")) << endl;
	processStartForm();
	stringstream ss;
	ss << fulldirectory << "wormlist.csv";

	if (mapx != 0 && mapy !=0 )
		addWorm(mapx,mapy, currframe, ss.str());

	deleteWorms(ss.str());
	loadWorms(ss.str());
	readDirectory();
	processControlPanel();

	//buildMovie(fulldirectory,0,300);

	cout << "<style type =text/css> "<< endl;
	cout << " a:link {display:inline-block;background-color:#ff0000; color:white; padding:10px,10px; }" <<endl;
	cout << ".body { background-color:black;}" <<endl;
	cout << ".under { position:absolute; left:0px; top:0px; z-index:0;}" <<endl;

	printWormDivs();

	cout << ".controlpanel { font-weight:bold;position:absolute; left:0px; top:0px; z-index:1; color:white;width:300px;font-size:0.75em;}" << endl;
	cout << ".wormlist { position:absolute; left:1525px; top:0px; width:300px; height:600px; background-color:#808080; z-index:2; color:white; overflow:auto;}" << endl;
	cout << "</style>";
	cout << "<body class=\"body\">" << endl;

	// Set up the HTML document
	cout << "<form action=\"/cgi-bin/marker\">" << endl;
	cout << "<input type=\"hidden\" name=\"loadedexpID\" value=\"" << expID << "\"> " << endl;
    printWormImgs();
	cout << "<input type=\"image\" src=\"" << getCurrframe() << "\" class=\"under\" name=\"mappy\">" << endl;
    cout << "<input type=\"image\" name=\"cpan\" src=\"/wormbot/img/CONTROL.png\" class=\"controlpanel\">" << endl;

    cout << "<div class=\"controlpanel\" >" << endl;
	//cout << cgi("mappy.x") <<endl;
	//cout << cgi("mappy.y") <<endl;
	cout << "current frame: " << currframe << "<br>" << endl;
	cout << "total frames: " << numframes << "<br>" << endl;
	stringstream dss;
	dss << fulldirectory << "description.txt";
	cout << getDescription(dss.str());
    cout << "Start Frame: <input type=\"text\" name=\"moviestartframe\" size=4> <br> "<< endl;
    cout << "  End Frame: <input type=\"text\" name=\"movieendframe\" size=4> <br> "<< endl;
    cout << "Frame skip: <input type=\"text\" name=\"frameskip\" size=4 value=\"" << skipframes << "\"><br> "<< endl;
    cout << "<h3><a href=\"" << directory << expID << ".avi\" > Current movie </a>" << "</h3><br>" <<endl;
	cout << "</div>" << endl;
	printWormForm();

	cout << "</body></html>" << endl;
	return 0;
}
