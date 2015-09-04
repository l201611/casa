
// -----------------------------------------------------------------------------

/*

CalStatsDerived.h

Description:
------------
This header file contains definitions for the classes derived from CalStats.

Classes:
--------
CalStatsReal  - This class feeds real data to the CalStats base class.
CalStatsAmp   - This class converts complex data to amplitudes and initializes
                the CalStats base class.
CalStatsPhase - This class converts complex data to phases and initializes the
                CalStats base class.

Inhertited classes:
-------------------
CalStats - This class calculates statistics on CASA caltables.

Modification history:
---------------------
2011 Nov 15 - Nick Elias, NRAO
              Initial version.
2012 Jan 25 - Nick Elias, NRAO
              Logging capability added.  Error checking added.

*/

// -----------------------------------------------------------------------------
// Start of define macro to prevent multiple loading
// -----------------------------------------------------------------------------

#ifndef CAL_STATS_DERIVED_H
#define CAL_STATS_DERIVED_H

// -----------------------------------------------------------------------------
// Includes
// -----------------------------------------------------------------------------

#define _USE_MATH_DEFINES
#include <cmath>

#include <casa/Arrays/ArrayMath.h>
#include <casa/Arrays/MaskedArray.h>

#include <calanalysis/CalAnalysis/CalStats.h>

// -----------------------------------------------------------------------------
// Start of casa namespace definitions
// -----------------------------------------------------------------------------

namespace casa {

// -----------------------------------------------------------------------------
// Start of CalStatsReal class definition
// -----------------------------------------------------------------------------

/*

CalStatsReal

Description:
------------
This class feeds real data to the CalStats base class.

Class public member functions:
------------------------------
CalStatsReal  - This class feeds real data to the CalStats base class.  It is
                primarily used for initial testing.
~CalStatsReal - This destructor deallocates the internal memory of an instance.

Modification history:
---------------------
2011 Dec 11 - Nick Elias, NRAO
              Initial version.  The public member functions are CalStatsReal()
              (generic) and ~CalStatsReal().

*/

// -----------------------------------------------------------------------------

class CalStatsReal : public CalStats {

  public:

    // Generic constructor
    CalStatsReal( const Cube<Double>& oValue, const Cube<Double>& oValueErr,
        const Cube<Bool>& oFlag, const Vector<String>& oFeed,
        const Vector<Double>& oFrequency, const Vector<Double>& oTime,
        const CalStats::AXIS& eAxisIterUserID );

    // Destructor
    ~CalStatsReal( void );

};

// -----------------------------------------------------------------------------
// End of CalStatsReal class definition
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Start of CalStatsAmp class definition
// -----------------------------------------------------------------------------

/*

CalStatsAmp

Description:
------------
This class converts complex data to amplitudes and initializes the CalStats base
class.

Class public member functions:
------------------------------
CalStatsAmp  - This generic constructor converts complex data to amplitudes and
               initializes the CalStats base class.  It is primarily used for
               initial testing.
~CalStatsAmp - This destructor deallocates the internal memory of an instance.

Class public static member functions:
-------------------------------------
norm - This member function normalizes the amplitudes and their errors.

Modification history:
---------------------
2011 Nov 15 - Nick Elias, NRAO
              Initial version.  The public member functions are CalStatsAmp()
              (generic) and ~CalStatsAmp().  The static member function is
              norm().

*/

// -----------------------------------------------------------------------------

class CalStatsAmp : public CalStats {

  public:

    // Generic constructor
    CalStatsAmp( const Cube<DComplex>& oValue, const Cube<Double>& oValueErr,
        const Cube<Bool>& oFlag, const Vector<String>& oFeed,
        const Vector<Double>& oFrequency, const Vector<Double>& oTime,
        const CalStats::AXIS& eAxisIterUserID, const Bool& bNorm );

    // Destructor
    ~CalStatsAmp( void );

