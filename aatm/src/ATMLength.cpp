/*******************************************************************************
 * ALMA - Atacama Large Millimiter Array
 * (c) Institut de Radioastronomie Millimetrique, 2009
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 *
 * "@(#) $Id: ATMLength.cpp,v 1.5 2010/01/12 17:03:12 dbroguie Exp $"
 *
 * who       when      what
 * --------  --------  ----------------------------------------------
 * pardo     24/03/09  created
 */

#include <stdio.h>

#include "ATMLength.h"

using namespace std;

namespace atm {

  Length::Length() :
    valueIS_(0.0)
  {
  }
  
  Length::Length(double length) :
    valueIS_(length)
  {
  }
  
  Length::Length(double length, const string &units)
  {
    valueIS_ = sput(length, units);
  }
  
  Length::Length(const Length &length) :
    valueIS_(length.valueIS_)
  {
  }
  
  Length::~Length()
  {
  }
  
  double Length::sget(double value, const string &units)
  {
    if(units == "km" || units == "KM") {
      return 1.0E-3 * value;
    } else if(units == "m" || units == "M") {
      return value;
    } else if(units == "mm" || units == "MM") {
      return 1.0E+3 * value;
    } else if(units == "micron" || units == "MICRON") {
      return 1.0E+6 * value;
    } else if(units == "microns" || units == "MICRONS") {
      return 1.0E+6 * value;
    } else if(units == "nm" || units == "NM") {
      return 1.0E+9 * value;
    } else {
      return value;
    }
  }
  
  double Length::sput(double value, const string &units)
  {
    if(units == "km" || units == "KM") {
      return 1.0E+3 * value;
    } else if(units == "m" || units == "M") {
      return value;
    } else if(units == "mm" || units == "MM") {
      return 1.0E-3 * value;
    } else if(units == "micron" || units == "MICRON") {
      return 1.0E-6 * value;
    } else if(units == "nm" || units == "NM") {
      return 1.0E-9 * value;
    } else {
      return value;
    }
  }
  

/*  
string Length::get(const string &form, const string &units) const
  {
    char myString[18];

    sprintf(myString, "%f %s", get(units), units.c_str());

    
    return string(myString);
    } 
*/

  
} // namespace atm

