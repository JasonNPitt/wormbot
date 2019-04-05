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
#include <cstring>
#include <unistd.h>
#include <cgicc/CgiDefs.h>
#include <cgicc/Cgicc.h>
#include <cgicc/HTTPHTMLHeader.h>
#include <cgicc/HTMLClasses.h>
#include <sys/statvfs.h>




using namespace std;
using namespace cgicc;


//globals
Cgicc cgi;
string datapath;


string reportFreeSpace(string filename){


 	stringstream returnstring;

	struct statvfs buf;
	char cfilename[1024];
	
	char stringoutput[1024];
	strcpy(cfilename, filename.c_str());
	float freegb;

	
	if (!statvfs(cfilename, &buf)) {
	unsigned long long blksize, blocks, freeblks, disk_size, used, free;
	 
	blksize = buf.f_bsize;
	blocks = buf.f_blocks;
	freeblks = buf.f_bfree;
	 
	disk_size = blocks * blksize;
	free = freeblks * blksize;
	used = disk_size - free;

	

	freegb = (float)free/(float)1000000000;
	 
	sprintf(stringoutput,"Disk usage : %lu \t Free space %lu\n", used, free);} else {
	sprintf(stringoutput,"Couldn't get file system statistics\n");
	}

	returnstring << stringoutput <<  "<P>GB Free:" << freegb << endl;

	return returnstring.str();

}




int main(int argc, char **argv) {



//get the wormbot path
	  ifstream readpath("data_path");
	  readpath >> datapath;


string fn = datapath + "ostatus.html";
string lockname = datapath + "serverlock.lock";
string wormbottmp(datapath + "tmpwormbotstatus");
string aline; 

ifstream lockfile(lockname.c_str());

bool islocked = lockfile.good();
bool retrorunning =false;


ofstream logfile(fn.c_str());
  
	  

	  

cout << HTTPHTMLHeader() << endl;
cout << html() << head(title("Wormbot Status")) << endl;
cout << "   <meta http-equiv=\"refresh\" content=\"10\">" << endl;
cout << "<body>\n";

logfile << "<pre>";		

string linkfn = datapath + "status.html";
ifstream linkfile(linkfn.c_str());
cout << linkfile.rdbuf();

	stringstream commandlist;
	commandlist << "ps -C \"controller,cgiccretro,alignerd,ffmpeg,backupexperiment,deleteexperiment,master_control_program\" -o comm,lstart > " << wormbottmp << endl;
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

cout << "<h3> Free Disk Space Available: <P> " << endl;

string joblist("/wormbot/RRRjoblist.csv");
cout << reportFreeSpace(joblist) << endl;




//clear hung lock

if (islocked && !retrorunning){ remove(lockname.c_str()); cout << "removed hung lock <br>" << endl;}

logfile << "</body></html>" << endl;

stat.close();
return 1;

}//end main

