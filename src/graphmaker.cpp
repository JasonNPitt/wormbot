#include <sstream>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/replace.hpp>


#include <cgicc/CgiDefs.h>
#include <cgicc/Cgicc.h>
#include <cgicc/HTTPHTMLHeader.h>
#include <cgicc/HTMLClasses.h>


#define WELL_STATE_START 2
#define WELL_STATE_ACTIVE 1
#define WELL_STATE_STOP 0

#define GFP_ON 1
#define TRANSMITTED_ON 1
#define TIMELAPSE_ON 1
#define DAILY_MONITOR_ON -2

#define GFP_OFF 0
#define TRANSMITTED_OFF 0
#define TIMELAPSE_OFF 0
#define DAILY_MONITOR_OFF -1

//field order for TEMPLATE CSV
#define PLATE 0
#define ROW 1
#define WELL 2
#define TIMELAPSE 3
#define DAILYMONITOR 4
#define EMAIL 5
#define INVESTIGATOR 6
#define TITLE 7
#define DESCRIPTION 8
#define STARTN 9
#define AGE 10
#define STRAIN 11

//field order for LIFESPAN CSV
#define L_TITLE 0
#define L_EMAIL 1
#define L_INVESTIGATOR 2
#define L_DESCRIPTION 3
#define L_STARTTIME 4
#define L_LASTFRAME 5
#define L_STRAIN 6
#define L_DIRECTORY 7
#define L_STARTN 8
#define L_STARTAGE 9
#define L_EXPID 10
#define L_PLATENUM 11
#define L_WELL 12
#define L_N 13
#define L_MEAN 14
#define L_MEDIAN 15
#define L_MAX 16
#define MAXOUTFLAGS 17


using namespace std;
using namespace cgicc;


//globals
Cgicc cgi;
string datapath;
stringstream externaldump;		//full processed file to dump

struct XYpair{
	int x;
	int y;
};

struct wellSort{
	int rank;
	string welldataline;

	bool operator < (const wellSort str) const {
		return (rank < str.rank);
	}
};

void sortJobList(void);

XYpair getXYpair(string queryline) {
	string filename;
	filename = datapath + string("platecoordinates.dat");
	ifstream ifile(filename.c_str());
	string readline;
	XYpair found;
	found.x=0;
	found.y=0;
	string token;

	while (getline(ifile,readline)) {
		if (readline.find(queryline) != std::string::npos) {
			stringstream isis(readline);
			getline(isis, token, ','); //remove wellname
			getline(isis, token, ','); //get x
			found.x = atoi(token.c_str());
			getline(isis, token, ','); //get y
			found.y = atoi(token.c_str());

			return(found);
		}
	}
	return(found);
}


string removeNastiness(string jerky){

	 boost::replace_all(jerky,",","COMMA");
	// boost::replace_all(jerky,".","PERIOD");
	 boost::replace_all(jerky,"/","SLASH");
	 boost::replace_all(jerky,"\\","BACKSLASH");
	 boost::replace_all(jerky,";","SEMICOLON");
	 boost::replace_all(jerky,":","COLON");
	 boost::replace_all(jerky,"~","TILDE");
	 boost::replace_all(jerky,"&","AND");
	 boost::replace_all(jerky,"|","PIPE");
 return (jerky);
}//end removeNastiness