    // Normalize member function
    static void norm( Vector<Double>& oAmp, Vector<Double>& oAmpErr,
        Vector<Bool>& oFlag );

};

// -----------------------------------------------------------------------------
// End of CalStatsAmp class definition
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Start of CalStatsPhase class definition
// -----------------------------------------------------------------------------

/*

CalStatsPhase

Description:
------------
This class converts complex data to phases and initializes the CalStats base
class.

Inhertited classes:
-------------------
CalStats - This class calculates statistics of new CASA caltables.

Class public member functions:
------------------------------
CalStatsPhase  - This generic constructor converts complex data to amplitudes
                 and initializes the CalStats base class.
~CalStatsPhase - This destructor deallocates the internal memory of an instance.

CalStatsPhase public static member functions:
---------------------------------------------
unwrapGD     - This member function unwraps the phases along the frequency axis
               with respect to the group delay.
unwrapSimple - This member function performs a simple unwrapping procedure for
               both frequency and temporal abscissae.

CalStatsPhase private static member functions:
----------------------------------------------
fringePacket2 - This member function forms the squared-amplitude fringe packet.

CalStatsPhase templated private static member functions:
--------------------------------------------------------
maxLocation - This member function finds the abscissa corresponding to the peak
              value of an ordinate vector.

Modification history:
---------------------
2011 Nov 15 - Nick Elias, NRAO
              Initial version created with public member functions are
              CalStatsPhase() and ~CalStatsPhase(); and public static member
              function is unwrap().
2012 Mar 27 - Nick Elias, NRAO
              Private static member functions fringePacket2() and maxLocation()
              added. Private static member variables NUM_ITER_UNWRAP and
              NEW_RANGE_FACTOR added.
2012 Mar 30 - Nick Elias, NRAO
              Public static member function unwrap() renamed to unwrapGD().
              Public static member function unwrapSimple() added.

*/

// -----------------------------------------------------------------------------

class CalStatsPhase : public CalStats {

  public:

    // Generic constructor
    CalStatsPhase( const Cube<DComplex>& oValue, const Cube<Double>& oValueErr,
        const Cube<Bool>& oFlag, const Vector<String>& oFeed,
        const Vector<Double>& oFrequency, const Vector<Double>& oTime,
        const CalStats::AXIS& eAxisIterUserID, const Bool& bUnwrap,
        const Double& dJumpMax );

    // Destructor
    ~CalStatsPhase( void );

    // Group-delay unwrapping member function
    static void unwrapGD( Vector<Double>& oPhase,
        const Vector<Double>& oFrequency, const Vector<Bool>& oFlag );

    // Simple phase unwrapping member function
    static void unwrapSimple( Vector<Double>& oPhase, const Double& dJumpMax,
        const Vector<Bool>& oFlag );

  private:

    // Form the squared-amplitude fringe packet
    static Vector<Double>& fringePacket2( const Vector<Double>& oPhase,
        const Vector<Double>& oFrequency, const Vector<Double>& oTime );

    // Find the abscissa corresponding to the peak value of an ordinate vector
    template <typename T> static Double& maxLocation( const Vector<T>& oAbs,
        const Vector<T>& oValue );

    // The maximum number of iterations for unwrapping
    static const uInt NUM_ITER_UNWRAP;

    // The new time range per unwrapping iteration is this factor times the
    // present time increment
    static const Double NEW_RANGE_FACTOR;

};

// -----------------------------------------------------------------------------
// End of CalStatsPhase class definition
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Start of CalStatsDerived::maxLocation<T> template private statistics member
// function
// -----------------------------------------------------------------------------

/*

CalStatsPhase::maxLocation<T>

Description:
------------
This member function finds the abscissa corresponding to the peak value of an
ordinate vector.

Inputs:
-------
oAbs   - This Vector<T>() instance contains the abscissae.
oValue - This Vector<T>() instance contains the ordinates.

Outputs:
--------
The reference to the Double variable containing the abscissa corresponding to
the peak value of the ordinate vector, returned via the function value.

Modification history:
---------------------
2012 Mar 27 - Nick Elias, NRAO
              Initial version.

*/

// -----------------------------------------------------------------------------

template <typename T>
Double& CalStatsPhase::maxLocation( const Vector<T>& oAbs,
    const Vector<T>& oValue ) {

  // Call the minMax() function and return the position of the maximum value

  Double dValMin = 0.0;
  Double dValMax = 0.0;

  IPosition oPosMin( 1, 0 );
  IPosition oPosMax( 1, 0 );

  minMax( dValMin, dValMax, oPosMin, oPosMax, oValue );

  Double* pdAbsMax = new Double;
  *pdAbsMax = oAbs(oPosMax);

  return( *pdAbsMax );

}

// -----------------------------------------------------------------------------
// End of CalStatsDerived::maxLocation<T> template private statistics member
// function
// -----------------------------------------------------------------------------

};

// -----------------------------------------------------------------------------
// End of casa namespace
// -----------------------------------------------------------------------------

#endif

// -----------------------------------------------------------------------------
// End of define macro to prevent multiple loading
// -----------------------------------------------------------------------------