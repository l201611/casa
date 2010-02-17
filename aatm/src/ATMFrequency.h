#ifndef _ATM_FREQUENCY_H
#define _ATM_FREQUENCY_H
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
 * "@(#) $Id: ATMFrequency.h,v 1.3 2009/05/04 21:30:54 dbroguie Exp $"
 *
 * who       when      what
 * --------  --------  ----------------------------------------------
 * pardo     24/03/09  created
 */

#ifndef __cplusplus
#error This is a C++ include file and cannot be used from plain C
#endif

#include <string>

using namespace std;

namespace atm
{

/*! \brief Defines a frequency value with units
 *
 *   Objects of type Frequency has a value in Hz (International System Units). The
 *   class Frequency allows clients to instantiate or retrieve Frequency objects with
 *   values specified in any units within a restricted set of predefined units, namely
 *   in THz, GHz, MHz, kHz or Hz. Hence units are handled only at the interface.
 *   If the units is not specified at this interface it is assumed to be Hz.
 */

class Frequency
{
public:
  /** Default constructor: Frequency value set to 0 Hz */
  Frequency();
  /** A full constructor: Frequency value + units. Valid units are THz [THZ] [thz], GHz [GHZ] [ghz], MHz [MHZ] [mhz], Hz [HZ] [hz] */
  Frequency(double frequency, const string &units);
  /** A full constructor: Frequency value in default units (International System: Hz) */
  Frequency(double frequency);
  /** Copy constructor */
  Frequency (const Frequency &frequency);

  /** Destructor */
  virtual ~Frequency();

  /** Accessor to get the numerical value of frequency (in International System units: Hz) */
  inline double get() const { return valueIS_; }
  /** Accessor to the frequency value in specified units. Implemented units are THz [THZ] [thz], GHz [GHZ] [ghz], MHz [MHZ] [mhz], Hz [HZ] [hz].
   *  If none of these implemented units is given, the SI value will be returned. */
  inline double get(const string &units) const { return sget( valueIS_, units); }
  /** Accessor to the frequency in specified units as a formatted string.
   *  Implemented units are THz [THZ] [thz], GHz [GHZ] [ghz], MHz [MHZ] [mhz], Hz [HZ] [hz].
   *  If none of these implemented units is given, the SI value will be returned. */
  string get(const string &form, const string &units) const;

  /** Operator "equal to a Frequency" */
  inline Frequency& operator=(const Frequency &rhs) { if(&rhs != this) valueIS_ = rhs.valueIS_; return *this; }
  /** Operator "equal to a double converted to Frequency in Hz" */
  inline Frequency& operator=(double rhs) { valueIS_ = rhs; return *this; }
  /** Operator "addition of frequencies" */
  inline Frequency operator+(const Frequency &rhs) { return Frequency(valueIS_ + rhs.get()); }
  /** Operator "subtraction of frequencies" */
  inline Frequency operator-(const Frequency &rhs) { return Frequency(valueIS_ - rhs.get()); }
  /* operator+= operator-= */
  /** Operator "multiplication of a frequency by a double" */
  inline Frequency operator*(double scf) { return Frequency(valueIS_ * scf); }
  /** Operator "multiplication of a frequency by a float" */
  inline Frequency operator*(float scf) { return Frequency(valueIS_ * (double) scf); }
  /** Operator "multiplication of a frequency by an int" */
  inline Frequency operator*(int scf) { return Frequency(valueIS_ * (double) scf); }
  /** Operator "multiplication of a frequency by an unsigned int" */
  inline Frequency operator*(unsigned int scf) { return Frequency(valueIS_ * (double) scf); }
  /** Operator "division of a frequency by a double" */
  inline Frequency operator/(double scf) { return Frequency(valueIS_ / scf); }
  /** Operator "division of a frequency by a float" */
  inline Frequency operator/(float scf) { return Frequency(valueIS_ / (double) scf); }
  /** Operator "division of a frequency by an int" */
  inline Frequency operator/(int scf) { return Frequency(valueIS_ / (double) scf); }
  /** Operator "division of a frequency by an unsigned int" */
  inline Frequency operator/(unsigned int scf) { return Frequency(valueIS_ / (double) scf); }
  /** Operator "comparator < for two frequencies" */
  inline bool operator<(const Frequency &rhs) const {return (valueIS_ < rhs.get()); }
  /** Operator "comparator > for two frequencies" */
  inline bool operator> (const Frequency &rhs) const {return (valueIS_ > rhs.get()); }
  /** Operator "comparator <= for two frequencies" */
  inline  bool operator<=(const Frequency &rhs) const {return (valueIS_ <= rhs.get()); }
  /** Operator "comparator >= for two frequencies" */
  inline bool operator>=(const Frequency &rhs) const {return (valueIS_ >= rhs.get()); }
  /** Operator "comparator == for two frequencies" */
  inline bool operator==(const Frequency &rhs) const {return (valueIS_ == rhs.get()); }
  /** Operator "comparator != for two frequencies" */
  inline bool operator!=(const Frequency &rhs) const {return (valueIS_ != rhs.get()); }

private:
  static double sget(double value, const string &units);
  static double sput(double value, const string &units);

private:
  double valueIS_;
}; // class Frequency

} // namespace atm

#endif /*!_ATM_FREQUENCY_H*/

