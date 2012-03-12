//# ExtendFlagger.h: 
//# Copyright (C) 1994,1995,1996,1997,1998,1999,2000,2001,2002,2003-2008
//# Associated Universities, Inc. Washington DC, USA.
//#
//# This library is free software; you can redistribute it and/or modify it
//# under the terms of the GNU Library General Public License as published by
//# the Free Software Foundation; either version 2 of the License, or (at your
//# option) any later version.
//#
//# This library is distributed in the hope that it will be useful, but WITHOUT
//# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
//# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
//# License for more details.
//#
//# You should have receied a copy of the GNU Library General Public License
//# along with this library; if not, write to the Free Software Foundation,
//# Inc., 675 Massachusetts Ave, Cambridge, MA 02139, USA.
//#
//# Correspondence concerning AIPS++ should be addressed as follows:
//#        Internet email: aips2-request@nrao.edu.
//#        Postal address: AIPS++ Project Office
//#                        National Radio Astronomy Observatory
//#                        520 Edgemont Road
//#                        Charlottesville, VA 22903-2475 USA
//#
//# $Id$
//#
//# ----------------------------------------------------------------------------
//# Change Log
//# ----------------------------------------------------------------------------
//# Date         Name             Comments


#ifndef EXTENDFLAGGER_H
#define EXTENDFLAGGER_H

//# Includes

#include <casa/aips.h>
#include <casa/iostream.h>
#include <casa/OS/Timer.h>

#include <casa/Arrays/Vector.h>
#include <casa/Arrays/Array.h>
#include <casa/Arrays/Matrix.h>
#include <casa/Arrays/Slicer.h>

#include <tables/Tables/Table.h>
#include <tables/Tables/ArrayColumn.h>
#include <tables/Tables/ScalarColumn.h>
#include <tables/Tables/TableColumn.h>

#include <tables/Tables/ExprNode.h>
#include <tables/Tables/RecordGram.h>

#include <ms/MeasurementSets/MeasurementSet.h>
#include <flagging/Flagging/Flagger.h>