void dumpLifespan(long expID, bool useMin, bool useIDs, vector<int> fieldoutputs, ofstream& ofile, bool useTab){
	int linecounter =0; //used to avoid % in description fields
	stringstream filename;
	stringstream metaouts;
	string meanline;
	string nline;
	filename << datapath << expID << "/lifespanoutput_" << expID << ".csv";	
	ifstream ifile(filename.str().c_str());
	//cout << filename.str() << "<P>" << endl;
	if (!ifile) {
		cout << "# lifespan data for experimentID " << expID << " was not found" << endl;
	}//end if file not found

		

	string line;
	bool isMinutes=false;
	bool firstpass=true;
	bool doDump=false;
	bool foundsecond=false;

	while (getline(ifile, line)){
		for (vector<int>::iterator citer = fieldoutputs.begin(); citer != fieldoutputs.end(); citer++){
		 	if (linecounter == (*citer)) metaouts << "#" << line << endl; //dump the line to stringstream 		
		}//end for each requested output field		
		linecounter++;
		
		bool descriptorLine=false;
		if (line.find('%') != string::npos && firstpass && linecounter > MAXOUTFLAGS) {
			firstpass=false;
			if(!useMin)doDump = true;
			descriptorLine=true;
		}//end if found the first oasis descriptor
		else if (line.find('%') != string::npos && !firstpass){
			descriptorLine=true;
			foundsecond=true;
			if (!useMin)
				return; else doDump=true;
		} //end if found the second oasis descriptor
		 if (useTab) boost::replace_all(line, ",", "\t");
		if (doDump){
			if (descriptorLine && useIDs){
			   cout << "%" << expID << endl;
			   cout <<  metaouts.str();

			   ofile << "%" << expID << endl;
			   ofile <<  metaouts.str();	
			   
			} else{
				cout << line << endl;
				ofile << line << endl;
				
				if (descriptorLine) {
					cout <<  metaouts.str();
					ofile <<  metaouts.str();
				}				 
				
			}
		}//end if dump

	}//end while lines in the file

}//end dumpLifespan


int main(int argc, char **argv) {
	try {

		vector<int> outputflags;

		ifstream readpath("data_path");

		
		

		readpath >> datapath;

// Send HTTP header
		cout << HTTPHTMLHeader() << endl;

		// Set up the HTML document
		cout << html() << head(title("WormBot GraphMaker Output")) << endl;
		cout << body() << endl;

		cout << br() <<endl;
		cout << img().set("src", "/wormbot/img/Bender.png").set("width","300") << endl;
		cout << br() <<endl;

		
		cout << "Oasis Output=" "<P><pre>" << endl;

		
		//genetrate a filname using the epoch
		time_t t;
		time(&t);
		stringstream oss;
		oss << "/wormbot/graphs/" << t << "_graphmaker_output.csv" ;

		cout << "<a href=\"" << oss.str() << "\">Download: " << oss.str() << "</a><P>" << endl;

		ofstream outputfile(oss.str().c_str());

		stringstream restorecommand;


		stringstream expparser;

		bool useMinutes =false;
		bool preDead = false;
		bool useIDs= false;
		bool useTabs = false;

		useMinutes=cgi.queryCheckbox("useMinutes");
		useIDs= cgi.queryCheckbox("useIDs");
		useTabs= cgi.queryCheckbox("useTabs");

		for (int i=0; i < MAXOUTFLAGS; i++) {
			stringstream checkbx;
			checkbx << i;
			if (cgi.queryCheckbox(checkbx.str())) outputflags.push_back(i);
		}//end scan flags
		
		expparser << cgi("experiments"); 
		string atoken;
		while (getline(expparser,atoken,' ')){
			
			if (atoken.find("-") != string::npos) {
				vector<string> ids;
				boost::split(ids,atoken,boost::is_any_of("-"));
				for (long i=atol(ids[0].c_str()); i <= atol(ids[1].c_str()); i++){
					dumpLifespan(i,useMinutes, useIDs, outputflags, outputfile, useTabs);
				}//end for each id in range	
			} //end if found a range
			else dumpLifespan(atol(atoken.c_str()),useMinutes, useIDs,outputflags, outputfile, useTabs);
			
			cout << "<P>" << endl;

			

		}//end while experiments in list

	
	
		
		// Close the HTML document
		cout << "</pre> " << body() << html();
		outputfile.close();

	} catch(exception& e) {
		// handle any errors - omitted for brevity
	}
}
