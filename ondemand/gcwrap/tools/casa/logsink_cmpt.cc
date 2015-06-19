
/***
 * Framework independent implementation file for logsink...
 *
 * Implement the logsink component here.
 * 
 * // TODO: WRITE YOUR DESCRIPTION HERE! 
 *
 * @author
 * @version 
 ***/

#include <iostream>
#include <fstream>
#include <logsink_cmpt.h>
#include <casa/Logging/LogSink.h>
#include <casa/Logging/LogFilter.h>
#include <tools/casa/TSLogSink.h>
#include <casa/Logging/NullLogSink.h>
#include <casa/Logging/StreamLogSink.h>
#include <stdcasa/version.h>
#include <unistd.h>
#include <sys/param.h>
#include <tools/utils/CasapyWatcher.h>
#include <casa/OS/File.h>
#include <casa/BasicMath/Random.h>
#include <casa/System/Aipsrc.h>

using namespace std;
using namespace casa;

namespace casac {

	/*
static ofstream *logfile = new ofstream("logfile");
static LogSinkInterface *thelogsink
  = new StreamLogSink(logfile);
  */
static string theLogName;

//logsink::logsink():thelogsink(0)
logsink::logsink()
{
  if(!theLogName.size()){
     char *buff = NULL;
     char *mybuff = getcwd(buff, MAXPATHLEN);
     theLogName = string(mybuff) + string("/casapy.log");
  }
  //cout << "thelogsink=" << thelogsink << endl;
  thelogsink = new casa::TSLogSink();
  setlogfile(theLogName);
  itsorigin = new LogOrigin("casa");
  thelogsink->postLocally(LogMessage("", *itsorigin, LogMessage::NORMAL));
  globalsink = false;
  logname = theLogName ;
  //version();
  
   string tmpname = "" ;
      String logfileKey="logfile.no.default";
      String logname2;
      if(!Aipsrc::find(logname2, logfileKey)){
         tmpname = "casapy.log";
      } else {
         //tmpname = logname2;
         ACG g(7326458, 98);
         tmpname = "/tmp/"+String::toString(g.asuInt());
         //tmpname = "null";
      }
   //std::cout << "logfile.default: " << tmpname << std::endl;
   if(tmpname != "null") {
      casa::File filein( tmpname ) ;
      logname = filein.path().absoluteName() ;
      //static_cast<TSLogSink*>(thelogsink)->setLogSink(logname);
   }
   else
      //thelogsink = new NullLogSink();
      ;
      
}

std::string logsink::version(){
  if(!thelogsink){
       thelogsink = &LogSink().globalSink();
  }
  ostringstream os1, os2;
  os1 << "CASA Version ";
  casa::VersionInfo::report(os1);
  os2 << "  Tagged on: "<< casa::VersionInfo::date();
  std::string mymess = os1.str();

  thelogsink->postLocally(LogMessage(mymess, *itsorigin, LogMessage::NORMAL));
  thelogsink->postLocally(LogMessage(os2.str(), *itsorigin, LogMessage::NORMAL));
  return mymess;
}

logsink::~logsink()
{
	delete itsorigin;
	if(!globalsink)
	   delete thelogsink;
	thelogsink=0;
	itsorigin=0;
}

bool logsink::origin(const std::string &fromwhere)
{
    bool rstat(true);
    if(!thelogsink){
       thelogsink = &LogSink().globalSink();
    }
    delete itsorigin;
    itsorigin = new LogOrigin("casa");
    //String* taskname = new String(fromwhere);
    taskname = new String(fromwhere);
    itsorigin->taskName(*taskname);
    thelogsink->setTaskName(*taskname);
    LogSink().globalSink().setTaskName(*taskname);
    return rstat;
}

bool logsink::filter(const std::string &level)
{
    if(!thelogsink){
       thelogsink = &LogSink().globalSink();
    }
	bool rstat(true);
	LogMessage::Priority priority = LogMessage::NORMAL;
        if (level == "DEBUG" || level == "DEBUGGING")
           priority = LogMessage::DEBUGGING;
        else if (level == "DEBUG1")
           priority = LogMessage::DEBUG1;
        else if (level == "DEBUG2")
           priority = LogMessage::DEBUG2;
        else if (level == "NORMAL")
           priority = LogMessage::NORMAL;
        else if (level == "NORMAL1")
           priority = LogMessage::NORMAL1;
        else if (level == "NORMAL2")
           priority = LogMessage::NORMAL2;
        else if (level == "NORMAL3")
           priority = LogMessage::NORMAL3;
        else if (level == "NORMAL4")
           priority = LogMessage::NORMAL4;
        else if (level == "NORMAL5")
           priority = LogMessage::NORMAL5;
        else if (level == "INFO" || level == "NORMAL")
           priority = LogMessage::NORMAL;
        else if (level == "INFO1")
           priority = LogMessage::NORMAL1;
        else if (level == "INFO2")
           priority = LogMessage::NORMAL2;
        else if (level == "INFO3")
           priority = LogMessage::NORMAL3;
        else if (level == "INFO4")
           priority = LogMessage::NORMAL4;
        else if (level == "INFO5")
           priority = LogMessage::NORMAL5;
        else if (level == "WARN")
           priority = LogMessage::WARN;
        else if (level == "ERROR" || level == "SEVERE")
           priority = LogMessage::SEVERE;
	else 
		rstat = false;
	if(rstat){
	   LogFilter filter(priority);
	   thelogsink->filter(filter);
	   
	   // Also set for any watchers.
	   CasapyWatcher::logChanged_(priority);
	}
	return rstat;
}

bool logsink::post(const std::string& message,
		   const std::string& priority,
		   const std::string& origin)
{
	return postLocally(message, priority, origin);
}

bool
logsink::postLocally(const std::string& message,
                     const std::string& priority,
		     const std::string& origin)
{
    LogMessage::Priority messagePriority = LogMessage::NORMAL;

    if(!thelogsink){
       thelogsink = &LogSink().globalSink();
    }
    if(!itsorigin)
       itsorigin = new LogOrigin("casa");
    itsorigin->className(origin);
    taskname = new String(origin);
    //String* taskname = new String(origin);
    //cout << "(*taskname==\"\")=" <<  (*taskname == "") << endl;
    thelogsink->setTaskName(*taskname);
    //LogSink().globalSink().setTaskName(*taskname);
    if (priority == "DEBUG" || priority == "DEBUGGING")
       messagePriority = LogMessage::DEBUGGING;
    else if (priority == "DEBUG1")
       messagePriority = LogMessage::DEBUG1;
    else if (priority == "DEBUG2")
       messagePriority = LogMessage::DEBUG2;
    else if (priority == "NORMAL5")
       messagePriority = LogMessage::NORMAL5;
    else if (priority == "NORMAL4")
       messagePriority = LogMessage::NORMAL4;
    else if (priority == "NORMAL3")
       messagePriority = LogMessage::NORMAL3;
    else if (priority == "NORMAL2")
       messagePriority = LogMessage::NORMAL2;
    else if (priority == "NORMAL1")
       messagePriority = LogMessage::NORMAL1;
    else if (priority == "NORMAL")
       messagePriority = LogMessage::NORMAL;
    else if (priority == "INFO5")
       messagePriority = LogMessage::NORMAL5;
    else if (priority == "INFO4")
       messagePriority = LogMessage::NORMAL4;
    else if (priority == "INFO3")
       messagePriority = LogMessage::NORMAL3;
    else if (priority == "INFO2")
       messagePriority = LogMessage::NORMAL2;
    else if (priority == "INFO1")
       messagePriority = LogMessage::NORMAL1;
    else if (priority == "INFO")
       messagePriority = LogMessage::NORMAL;
    else if (priority == "WARN")
       messagePriority = LogMessage::WARN;
    else if (priority == "SEVERE")
       messagePriority = LogMessage::SEVERE;
    else if (priority == "ERROR")
       messagePriority = LogMessage::SEVERE;
    return thelogsink->postLocally(LogMessage(message,
       *itsorigin,
       messagePriority));
}

std::string
logsink::localId()
{
  return thelogsink->localId();
}

std::string
logsink::id()
{
  return thelogsink->id();
}

bool logsink::setglobal(const bool isglobal)
{
   bool rstat(true);
   
   if(isglobal){
      if (globalsink==isglobal && thelogsink)
         return rstat;
      LogSink().globalSink(thelogsink);
      globalsink = isglobal;
   } //else {
   //   LogSinkInterface *dummy = new StreamLogSink(LogMessage::NORMAL, &cerr);
   //   LogSink().globalSink(dummy);
   //}
   return rstat;
}
//
bool logsink::setlogfile(const std::string& filename)
{
   bool rstat(true);

   string tmpname = filename ;
   if(!filename.size()){
      String logfileKey="logfile.no.default";
      String logname2;
      if(!Aipsrc::find(logname2, logfileKey)){
         tmpname = "casapy.log";
      } else {
         //tmpname = logname2;
         ACG g(7326458, 98);
         tmpname = "/tmp/"+String::toString(g.asuInt());
         //tmpname = "null";
      }
   }
   if(tmpname != "null") {
      casa::File filein( tmpname ) ;
      logname = filein.path().absoluteName() ;
      static_cast<TSLogSink*>(thelogsink)->setLogSink(logname);
   }
   else
      thelogsink = new NullLogSink();

   //
   // Also set for any watchers.
   CasapyWatcher::logChanged_(logname);
      
   return rstat;
}

bool
logsink::showconsole(const bool onconsole)
{
   bool rstat(true);
   if(thelogsink)
      thelogsink->cerrToo(onconsole);
   return rstat;
}
std::string
logsink::logfile()
{
  //return theLogName;
  return logname ;
}


} // casac namespace
