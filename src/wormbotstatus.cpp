//============================================================================
// Name        : wormbotstatus.cpp
// Author      : Jason N Pitt 
// Version     :
// Copyright   : MIT LICENSE
// Description : Just runs a script to check if the wormbot is active can reset the serverlock
//============================================================================


#include <sstream>
#include <iostream>
#include <sys/time.h>
#include <stdlib.h>
#include <glob.h>
#include <sys/stat.h>
#include <utime.h>
#include <fstream>
#include <cstdio>
#include <unistd.h>
#include <cgicc/CgiDefs.h>
#include <cgicc/Cgicc.h>
#include <cgicc/HTTPHTMLHeader.h>
#include <cgicc/HTMLClasses.h>




using namespace std;
using namespace cgicc;


//globals
Cgicc cgi;
string datapath;

int main(int argc, char **argv) {



//get the wormbot path
	  ifstream readpath("data_path");
	  readpath >> datapath;


string fn = datapath + "status.html";
string lockname = datapath + "serverlock.lock";
string wormbottmp(datapath + "tmpwormbotstatus");
string aline; 

ifstream lockfile(lockname.c_str());

bool islocked = lockfile.good();
bool retrorunning =false;


ofstream logfile(fn.c_str());
  
	  

	  

logfile << HTTPHTMLHeader() << endl;
logfile << html() << head(title("Wormbot Status")) << endl;
logfile << "   <meta http-equiv=\"refresh\" content=\"10\">" << endl;
logfile << "<body><pre>\n";		

	stringstream commandlist;
	commandlist << "ps -C \"controller,cgiccretro,alignerd,ffmpeg,backupexperiment,deleteexperiment\" -o comm,lstart > " << wormbottmp << endl;
	system(commandlist.str().c_str());
	
	commandlist.str("");	
	commandlist << "echo \"ALIGNERD LOG: \"  >> " << wormbottmp << endl;
	system(commandlist.str().c_str());
	commandlist.str("");

	commandlist.str("");	
	commandlist << "tail " << datapath << "alignerd.log >> " << wormbottmp << endl;
	system(commandlist.str().c_str());
	commandlist.str("");

	commandlist.str("");	
	commandlist << "echo \"CONTROLLER LOG: \"  >> " << wormbottmp << endl;
	system(commandlist.str().c_str());
	commandlist.str("");
	
	commandlist.str("");	
	commandlist << "tail " << datapath << "runlog >> " << wormbottmp << endl;
	system(commandlist.str().c_str());
	commandlist.str("");			
		
        ifstream tmpf(wormbottmp.c_str());
	while (getline(tmpf,aline)){
		logfile << aline << endl;
		if (aline.find("cgiccretro") != string::npos) retrorunning = true; 
	}//end while process lines
	//logfile << tmpf.rdbuf();

	tmpf.close();
	
	


logfile.close();

ifstream stat(fn.c_str());

cout << stat.rdbuf();
cout << "</pre>" << endl;

//check for server lock
cout << "retrorunning:" << retrorunning << "<BR>" << endl;
cout << "islocked:" << islocked << "<BR>" << endl;



//clear hung lock

if (islocked && !retrorunning){ remove(lockname.c_str()); cout << "removed hung lock <br>"<<end;}

logfile << "</body></html>" << endl;

stat.close();
return 1;

}//end main