namespace casa { //# NAMESPACE CASA - BEGIN

// <summary>
// class for flaging and extend flagging
// </summary>

// <use visibility=export>

// <reviewed reviewer="" date="" tests="">
// </reviewed>

// <prerequisite>
//#! Classes or concepts you should understand before using this class.
// </prerequisite>

// <etymology>
//
//
//There will be an "extendflag" T/F parameter and a number of
//subparameters. This so that individual extensions can be better
//controlled.
//
//extendflag = T
//   extendcorr = '' # ''=default,'all' (future: 'half')
//   extendchan = '' # ''=default,'all'
//   extendspw = '' # ''=default,'all'
//   extendant = '' # ''=default,'all'
//   extendtime = '' # ''=default,'all' (future: 'scan','field')
//
//In all cases the '' and 'all' options are the ones to implement first.
//Next, if time allows, implement the ones in parentheses. More
//esoteric ones are given below and would be after these (and probably
//need more thought).
//
//NOTE: the extendchan and extendcorr are "in-row" selection (they
//extend the flags to quantities in the same visibility that is plotted
//and selected on), while the others are in different rows. The in-row
//ones will be implemented first (CAS-714), the rest later (CAS-715).
//
//Note that these extensions are combined at the end, thus if you choose
//'all' for all of these I think you will flag the entire dataset if you
//flag any point! So be it. Also, averaging effectively extends the
//flags to everything included in the average.
//
//Individually:
//
//extendcorr
//    '' flag only current point(s) correlations
//    'all' extend flag to all correlations,
//           example: if RR selected then flag RR,RL,LR,LL
//
//    Future options:
//    'half' extend flag to all correlations with current one
//           example: if RR selected then flag RR,RL,LR (not LL)
//           NOTE: if RL selected then this rule would nominally
//           flag RR,RL,LL but not LR, should probably default to
//           'all' in this case?
//
//    Possible other options:
//    <correlation> take standard correlation selection string
//           example: extendcorr='RL,LR'
//
//extendchan
//    '' flag only current selected channels
//    'all' extend flag to all channels
//
//    NOTE: the spw that these apply to is given by extendspw (default is
//          current)
//
//    Possible other options:
//    <chan> take standard channel selection/range string (the part after :
//           in spw string)
//           example: extendchan='0~4,59~63'
//
//extendspw
//    '' flag channel(s) only in current spw
//    'all' extend flag to all spw (of current shape if extendchan not 'all')
//
//    NOTE: if extendchan='' and extendspw='all', then only spw of the same
//shape as the current spw will have the specified channel
//flagged. This is so flagging on "narrow-band" spw will not
//flag "wide-band" spw by mistake.
//          example: if CH 0 of spw with 64 chans is selected, then CH 0 in
//          all spw of 64 chans will flagged by extension
//
//    Possible other options:
//    <spw> take standard spw selection/range string (the part before : in
//           spw string)
//           example: extendspw='0,3~6'
//extendant
//    '' flag only current point(s) baseline
//    'all' flag all baselines (fitting other selection/extension)
//           BEWARE - this option could delete excessive amounts of data
//           NOTE: if the user has chosen iteration='antenna' then 'all'
//           should flag only baselines to the current iteration of antenna.
//           Likewise if iteration='baseline', extendant='all' should flag
//           only the current baseline (in this case will act like '').
//           Otherwise the user would not be iterating.
//
//    Possible other options:
//    'ant1' extend flag to all baselines with the current point(s) ANT1
//    'ant2' extend flag to all baselines with the current point(s) ANT2
//    <antenna> take standard antenna selection string
//           example: extendant='EA11,1&3'
//
//extendtime
//    '' flag only current point(s) integration (or average)
//    'all' flag all integs matching rest of selection in entire dataset -
//           BEWARE - this could delete excessive amounts of data
//           NOTE: if iteration='field' is chosen, then 'all' will flag only
//           integrations in the current field iteration (not all fields)
//
//    Future options:
//    'scan' flag all integs in current scan
//    'field' if xaxis='time' then flag all fields that would fall in box
//           drawn
//           example: would flag target source between plotted calibrators
//
//    Possible other options:
//    <timerange> take a standard timerange string
//            example: extendtime='25:00:00~27:30:00'
//
//
//Note that if the suggested future extend strings using selection
//syntax (e.g.extendchan='10~20') is implemented, the extensions could
//be to data disjoint from what is actually displayed.
//
//GUI Developments:
//
//Eventually, we will want to have an icon on the GUI that will bring up
//a menu to change these during flagging. This is particularly useful
//for the in-row selections (extendchan and extendcorr).
//
// </etymology>

// <synopsis>
// 
// </synopsis>

// <example>
// <srcblock>
// ExtendFlagger eflg;
// eflg.attach(ms);
// eflg.setdata();
// eflg.setmanualflags();
// eflg.run()
// </srcblock>
// </example>

// <motivation>
//  It has long been on the users wish list to do extended flagging
// </motivation>

// <thrown>
//    <li>
//    <li>
// </thrown>

// <todo asof="$DATE:$">
//
// </todo>

class FlagIDS {
  public:
    FlagIDS() {
       bl = "";
       field = 0;
       time = "";
       spw = "";
       corr = "";
       chan = "";
    }

    String bl;
    uInt field;
    String time;
    String spw;
    String corr;
    String chan;

    FlagIDS &operator=(const FlagIDS &other) {
       bl = other.bl;
       field = other.field;
       time = other.time;
       spw = other.spw;
       corr = other.corr;
       chan = other.chan;
       return *this;
    }

   static int compareID (const void* val1, const void* val2);

   void show() {
       cout 
            << " time=" << time
            << " bl=" << bl 
            << " field=" << field
            << " spw=" << spw
            << " corr=" << corr
            << " chan=" << chan
            << endl;
    }
};

class FlagID {
  public:
    FlagID() {
       rowNum = 0;
       //uInt npts;
       ant1 = 0;
       ant2 = 0;
       field = 0;
       time = "";
       spw = 0;
       corr = "";
       chan = "";
       polid = 0;
    }

