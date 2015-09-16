//# MWCPannerTool.h: MultiWorldCanvas panning tool 
//# Copyright (C) 2000,2001,2002
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
//# You should have received a copy of the GNU Library General Public License
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

#ifndef TRIALDISPLAY_MWCRULERTOOL_H
#define TRIALDISPLAY_MWCRULERTOOL_H

#include <casa/aips.h>
#include <casa/Arrays/Vector.h>
#include <display/DisplayEvents/MultiWCTool.h>
#include <display/DisplayEvents/DTVisible.h>

namespace casa { //# NAMESPACE CASA - BEGIN

class WorldCanvas;

// <summary>
// Multi WorldCanvas event-based ruler line tool
// </summary>
//
// <use visibility=export>
//
// <reviewed reviewer="" date="yyyy/mm/dd" tests="" demos="">
// </reviewed>
//
// <prerequisites>
// </prerequisites>
//
// <etymology>
// MWCRulerlineTool stands for MultiWorldCanvas ruler line Tool
// </etymology>
//
// <synopsis>
// </synopsis>
//
// <example>
// </example>
//
// <motivation>
// A nice little tool to measure distances on the screen.
// </motivation>
//
// <todo asof="2011/06/10">
// Does not get the proper units if measuring distances on a frequency axis.
// </todo>

class MWCRulerlineTool : public MultiWCTool, public DTVisible {
  
 public:  
  // Constructor
  MWCRulerlineTool(Display::KeySym keysym = Display::K_Pointer_Button1,
		 Bool scrollingAllowed = True);
  
  // Destructor
  virtual ~MWCRulerlineTool();
  
  // Reset to non-showing, non-active.  Refreshes if necessary to erase
  // (unless skipRefresh==True).
  // (Does not unregister from WCs or disable future event handling).
  virtual void reset(Bool skipRefresh=False);

 protected:
  // Functions called by the base class mouse event handling operators -
  // these maintain the state of the pan vector and order it to be drawn
  // via refresh().
  // <group>
  virtual void keyPressed(const WCPositionEvent &ev);
  virtual void keyReleased(const WCPositionEvent &ev);
  virtual void moved(const WCMotionEvent &ev);
  // </group>

  // draw the distance vector on a PixelCanvas.  (To be called only by the
  // base class refresh event handler).
  virtual void draw(const WCRefreshEvent &);

 private:
  // Should we respond to mouse movement and button release?  Should
  // we draw?  Set when the button is pushed in one of the tool's WCs.
  Bool itsActive;

  // are the units in x and y identical?
  Bool itsEqualUnits;

  // pixel coordinates of the pan vector.  1 = anchor, 2 = new position.
  Int itsX1, itsY1, itsX2, itsY2, itsX3, itsY3;

  // axis index for RA and DEC
  Int itsRaIndex, itsDecIndex;
};


} //# NAMESPACE CASA - END

#endif