    uInt rowNum;
    //uInt npts;
    uInt ant1;
    uInt ant2;
    uInt field;
    String time;
    uInt spw;
    String corr;
    String chan;
    uInt polid;

    FlagID &operator=(const FlagID &other) {
       rowNum = other.rowNum; 
       //uInt npts;
       ant1 = other.ant1;
       ant2 = other.ant2;
       field = other.field;
       time = other.time;
       spw = other.spw;
       corr = other.corr;
       chan = other.chan;
       polid = other.polid;
       return *this;
    }

   static int compare (const void* val1, const void* val2);

   void show() {
       cout << "rowNum=" << rowNum
            //uInt npts;
            << " ant1=" << ant1
            << " ant2=" << ant2
            << " field=" << field
            << " time=" << time
            << " spw=" << spw
            << " corr=" << corr
            << " chan=" << chan
            << " polid=" << polid
            << endl;
    }
};

class ExtendFlagger 
{
   public:
      // Constructor
      ExtendFlagger();  
      ExtendFlagger(MeasurementSet& ms, const String& exchan,
                const String& excorr, const String& exant,
                const String& exspw, const String& time,
                const Vector<Vector<String> >& corrs, const Vector<Int>& chans);

      // Destructor
      virtual ~ExtendFlagger();

      // Operator=
      // Equate by reference.
      ExtendFlagger& operator=(const ExtendFlagger&){return *this;}

      Bool attach(MeasurementSet &ms);
      void detach();

      Bool initdata(const String& field = "", const String& spw = "", 
            const String& array = "", const String& feed = "", 
            const String& scan = "", const String& baseline = "", 
            const String& uvrange = "", const String& time = "", 
            const String& correlation = "", const String& intent = "");

      Bool setdata();

      Bool selectdata(Bool useoriginalms = True);

      Bool setmanualflags(Bool unflag = False,
			  Bool autocorr = False);

      Bool run(Bool trial = False, Bool reset = False);

      Bool extend(const Vector<FlagID>& flagids);
      Bool extend2(const Vector<FlagID>& flagids);

      void setField(const String& field);
      void setSpw(const String& spw);
      void setArray(const String& array);
      void setFeed(const String& feed);
      void setScan(const String& scan);
      void setBaseline(const String& baseline);
      void setUvrange(const String& uvrange);
      void setTime(const String& time);
      void setCorrelation(const String& correlation);
      void setIntent(const String& intent);
      void setUnflag(Bool unflg);

      void setExtendChan(const String& exchan);
      void setExtendCorr(const String& excorr);
      void setExtendSpw(const String& exspw);
      void setExtendTime(const String& extime);
      void setExtendAnt(const String& exant);
      void setExtend(const String& exchan, const String& excorr,
                     const String& exspw, const String& exant,
                     const String& extime,
                const Vector<Vector<String> >& corrs, const Vector<Int>& chans);
      void setAbleCorr(const Vector<Vector<String> >& corrName);
      void setChanNum(const Vector<Int>& chanNum);
      String halfExtendCorr(const String& corr, const uInt polid = 0);
      String ableSpw(const Int spw);
      void show();
   private:
      
      Flagger flagger;
      Bool useoriginalms;

      String field;
      String spw;
      String array;
      String feed;
      String scan;
      String baseline;
      String uvrange;
      String time;
      String correlation;
      String intent;
   
      String clipexpr;
      Vector<Double> cliprange;
      String clipcolumn;
      Bool outside;
      Bool channelavg;
      Double quackinterval;
      String opmode;

      Bool unflag;

      String extendChan;
      String extendCorr;
      String extendSpw;
      String extendAnt;
      String extendTime;

      Vector<Vector<String> > ableCorr;
      Vector<Int> chanNum;

};

} //# NAMESPACE CASA - END 

//#ifndef AIPS_NO_TEMPLATE_SRC
//#include <flagging/Flagging/ExtendFlagger.cc>
//#endif //# AIPS_NO_TEMPLATE_SRC
#endif

