"""
<?xml version="1.0" encoding="UTF-8"?>
<?xml-stylesheet type="text/xsl" ?>
<casaxml xmlns="http://casa.nrao.edu/schema/psetTypes.html"
xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
xsi:schemaLocation="http://casa.nrao.edu/schema/casa.xsd
file:///opt/casa/code/xmlcasa/xml/casa.xsd">

 
        <tool name="coordsys" module="images">
<needs>measures</needs>
        <shortdescription>Operations on CoordinateSystems</shortdescription>
       

include "coordsys.g"



<keyword>coordinates</keyword>
<keyword>image</keyword>
<keyword>measures</keyword>
<keyword>quanta</keyword>
<code>
	<include>xmlcasa/images/coordsys_forward.h</include>
	<private>
		#include <xmlcasa/images/coordsys_private.h>
	</private>
</code>


<description>
\medskip
\noindent{\bf Summary}

A Coordsys \tool\ is used to store and manipulate a Coordinate System
(we will use the term `Coordinate System' interchangeably with `Coordsys
\tool'). A Coordinate System is a collection of coordinates, such as a
direction coordinate (E.g. RA/DEC), or a spectral coordinate (e.g. an
LSRK frequency).  

The main job of the Coordsys \tool\ is to convert between absolute pixel and world
(physical) coordinates.  It also supports relative pixel and world coordinates
(relative to reference location).

A Coordinate System is generally associated with an image (manipulated
via an <link anchor="images:image">Image</link> \tool) but can also exist in its own
right.  An image is basically just a regular lattice of pixels plus a
Coordinate System describing the mapping from pixel coordinate to world
(or physical) coordinate.

Each coordinate is associated with a number of axes.  For example, a
direction coordinate has two coupled axes; a longitude and a latitude. 
A spectral coordinate has one axis.  A linear coordinate can have an
arbitrary number of axes, but they are all uncoupled.   The Coordinate
System actually maintains two kinds of axes; pixel axes and world
axes.    

As well as the coordinates, there is some extra information stored in
the Coordinate System.  This consists of the telescope, the epoch (date
of observation), and the highly influential observer's name.   The
telescope (i.e. position on earth) and epoch are important if you want
to, say, change a spectral coordinate from LSRK to TOPO.  

For general discussion about celestial coordinate systems, see the
papers by Mark Calabretta and Eric Greisen.
Background on the WCS system and relevant papers (including the
papers published in 
\begin{verbatim}
A&A 2002, 1061-1075 and 1077-1122
\end{verbatim}
can be found 
\htmladdnormallink{here}{http://www.atnf.csiro.au/people/mark.calabretta/WCS}.
Note that the actual system implemented originally in \casa\ was
based on a 1996 draft of these papers. The final papers are being
implemented while new version of the defining library become
available.

\bigskip
\noindent {\label{COORDSYS:FORMATTING} \bf Coordinate formatting}

Many of the Coordsys \tool\ functions use a world coordinate value
as an argument.  This world value can be formatted in many ways.

Some functions (e.g.  <link anchor="images:coordsys.toworld.function">toworld</link>) have a function
argument called {\stfaf format} which takes a string.  This controls 
the format in which the coordinate is output and hence possibly input 
into some other function.

Possibilities for {\stfaf format} are :

\begin{itemize}

\item 'n' - means the world coordinate is given as a numeric vector (actually doubles).
The units are implicitly those returned by function <link anchor="images:coordsys.units.function">units</link>.

\item 'q' - means the world coordinate is given as a vector of quantities
(value and unit) - see the <link anchor="quanta">quanta</link> module.  If there
is only one axis (e.g. spectral coordinate), you will get
a single quantum only.

\item 'm' - means the world coordinate is given as a record
of measures - see the <link anchor="measures:measures">measures</link> module.

The record consists of fields named {\cf direction}, 
{\cf spectral}, {\cf stokes}, {\cf linear}, and {\cf tabular},
depending upon which coordinate types are present in the Coordinate System.

The {\cf direction} field holds a <link anchor="measures:measures.direction.function">direction</link>
measure.    

The {\cf spectral} field holds further subfields  {\cf frequency}, {\cf
radiovelocity}, {\cf opticalvelocity}, {\cf betavelocity}. The {\cf
frequency} subfield holds a
<link anchor="measures:measures.frequency.function">frequency</link> measure. The {\cf
radiovelocity} subfield holds a
<link anchor="measures:measures.doppler.function">doppler</link> measure using the radio
velocity definition. The {\cf opticalvelocity} subfield holds a
<link anchor="measures:measures.doppler.function">doppler</link> measure using the optical
velocity definition. The {\cf betavelocity} subfield holds a
<link anchor="measures:measures.doppler.function">doppler</link> measure using the true or
beta velocity definition.

The {\cf stokes} field just holds a string giving the Stokes type
(not a real measure).  

The {\cf linear} and {\cf tabular} fields hold a vector of quanta (not a real measure).

\item 's' - means the the world coordinate is given as a vector of formatted strings

\end{itemize}

You can give a combination of one or more of the allowed letters
when using the {\stfaf format} argument. <!-- If you supply one letter the
coordinate is as described above. If you supply more than one letter, -->
The coordinate is given as a record, with possible fields 'numeric',
'quantity', 'measure' and 'string' where each of these fields is given
as described above.

\bigskip There are functions <link
anchor="images:coordsys.torel.function">torel</link> and <link
anchor="images:coordsys.toabs.function">toabs</link> used to
inter-convert between absolute and relative world and pixel
coordinates.  These functions have an argument {\stfaf isworld}
whereby you can specify whether the coordinate is a pixel coordinate
or a world coordinate.  In general, you should not need to use this
argument because any coordinate variable generated by Coordsys \tool\
functions 'knows' whether it is absolute or relative, world or
pixel. <!-- (uses a \glish\ attribute) -->.  However, you may be
inputting a coordinate variable which you have generated in some other
way, and then you may need this.

\bigskip
\noindent {\bf Stokes Coordinates}

Stokes axes don't fit very well into our Coordinate model since
they are not interpolatable.   The alternative to having a Stokes
Coordinate is having a Stokes pixel type (like double, complex).  Both
have their good and bad points.  We have chosen to use a Stokes
Coordinate.  

With the Stokes Coordinate, any absolute pixel coordinate value must be in the
range of 1 to $nStokes$, where $nStokes$ is the number of Stokes types in
the Coordinate.

We define relative world coordinates for a Stokes axis to be the same as absolute
world coordinates (it makes no sense to think of a relative value
$XY - XX$ say).

You can use the specialized functions 
<link anchor="images:coordsys.stokes.function">stokes</link>  and
<link anchor="images:coordsys.setstokes.function">setstokes</link>  to recover and set new
Stokes values in the Stokes Coordinate.

\bigskip
\noindent {\label{COORDSYS:PWAXES} \bf World and Pixel axes}

The Coordinate System maintains what it calls pixel and world axes.  The
pixel axis is associated with, say, the axes of a lattice of pixels. 
The world axes describe notional world axes, generally in the same order
 as the pixel axes.  However, they may be different.  Imagine that a 3-D
image is collapsed along one axis.  The resultant image has 2 pixel
axes.  However, we can maintain the world coordinate for the collapsed
axis (so we know the coordinate value still).  Thus we have three world
axes and two pixel axes.  It is also possible for the C++ programmer to
reorder these pixel and world axes.  However, this is strongly
discouraged, and you should never actually encounter a situation where
the pixel and world axes are in different orders, but you may encounter
cases where the number of world and pixela axes is different.

For those of us (\casa\ programmers) writing robust scripts, we must
account for these possibilities, although the user really shouldn't
bother.  Thus, the {\stfaf pixel} and {\stfaf world} vectors return the
pixel and world axes of the found coordinate.

The functions 
<link anchor="images:coordsys.referencevalue.function">referencevalue</link>,
<link anchor="images:coordsys.increment.function">increment</link>,
<link anchor="images:coordsys.units.function">units</link>, and
<link anchor="images:coordsys.names.function">names</link> return their vectors in world axis
order.  However, function
<link anchor="images:coordsys.referencepixel.function">referencepixel</link> returns in pixel
axis order (and the world vectors might have more values than the
referencepixel vector).

\bigskip
{\bf Overview of Coordsys tool functions}

\begin{itemize}

\item {\bf Get/set - } Functions to get and set various items within the
Coordinate System are 

\begin{itemize}

<!-- "functions parentname and setparentname are not documented as the user should not need them and will just confuse them" -->

\item <link anchor="images:coordsys.referencepixel.function">referencepixel</link> - get the reference pixel

\item <link anchor="images:coordsys.setreferencepixel.function">setreferencepixel</link> - set the reference pixel

\item <link anchor="images:coordsys.referencevalue.function">referencevalue</link> - get the reference value

\item <link anchor="images:coordsys.setreferencevalue.function">setreferencevalue</link> - set the reference value

\item <link anchor="images:coordsys.setreferencelocation.function">setreferencelocation</link> - Set reference pixel and value
to these values

\item <link anchor="images:coordsys.increment.function">increment</link> - get axis increments

\item <link anchor="images:coordsys.setincrement.function">setincrement</link> - set axis increments

\item <link anchor="images:coordsys.lineartransform.function">lineartransform</link> - get linear transform

\item <link anchor="images:coordsys.setlineartransform.function">setlineartransform</link> - set linear transform

\item <link anchor="images:coordsys.names.function">names</link> - get axis names

\item <link anchor="images:coordsys.setnames.function">setnames</link> - set axis names

\item <link anchor="images:coordsys.units.function">units</link> - get axis units

\item <link anchor="images:coordsys.setunits.function">setunits</link> - set axis units

\item <link anchor="images:coordsys.stokes.function">stokes</link> - get Stokes values

\item <link anchor="images:coordsys.setdirection.function">setdirection</link> - set Direction coordinate values

\item <link anchor="images:coordsys.setstokes.function">setstokes</link> - set Stokes values

\item <link anchor="images:coordsys.setspectral.function">setspectral</link> - set Spectral coordinate tabular values

\item <link anchor="images:coordsys.settabular.function">settabular</link> - set Tabular coordinate tabular values

\item <link anchor="images:coordsys.projection.function">projection</link> - get direction coordinate projection

\item <link anchor="images:coordsys.setprojection.function">setprojection</link> - set direction coordinate projection

\item <link anchor="images:coordsys.referencecode.function">referencecode</link> - get reference codes

\item <link anchor="images:coordsys.setreferencecode.function">setreferencecode</link> - set reference codes

\item <link anchor="images:coordsys.restfrequency.function">restfrequency</link> - get the spectral coordinate rest frequency

\item <link anchor="images:coordsys.setrestfrequency.function">setrestfrequency</link> - set the spectral coordinate rest frequency

\item <link anchor="images:coordsys.epoch.function">epoch</link> - get the epoch of observation

\item <link anchor="images:coordsys.setepoch.function">setepoch</link> - set the epoch of observation

\item <link anchor="images:coordsys.telescope.function">telescope</link> - get the telecope of the observation

\item <link anchor="images:coordsys.settelescope.function">settelescope</link> - set the telecope of the observation

\item <link anchor="images:coordsys.observer.function">observer</link> - get observer name

\item <link anchor="images:coordsys.setobserver.function">setobserver</link> - set observer name

\end{itemize}

\item {\bf Utility - }  There is a range of utility services available through the functions

\begin{itemize}

<!-- "Multiple coordinates of the same type not well supported so not advertised yet." -->


\item <link anchor="images:coordsys.axesmap.function">axesmap</link> - get mapping between
pixel and world axes order

\item <link anchor="images:coordsys.axiscoordinatetypes.function">axiscoordinatetypes</link> - get type
of coordinate for each axis

\item <link anchor="images:coordsys.coordinatetype.function">coordinatetype</link> - get type
of coordinates

\item <link anchor="images:coordsys.copy.function">copy</link> - make a copy of this tool

\item <link anchor="images:coordsys.done.function">done</link> - destroy this \tool\

\item <link anchor="images:coordsys.findaxis.function">findaxis</link> - find specified axis (by number) in
coordinate system

\item <link anchor="images:coordsys.findcoordinate.function">findcoordinate</link> - find specified (by number) coordinate 

\item <link anchor="images:coordsys.fromrecord.function">fromrecord</link> - set Coordinate System from a casapy record

\item <link anchor="images:coordsys.id.function">id</link> - get the fundamental identifier of this \tool\

\item <link anchor="images:coordsys.naxes.function">naxes</link> - get number of axes

\item <link anchor="images:coordsys.ncoordinates.function">ncoordinates</link> - get the number of coordinates



\item <link anchor="images:coordsys.reorder.function">reorder</link> - reorder coordinates

\item <link anchor="images:coordsys.summary.function">summary</link> - summarize the Coordinate System

\item <link anchor="images:coordsys.torecord.function">torecord</link> - Convert a Coordinate SYstem to a casapy record

\item <link anchor="images:coordsys.type.function">type</link> - the type of this \tool\

\end{itemize}

\item {\bf Coordinate conversion}

\begin{itemize}

\item <link anchor="images:coordsys.convert.function">convert</link> - Convert one numeric coordinate with mixed input and output formats (abs/rel/world/pixel) 

\item <link anchor="images:coordsys.toabs.function">toabs</link> - Convert a relative coordinate to an absolute coordinate

\item <link anchor="images:coordsys.topixel.function">topixel</link> - Convert from absolute world coordinate to absolute pixel coordinate

\item <link anchor="images:coordsys.torel.function">torel</link> - Convert an absolute coordinate to a relative coordinate

\item <link anchor="images:coordsys.toworld.function">toworld</link> - Convert from an absolute pixel coordinate to an absolute world coordinate

\item <link anchor="images:coordsys.convertmany.function">convertmany</link> - Convert many numeric coordinates with mixed input and output formats (abs/rel/world/pixel) 

\item <link anchor="images:coordsys.toabsmany.function">toabsmany</link> - Convert many relative coordinates to absolute coordinates

\item <link anchor="images:coordsys.topixelmany.function">topixelmany</link> - Convert many absolute world coordinates to absolute pixel coordinates

\item <link anchor="images:coordsys.torelmany.function">torelmany</link> - Convert many absolute coordinates to relative coordinates

\item <link anchor="images:coordsys.toworldmany.function">toworldmany</link> - Convert many absolute pixel coordinates to absolute world coordinates

\item <link anchor="images:coordsys.frequencytovelocity.function">frequencytovelocity</link> - Convert from frequency to velocity

\item <link anchor="images:coordsys.frequencytofrequency.function">frequencytofrequency</link> - Convert from frequency to frequency via a velocity offset

\item <link anchor="images:coordsys.velocitytofrequency.function">velocitytofrequency</link> - Convert from velocity to frequency

\item <link anchor="images:coordsys.setconversiontype.function">setconversiontype</link> - Set extra reference frame conversion layer

\item <link anchor="images:coordsys.conversiontype.function">conversiontype</link> - Recover extra reference frame conversion types

\end{itemize}

\item {\bf Tests - }  

\begin{itemize}

\item <link anchor="images:coordsystest">coordsystest</link> - Run test suite for Coordsys \tool\

\end{itemize}

\end{itemize}

<!--
\bigskip
\noindent{\bf Events}

There are no events emitted or acted upon by this \tool.
-->

\goodbreak\bigskip
\noindent {\bf Example}

<example> 
<!--include 'coordsys.g'-->
\begin{verbatim}
"""
#
print "\t----\t Intro Ex 1 \t----"
csys = cs.newcoordsys(direction=T, linear=2)
csys.summary()
#
"""
\end{verbatim}
</example>
</description>
 
   <method type="function" name="newcoordsys">
   <shortdescription>Create a non-default coordsys tool</shortdescription>
   
<input>  
  
	<param xsi:type="bool" direction="in" name="direction">
     <description>Make a direction coordinate ?</description>
     <value>false</value>
     </param>  
  
     <param xsi:type="bool" direction="in" name="spectral">
     <description>Make a spectral coordinate ?</description>
     <value>false</value>
     </param>
  
     <param xsi:type="stringArray" direction="in" name="stokes">
     <description>Make a Stokes coordinate with these Stokes</description>
     <value>I</value>
     <value>Q</value>
     <value>U</value>
     <value>V</value>
     <value>XX</value>
     <value>YY</value>
     <value>XY</value>
     <value>YX</value>
     <value>RR</value>
     <value>LL</value>
     <value>RL</value>
     <value>LR</value>
     <value></value>
     </param>
  
     <param xsi:type="int" direction="in" name="linear">
     <description>Make a linear coordinate with this many axes</description>
     <value>0</value>
     </param>
  
     <param xsi:type="bool" direction="in" name="tabular">
     <description>Make a tabular coordinate</description>
     <value>false</value>
     </param>
</input>    
<returns xsi:type="casacoordsys"/>

<description>

By default, this constructor makes an empty Coordsys \tool.  You can ask
it to include various sorts of coordinates through the arguments. 
Except for Stokes, you don't have any control over the coordinate
contents (e.g.  reference value etc.) it does make for you on request. 
But you can edit the Coordinate System after creation if you wish. 

If you wish to make a Stokes coordinate, then you assign
{\stfaf stokes} to a string (or a vector of strings) saying
which Stokes you want.  \casa\ allows rather
a lot of potential Stokes types.  

Probably most useful is some combination of the
basic I, Q, U, V, XX, YY, XY, YX, RR, LL, RL, and LR.

However, a more esoteric choice is also possible:
RX, RY, LX, LY, XR, XL, YR, YL (these are mixed
linear and circular),  PP, PQ, QP, QQ (general quasi-orthogonal correlation products)
RCircular, LCircular, Linear  (single dish polarization types).

You can also specify some polarization `Stokes' types:
Ptotal (Polarized intensity ($(Q^2+U^2+V^2)^{1/2}$),
Plinear (Linearly Polarized intensity ($(Q^2+U^2)^{1/2}$),
PFtotal (Polarization Fraction (Ptotal/I)), 
PFlinear  (Linear Polarization Fraction (Plinear/I)), and
Pangle  (Linear Polarization Angle ($0.5~arctan(U/Q)$ in radians)).

Probably you will find the more unusual types aren't fully
supported throughout the system.

You can make a LinearCoordinate with as many uncoupled axes as you like.
Thus, {\stfaf linear=2} makes one LinearCoordinate with 2 axes (think
of it like a DirectionCoordinate which also has 2 axes [but coupled in
this case], a longitude and a latitude).

If you make a TabularCoordinate, it is linear to start with.
You can change it to a non-linear one by providing
a list of pixel and world values to function
<link anchor="images:coordsys.settabular.function">settabular</link>.

</description>

<example>
\begin{verbatim}
"""
#
print "\t----\t newcoordsys Ex 1 \t----"
cs1=cs.newcoordsys()
print 'ncoordinates =',cs1.ncoordinates()
#0
cs1.done()
#True
cs2=cs.newcoordsys(direction=T,stokes=['I','V'])
print 'ncoordinates =',cs2.ncoordinates()
#2L
print cs2.coordinatetype()
#['Direction', 'Stokes']
cs2.summary()
#
"""
\end{verbatim}
The second Coordinate System contains a direction coordinate
and a Stokes coordinate.   This means that there are three `axes'
associated with the 2 coordinates.

</example>
</method>

   <method type="function" name="addcoordinate">
   <shortdescription>Add default coordinates.  (For assay testing only.)</shortdescription>
   
<input>  
  
	<param xsi:type="bool" direction="in" name="direction">
     <description>Add a direction coordinate ?</description>
     <value>false</value>
     </param>  
  
     <param xsi:type="bool" direction="in" name="spectral">
     <description>Add a spectral coordinate ?</description>
     <value>false</value>
     </param>
  
     <param xsi:type="stringArray" direction="in" name="stokes">
     <description>Add a Stokes coordinate with these Stokes</description>
     <value>I</value>
     <value>Q</value>
     <value>U</value>
     <value>V</value>
     <value>XX</value>
     <value>YY</value>
     <value>XY</value>
     <value>YX</value>
     <value>RR</value>
     <value>LL</value>
     <value>RL</value>
     <value>LR</value>
     <value></value>
     </param>
  
     <param xsi:type="int" direction="in" name="linear">
     <description>Add a linear coordinate with this many axes</description>
     <value>0</value>
     </param>
  
     <param xsi:type="bool" direction="in" name="tabular">
     <description>Add a tabular coordinate</description>
     <value>false</value>
     </param>
</input>    
<returns xsi:type="bool"/>

<description>
Add default coordinates of the specified types.  This function allows
multiple coordinates of the same type which are not well supported.
Use only for assay tests.
</description>

<example>
\begin{verbatim}
"""
#
print "\t----\t addcoordinate Ex 1 \t----"
mycs=cs.newcoordsys()
mycs.addcoordinate(direction=T)
mycs.done()
#
"""
\end{verbatim}

</example>
</method>

 
   <method type="function" name="axesmap">
   <shortdescription>Find mapping between world and pixel axes</shortdescription>
   
<input>
  
     <param xsi:type="bool" direction="in" name="toworld">
     <description>Map from pixel to world ?</description>
     <value>true</value>
     </param>
</input>

<returns xsi:type="intArray"/>
<description>

This function returns a vector describing the mapping from pixel to
world or world to pixel axes.  It is not for general user use.

See the \htmlref{discussion}{COORDSYS:PWAXES} about pixel and world axis
ordering.  Generally they will be in the same order.

</description>
<example>
\begin{verbatim}
"""
#
print "\t----\t axesmap Ex 1 \t----"
csys = cs.newcoordsys(direction=T, spectral=T)
csys.axesmap(T);
#[1L, 2L, 3L]
csys.axesmap(F);
#[1L, 2L, 3L]
#
"""
\end{verbatim}
</example>
</method>

 
   <method type="function" name="axiscoordinatetypes">
   <shortdescription>Return types of coordinates for each axis</shortdescription>
   
<input>
  
     <param xsi:type="bool" direction="in" name="world">
     <description>World or pixel axes ?</description>
     <value>true</value>
     </param>
</input>

<returns xsi:type="stringArray"/>
<description>

This function <!--(short-hand name {\stff act})--> returns a vector string 
giving the coordinate type for each axis (world or pixel)
in the Coordinate System.

See the \htmlref{discussion}{COORDSYS:PWAXES} about pixel and world axis
ordering.

</description>

<example>
\begin{verbatim}
"""
#
print "\t----\t axiscoordinatetypes Ex 1 \t----"
csys=cs.newcoordsys(direction=T,spectral=T)
csys.axiscoordinatetypes()
#['Direction', 'Direction', 'Spectral']
#
"""
\end{verbatim}
</example>
</method>

 
   <method type="function" name="conversiontype">
   <shortdescription>Get extra reference conversion layer</shortdescription>
   
<input>
  
     <param xsi:type="string" direction="in" name="type">
	     <description>Coordinate type, direction, spectral</description>
     <value>direction</value>
     </param>
</input>

<returns xsi:type="string"/>

<description>

Some coordinates contain a reference code.  Examples of reference codes
are B1950 and J2000 for direction coordinates, or LSRK and BARY for
spectral coordinates.  When you do conversions between pixel and world
coordinate, the coordinates are in the reference frame corresponding to
these codes.   

Function  <link anchor="images:coordsys.setconversiontype.function">setconversiontype</link>
allows you to specify a different reference frame
which is used when converting between world and pixel coordinate.

This function allows you to recover those conversion types.  If no extra
conversion layer has been set, you get back the native reference types.

<example>
\begin{verbatim}
"""
#
print "\t----\t conversiontype Ex 1 \t----"
csys = cs.newcoordsys(direction=T, spectral=T)
print csys.conversiontype (type='direction'), ' ', csys.conversiontype (type='spectral')
#J2000   LSRK
csys.setconversiontype (direction='GALACTIC', spectral='BARY')
print csys.conversiontype (type='direction'), ' ', csys.conversiontype (type='spectral')
#GALACTIC   BARY
#
"""
\end{verbatim}
</example>
</description>
</method>

 
   <method type="function" name="convert">
   <shortdescription>Convert a numeric mixed coordinate </shortdescription>
   
<input>

     <param xsi:type="doubleArray" direction="in" name="coordin">
     <description>Input coordinate, as a numeric vector</description>
     </param>

     <param xsi:type="boolArray" direction="in" name="absin">
     <description>Are input coordinate elements absolute ?</description>
     <value>true</value>
     </param>

     <param xsi:type="string" direction="in" name="dopplerin">
     <description>Input doppler type for velocities</description>
     <value>radio</value>
     </param>

     <param xsi:type="stringArray" direction="in" name="unitsin">
     <description>Input units, string vector</description>
     <value>Native</value>
     </param>

     <param xsi:type="boolArray" direction="in" name="absout">
     <description>Are output coordinate elements absolute ?</description>
     <value>true</value>
     </param>

     <param xsi:type="string" direction="in" name="dopplerout">
     <description>Output doppler type for velocities</description>
     <value>radio</value>
     </param>

     <param xsi:type="stringArray" direction="in" name="unitsout">
     <description>Output units</description>
     <value>Native</value>
     </param>

     <param xsi:type="intArray" direction="in" name="shape">
	     <description>Image shape, integer vector</description>
	     <value>-1</value>
     </param>
</input>
<returns xsi:type="doubleArray"/>

<description>

This function converts between mixed pixel/world/abs/rel numeric
coordinates.  The input and output coordinates are specified via a 
numeric vector giving coordinate values, a string vector giving units, a
boolean vector specifying whether the coordinate is absolute or relative
(to the reference pixel) and doppler strings specifying the doppler
convention for velocities.

The units string may include {\cf pix} for pixel coordinates and
velocity units (i.e. any unit consistent with {\cf m/s}).

The allowed doppler strings and definition are described
in function <link anchor="images:image.summary.function">summary</link>.

The {\stfaf shape} argument is optional.  If your Coordinate
System is from an image, then assign the image shape to this
argument.  It is used only when making mixed (pixel/world) conversions
for Direction Coordinates to resolve ambiguity.

The example clarifies the use of this function.

</description>

<example>
In this example we convert from a vector of absolute pixels
to a mixture of pixel/world and abs/rel.
\begin{verbatim}
"""
#
print "\t----\t convert Ex 1 \t----"
csys=cs.newcoordsys(direction=T, spectral=T)    # 3 axes
cout=csys.convert(coordin=[10,20,30],absin=[T,T,T],
                  unitsin=["pix","pix","pix"],
                  absout=[T,F,T], dopplerout='optical',
                  unitsout=["pix","arcsec","km/s"])
print cout
#[10.0, 1140.0058038878046, 1139.1354056919731]
#
"""
\end{verbatim}
</example>

</method>

 
   <method type="function" name="convertmany">
   <shortdescription>Convert many numeric mixed coordinates</shortdescription>
   
<input>

     <param xsi:type="any" direction="in" name="coordin">
            <any type="variant"/>
     <description>Input coordinate, numeric matrix</description>
     </param>

     <param xsi:type="boolArray" direction="in" name="absin">
     <description>Are input coordinate elements absolute ?</description>
     <value>true</value>
     </param>

     <param xsi:type="string" direction="in" name="dopplerin">
     <description>Input doppler type for velocities</description>
     <value>radio</value>
     </param>

     <param xsi:type="stringArray" direction="in" name="unitsin">
     <description>Input units, string vector</description>
     <value>Native</value>
     </param>

     <param xsi:type="boolArray" direction="in" name="absout">
     <description>Are output coordinate elements absolute ?</description>
     <value>true</value>
     </param>

     <param xsi:type="string" direction="in" name="dopplerout">
     <description>Output doppler type for velocities</description>
     <value>radio</value>
     </param>

     <param xsi:type="stringArray" direction="in" name="unitsout">
     <description>Output units</description>
     <value>Native</value>
     </param>

     <param xsi:type="intArray" direction="in" name="shape">
     <description>Image shape, integer array</description>
     <value>-1</value>
     </param>
</input>
<returns xsi:type="any"><any type="variant"/></returns>

<description>

This function converts between many mixed pixel/world/abs/rel numeric
coordinates.   See function <link anchor="images:coordsys.convert.function">convert</link>
for more information.

The only diffference with that function is that you
provide a matrix holding many coordinates to convert
and a matrix of many converted coordinates is returned.

</description>

<example>
\begin{verbatim}
"""
#
print "\t----\t convertmany Ex 1 \t----"
csys = cs.newcoordsys(direction=T, spectral=T)    # 3 axes
# absolute pixel coordinates; 10 conversions each of length 3; spectral
cin=[(15, 15, 15, 15, 15, 15, 15, 15, 15, 15),  # pixel runs from 1 to 10
     (20, 20, 20, 20, 20, 20, 20, 20, 20, 20),
     ( 1,  2,  3,  4,  5,  6,  7,  8,  9, 10)]
cout = csys.convertmany (coordin=cin,     
                         absin=[T,T,T],
                         unitsin=["pix","pix","pix"],
                         absout=[T,F,T],
                         dopplerout='optical',
                         unitsout=["pix","deg","km/s"]);
print cout
#[(15.0, 15.0, 15.0, 15.0, 15.0, 15.0, 15.0, 15.0, 15.0, 15.0),
# (0.31666827885771637, 0.31666827885771637, 0.31666827885771637,
#  0.31666827885771637, 0.31666827885771637, 0.31666827885771637,
#  0.31666827885771637, 0.31666827885771637, 0.31666827885771637,
#  0.31666827885771637),
# (1145.3029083129913, 1145.0902316004676, 1144.8775551885467,
#  1144.6648790772279, 1144.4522032665102, 1144.2395277563601,
#  1144.0268525468437, 1143.8141776379266, 1143.6015030296085,
#  1143.3888287218554)]
#
"""
\end{verbatim}
</example>

</method>

 
   <method type="function" name="coordinatetype">
   <shortdescription>Return type of specified coordinate</shortdescription>
   
<input>
  
     <param xsi:type="int" direction="in" name="which">
     <description>Which coordinate ? (0-rel)</description>
     <value>-1</value>
     </param>
</input>

<returns xsi:type="stringArray"/>
<description>

This function <!--(short-hand name {\stff ct})--> returns a string describing
the type of the specified coordinate.  If {\stfaf which=unset} the types
for all coordinates are returned.

Possible output values are 'Direction', 'Spectral', 'Stokes', 'Linear', and
'Tabular'

</description>
<example>
\begin{verbatim}
"""
#
print "\t----\t coordinatetype Ex 1 \t----"
csys = cs.newcoordsys(direction=T, spectral=T)
csys.coordinatetype(0)
#'Direction'
cs.coordinatetype()
#['Direction', 'Spectral']
#
"""
\end{verbatim}
</example>
</method>

 
   <method type="function" name="copy">
   <shortdescription>Copy this Coordsys tool</shortdescription>
   
<returns xsi:type="casacoordsys"/>
<description>

<!--\glish\ uses copy-on-write semantics.  This sometimes
means that it maintains a reference between variables
that you would rather it didn't.-->

This function returns a copy, not a reference, of the Coordsys \tool.
It is your responsibility to call the {\stff done} function
on the new \tool.

</description>
<example>
\begin{verbatim}
"""
#
print "\t----\t copy Ex 1 \t----"
cs1 = cs.newcoordsys(direction=T, spectral=T)
cs2 = cs1         # Reference
print cs1, cs2
cs1.summary()
cs2.summary()
cs1.done()        # done invokes default coordsys tool
cs1.summary()
cs2.summary()     # cs2 gets doned when cs1 does
cs1 = cs.newcoordsys(direction=T, spectral=T)
cs2 = cs1.copy()  # Copy
cs1.done()
cs1.summary()     # cs1 is default coordsys tool
cs2.summary()     # cs2 is still viable
cs2.done()
cs2.summary()     # Now it's done (done just invokes default constructor)
#
"""
\end{verbatim}
</example>
</method>

 
   <method type="function" name="done">
   <shortdescription>Destroy this Coordsys tool, restore default tool</shortdescription>
   
<returns xsi:type="bool"/>
<description>

If you no longer need to use a Coordsys \tool calling this function
will free up its resources and restore the default coordsys tool. <!--  That
is, it destroys the \tool.  You can no
longer call any functions on the \tool\ after it has been {\stff done}. -->

</description>
<example>
\begin{verbatim}
"""
#
print "\t----\t done Ex 1 \t----"
csys = cs.newcoordsys(direction=T, spectral=T)
csys.done()
print csys.torecord()            # default tool
#
"""
\end{verbatim}
</example>
</method>

 
   <method type="function" name="epoch">
   <shortdescription>Return the epoch</shortdescription>
   
<returns xsi:type="any"><any type="record"/></returns>

<description>

This function <!--(short-hand name {\stff e})--> returns the epoch of the observation as a
<link anchor="measures:measures.epoch.function">Measure</link>.  <!-- You can format it with
<link anchor="quanta:quanta">quanta</link> -->

</description>
<example>
\begin{verbatim}
"""
#
print "\t----\t epoch Ex 1 \t----"
csys = cs.newcoordsys()
ep = csys.epoch()
print ep
#{'type': 'epoch', 'm0': {'value': 54151.96481085648, 'unit': 'd'}, 'refer': 'UTC'}
time = me.getvalue(ep)           # Extract time with measures
print time
#{'m0': {'value': 54151.96481085648, 'unit': 'd'}}
qa.time(time)                    # Format with quanta
#'23:09:20'
#
"""
\end{verbatim}
</example>
</method>

 
   <method type="function" name="findaxis">
   <shortdescription>Find specified axis in coordinate system</shortdescription>
   
<output>  
  
     <param xsi:type="int" direction="out" name="coordinate">
     <description>Coordinate number</description>
     </param>
  
     <param xsi:type="int" direction="out" name="axisincoordinate">
     <description>Axis in the coordinate</description>
     </param>
</output>  
  
<input>  
     <param xsi:type="bool" direction="in" name="world">
     <description>is axis a world or pixel axis ?</description>
     <value>true</value>
     </param>
  
     <param xsi:type="int" direction="in" name="axis">
     <description>Axis in coordinate system</description>
     <value>0</value>
     </param>
</input>    
<returns xsi:type="bool"/>
  
<description>

This function <!--(short-hand name {\stf fa})--> finds the specified axis in
the Coordinate System. If the axis does not exist, it returns F. <!-- (not
fail).-->

</description>

<example>
\begin{verbatim}
"""
#
print "\t----\t findaxis Ex 1 \t----"
csys=cs.newcoordsys(direction=T, linear=2)          # RA/DEC/Lin1/Lin2
rtn=csys.findaxis(T,1)                              # DEC
rtn
#{'axisincoordinate': 1L, 'coordinate': 0L, 'return': True}
rtn = csys.findaxis(T,2)                            # Lin1
rtn
#{'axisincoordinate': 0L, 'coordinate': 1L, 'return': True}
#
"""
\end{verbatim}

In these examples, the Coordinate System has 2 coordinates and 4 axes
(0-rel, both world and pixel the same).  The first example finds the
DEC axis (coordinate system axis 1) to be the second axis of the
Direction Coordinate (coordinate 0). The second example finds the
first linear axis (coordinate system axis 2) to be the first axis of
the Linear Coordinate (coordinate 1).

</example>
</method>

 
   <method type="function" name="findcoordinate">
   <shortdescription>Find axes of specified coordinate</shortdescription>
   
<output>  
  
     <param xsi:type="intArray" direction="out" name="pixel">
     <description>Pixel axes</description>
     </param>
  
     <param xsi:type="intArray" direction="out" name="world">
     <description>World axes</description>
     </param>
</output>  
  
<input>  
     <param xsi:type="string" direction="in" name="type">
     <description>Type of coordinate to find: direction, stokes, spectral, linear, or tabular</description>
     <value>direction</value>
     </param>
  
     <param xsi:type="int" direction="in" name="which">
     <description>Which coordinate if more than one</description>
     <value>0</value>
     </param>
</input>    
<returns xsi:type="bool"/>
  
<description>

This function <!--(short-hand name {\stf fc})--> finds the axes in the
Coordinate System for the specified coordinate (minimum match is active
for argument {\stfaf type}).  By default it finds the first coordinate,
but if there is more than one (can happen for linear coordinates), you
can specify which.  If the coordinate does not exist, it returns F. <!-- (not
fail).-->

See also the function <link anchor="images:coordsys.axesmap.function">axesmap</link>
which returns the mapping between pixel and world axes.

</description>

<example>
\begin{verbatim}
"""
#
print "\t----\t findcoordinate Ex 1 \t----"
csys=cs.newcoordsys(direction=T)
rtn=cs.findcoordinate('direction')
print rtn
#{'world': [0L, 1L], 'return': True, 'pixel': [0L, 1L]}
print 'pixel, world axes =', rtn['pixel'], rtn['world']
#pixel, world axes = [0 1] [0 1]
#
"""
\end{verbatim}
</example>
</method>

 
   <method type="function" name="frequencytofrequency">
   <shortdescription>Convert frequency to frequency with an offset</shortdescription>
   
<input>  
  
     <param xsi:type="doubleArray" direction="in" name="value">
     <description>Frequency to convert</description>
     </param>
  
     <param xsi:type="string" direction="in" name="frequnit">
     <description>Unit of input frequencies.
      Default is unit of the spectral coordinate.</description>
     <value></value>
     </param>
  
     <param xsi:type="any" direction="in" name="velocity">
	     <any type="variant"/>
     <description>Velocity offset</description>
     <value></value>
     </param>  
  
     <param xsi:type="string" direction="in" name="doppler">
     <description>Velocity doppler definition</description>
     <value>radio</value>
     </param>   
</input>
<returns xsi:type="doubleArray"/>
  
<description>

This function <!--(short-hand name {\stf ftf})--> converts frequencies to
frequencies by applying (subtracting) a velocity offset. 

The input frequencies are specified via a vector of numeric values and
a specified unit ({\stfaf frequnit}).  If you don't give a frequency
unit, it is assumed that the units are those given by function <link
anchor="images:coordsys.units.function">coordsys units()</link> for
the spectral coordinate.

This function does not make any frame conversions (e.g.  LSR to BARY)
but you can specifiy the velocity doppler definition for the velocity
via the {\stfaf doppler} argument (see <link
anchor="images:image.summary.function">image summary()</link> for
possible values).

This function fails if there is no spectral coordinate
in the Coordinate System. See also function
<link anchor="images:coordsys.frequencytovelocity.function">frequencytovelocity</link>.

This function presently uses an approximation only valid for velocities
less than the speed of light.  The correction tends to zero as the
velocity appraches the speed of light. You will get NaNs for larger
values of the velocity.

</description>

<example>
\begin{verbatim}
"""
#
print "\t----\t frequencytofrequency Ex 1 \t----"
csys = cs.newcoordsys(spectral=T)
idx = [1.1,1.2,1.3,1.4,1.5]
rval = csys.referencevalue()
rv=rval['numeric'][0]
freq=[]
for i in range(len(idx)):
  freq.append(rv*idx[i])
print "In = [%.4e %.4e %.4e %.4e %.4e]" % tuple(freq)
#In = [1.5565e+09 1.6980e+09 1.8395e+09 1.9810e+09 2.1225e+09]
print cs.frequencytofrequency(freq, velocity='100km/s')
#[1555980894.0489383, 1697433702.5988414, 1838886511.1487448,
# 1980339319.698648, 2121792128.2485518]
#
"""
\end{verbatim}

</example>
</method>

 
   <method type="function" name="frequencytovelocity">
   <shortdescription>Convert frequency to velocity</shortdescription>
   
<input>  
  
     <param xsi:type="doubleArray" direction="in" name="value">
     <description>Frequency to convert</description>
     </param>
  
     <param xsi:type="string" direction="in" name="frequnit">
     <description>Unit of input frequencies.  Default is
      unit of the spectral coordinate.</description>
     <value></value>
     </param>
  
     <param xsi:type="string" direction="in" name="doppler">
     <description>Velocity doppler definition</description>
     <value>radio</value>
     </param>   
  
     <param xsi:type="string" direction="in" name="velunit">
     <description>Unit of output velocities</description>
     <value>km/s</value>
     </param>  
</input>    
<returns xsi:type="doubleArray"/>
  
<description>

This function <!--(short-hand name {\stf ftv})--> converts frequencies to
velocities.  

The input frequencies are specified via a vector of numeric values and
a specified unit ({\stfaf frequnit}).  If you don't give a frequency
unit, it is assumed that the units are those given by function <link
anchor="images:coordsys.units.function">coordsys units()</link> for
the spectral coordinate.

This function does not make any frame conversions (e.g. LSR to BARY)
but you can specifiy the velocity doppler definition via the {\stfaf
doppler} argument (see <link
anchor="images:image.summary.function">image summary()</link> for
possible values).

The velocities are returned in a vector for which you specify the
units ({\stfaf velunit} - default is km/s).

This function will return a fail if there is no spectral coordinate
in the Coordinate System. See also function
<link anchor="images:coordsys.velocitytofrequency.function">velocitytofrequency</link>.

</description>

<example>
\begin{verbatim}
"""
#
print "\t----\t frequencytovelocity Ex 1 \t----"
im = ia.fromshape(shape=[10,10,10])
csys = ia.coordsys()
rtn = csys.findcoordinate('spectral')   # Find spectral axis
pa=rtn['pixel']
wa=rtn['world']
pixel = csys.referencepixel();          # Use reference pixel for non-spectral
nFreq = ia.shape()[pa];                 # Length of spectral axis
freq = [];
for i in range(nFreq):
  pixel[pa] = i            # Assign value for spectral axis of pixel coordinate
  w = csys.toworld(value=pixel, format='n')   # Convert pixel to world
  freq.append(w['numeric'][wa]);              # Fish out frequency
print "freq=", freq
#freq= [1414995000.0, 1414996000.0, 1414997000.0, 1414998000.0,
# 1414999000.0, 1415000000.0, 1415001000.0, 1415002000.0, 1415003000.0, 1415004000.0]
vel = csys.frequencytovelocity(value=freq, doppler='optical', velunit='km/s')
print "vel=", vel
#vel= [1146.3662963847394, 1146.153618169159, 1145.9409402542183, 1145.7282626398826,
# 1145.5155853261515, 1145.3029083129911, 1145.0902316004676, 1144.8775551885467,
# 1144.6648790772279, 1144.4522032665104]
#
"""
\end{verbatim}

In this example, we find the optical velocity in km/s of every pixel
along the spectral axis of our image.  First we  obtain the Coordinate
System from the image.  Then we find which axis of the Coordinate System
(image) pertain to the spectral coordinate.  Then we loop over each
pixel of the spectral axis, and convert a pixel coordinate (one for each
axis of the image) to world.  We obtain the value for the spectral axis
from that world vector, and add it to the vector of frequencies.  Then
we convert that vector of frequencies to velocity.

</example>
</method>

 
   <method type="function" name="fromrecord">
   <shortdescription>Fill Coordinate System from a record</shortdescription>
   
<input>  
  
     <param xsi:type="any" direction="in" name="record">
	     <any type="record"/>
     <description>Record containing Coordinate System</description>
     </param>
</input>    
<returns xsi:type="bool"/>
  
<description>

You can convert a Coordinate System to a record
(<link anchor="images:coordsys.torecord.function">torecord</link>).  This function
(fromrecord) allows you to set the contents of an existing Coordinate
System from such a record.   In doing so, you overwrite its current
contents.

<!--These functions are not for general user use.-->

</description>

<example>
\begin{verbatim}
"""
#
print "\t----\t fromrecord Ex 1 \t----"
csys = cs.newcoordsys(direction=T, stokes="I Q")
print csys.ncoordinates()
#2
r = csys.torecord()
cs2 = cs.newcoordsys()
print cs2.ncoordinates()
#0
cs2.fromrecord(r)
print cs2.ncoordinates()
#2
#
"""
\end{verbatim}
</example>
</method>

 
   <method type="function" name="increment">
   <shortdescription>Recover the increments</shortdescription>
   
<input>
  
     <param xsi:type="string" direction="in" name="format">
	     <description>Format string from combination of "n", "q", "s", "m"</description>
     <value>n</value>
     </param>
  
     <param xsi:type="string" direction="in" name="type">
	     <description>Coordinate type: "direction", "stokes", "spectral", "linear", "tabular".  Leave empty for all.</description>
	     <value></value>
     </param>
</input>
<returns xsi:type="any"><any type="record"/>
<description>Vector of doubles, vector of quantities, vector of string, record or fail</description>
</returns>

<description>

Each axis associated with the Coordinate System has a reference value,
reference pixel and an increment (per pixel).  These are used in the
mapping from pixel to world coordinate. 

This function <!--(short-hand name {\stff i})--> returns the increment (in
world axis order). You can recover  the increments either for all
coordinates (leave {\stfaf type} unset) or for a specific coordinate
type (mimumum match of the allowed types will do).  If you ask for a
non-existent coordinate an exception is generated.

See the \htmlref{discussion}{COORDSYS:FORMATTING} regarding the
formatting possibilities available via argument {\stfaf format}.

You can set the increment with function
<link anchor="images:coordsys.setincrement.function">setincrement</link>. 

</description>

<example>
\begin{verbatim}
"""
#
print "\t----\t increment Ex 1 \t----"
csys=cs.newcoordsys(direction=T,spectral=T)
print csys.increment(format='q')
#{'quantity': {'*1': {'unit': "'", 'value': -1.0},
#              '*2': {'unit': "'", 'value': 1.0},
#              '*3': {'unit': 'Hz', 'value': 1000.0}}}
print csys.increment(format='n')
#{'numeric': [-1.0, 1.0, 1000.0]}
print csys.increment(format='n', type='spectral')
#{'numeric': [1000.0]}
#
"""
\end{verbatim}
</example>
</method>

 
   <method type="function" name="lineartransform">
   <shortdescription>Recover the linear transform matrix</shortdescription>
   
<input>
  
     <param xsi:type="string" direction="in" name="type">
     <description>Coordinate type: "direction", "stokes", "spectral", "linear", "tabular"</description>
     </param>
</input>
<returns xsi:type="any"><any type="variant"/></returns>

<description>

Recover the linear transform component for the specified coordinate type.

You can set the linear transform with function
<link anchor="images:coordsys.setlineartransform.function">setlineartransform</link>. 

</description>
<example>
\begin{verbatim}
"""
#
print "\t----\t lineartransform Ex 1 \t----"
csys=cs.newcoordsys(direction=T,linear=3)
csys.lineartransform('dir')                             # 2 x 2
# [(1.0, 0.0), (0.0, 1.0)]
csys.lineartransform('lin')                             # 3 x 3
# [(1.0, 0.0, 0.0), (0.0, 1.0, 0.0), (0.0, 0.0, 1.0)]
#
"""
\end{verbatim}
</example>
</method>

 
   <method type="function" name="names">
   <shortdescription>Recover the names for each axis</shortdescription>
   
<input>
  
     <param xsi:type="string" direction="in" name="type">
     <description>Coordinate type: "direction", "stokes", "spectral", "linear", "tabular".  Leave empty for all.</description>
     <value></value>
     </param>
</input>

<returns xsi:type="stringArray"/>

<description>

Each axis associated with the Coordinate System has a name (they don't
mean anything fundamental).  This function returns those names in
world axis order.

You can recover the names either for all coordinates (leave {\stfaf
type} unset) or for a specific coordinate type (mimumum match of the
allowed types will do).  If you ask for a non-existent coordinate an
exception is generated.

You can set the names with function
<link anchor="images:coordsys.setnames.function">setnames</link>. 

</description>

<example>
\begin{verbatim}
"""
#
print "\t----\t names Ex 1 \t----"
csys = cs.newcoordsys(direction=T, spectral=T)
n = csys.names()
print n[0]
#Right Ascension 
print n[1]
#Declination 
print n[2]
#Frequency 
print cs.names('spec')
#Frequency
#
"""
\end{verbatim}
</example>
</method>

 
   <method type="function" name="naxes">
   <shortdescription>Recover the number of axes</shortdescription>
   
<input>
  
     <param xsi:type="bool" direction="in" name="world">
     <description>Find number of world or pixel axes ?</description>
     <value>true</value>
     </param>
</input>

<returns xsi:type="int"/>

<description>

Find the number of axes  in the Coordinate System.

You may find the number of world or pixel axes; these are generally the
same and general users can ignore the distinction.  See the
\htmlref{discussion}{COORDSYS:PWAXES} about pixel and world axis
ordering.

</description>

<example>
\begin{verbatim}
"""
#
print "\t----\t naxes Ex 1 \t----"
csys = cs.newcoordsys(direction=T, spectral=T)
n = csys.naxes(T)
print n
#3                          # 2 direction axes, 1 spectral
n = csys.naxes(F)
print n
#3
#
"""
\end{verbatim}
</example>
</method>

 
   <method type="function" name="ncoordinates">
   <shortdescription>Recover the number of coordinates in the Coordinate System</shortdescription>
   
<returns xsi:type="int"/>

<description>

This function <!--(short-hand name {\stff nc})--> recovers the number of
coordinates in the Coordinate System.
</description>


<example>
\begin{verbatim}
"""
#
print "\t----\t ncoordinates Ex 1 \t----"
csys = cs.newcoordsys(direction=T, spectral=T)
print csys.ncoordinates()
#2
cs2 = cs.newcoordsys(linear=4)
print cs2.ncoordinates()
#1
#
"""
\end{verbatim}
</example>

</method>
 
   <method type="function" name="observer">
   <shortdescription>Return the name of the observer</shortdescription>
   
<returns xsi:type="string"/>

<description>

This function returns the name of the observer.
You can set it with the function <link anchor="images:coordsys.setobserver.function">setobserver</link>.

</description>
<example>
\begin{verbatim}
"""
#
print "\t----\t observer Ex 1 \t----"
csys = cs.newcoordsys()
print csys.observer()
#Karl Jansky
#
"""
\end{verbatim}
</example>
</method>

 
   <method type="function" name="projection">
   <shortdescription>Recover the direction coordinate projection</shortdescription>
   
<input>
  
     <param xsi:type="string" direction="in" name="type">
     <description>Type of projection.  Defaults to current projection.</description>
     <value></value>
     </param>
</input>

<returns xsi:type="any"><any type="record"/></returns>

<description>

If the Coordinate System contains a direction coordinate, this function
<!--(short-hand name {\stff p})--> can be used to recover information about the
projection.  For discussion about celestial coordinate systems,
including projections, see the papers by Mark Calabretta and Eric
Greisen. The initial draft  from 1996 (implemented in
\casa.  Background information can be
found
\htmladdnormallink{here}{http://www.atnf.csiro.au/people/mark.calabretta/WCS}.

What this function returns depends upon the value
you assign to {\stfaf type}.

\begin{itemize}

\item {\stfaf type=unset}.  In this case (the default), the actual
projection type and projection parameters are returned in a
record with fields {\cf type} and {\cf parameters}, respectively.

\item {\stfaf type='all'}.  In this case, a vector of strings
containing all of the possible projection codes is returned.

\item {\stfaf type=code}.  If you specify a valid
projection type code (see list by setting {\stfaf type='all'})
then what is returned is the number of parameters required
to describe that projection (useful in function
<link anchor="images:coordsys.setprojection.function">setprojection</link>).

\end{itemize}

You can change the projection with
<link anchor="images:coordsys.setprojection.function">setprojection</link>. 

If the Coordinate System does not contain a direction coordinate,
an exception is generated.

</description>

<example>
\begin{verbatim}
"""
#
print "\t----\t projection Ex 1 \t----"
csys = cs.newcoordsys(direction=T)
print csys.projection()
#{'type': 'SIN', 'parameters': [0.0, 0.0]}
print csys.projection('all')
#{'all': True, 'types': ['AZP', 'TAN', 'SIN', 'STG', 'ARC', 'ZPN', 'ZEA',
# 'AIR', 'CYP', 'CAR', 'MER', 'CEA', 'COP', 'COD', 'COE', 'COO', 'BON',
# 'PCO', 'SFL', 'PAR', 'AIT', 'MOL', 'CSC', 'QSC', 'TSC']}
print csys.projection('ZPN')
#{'nparameters': 100}
#
"""
\end{verbatim}
We first recover the projection type and parameters from
the direction coordinate.  Then we find the list of all
possible projection types.  FInally, we recover the number of 
parameters required to describe the 'ZPN' projection.
</example>
</method>

 
   <method type="function" name="referencecode">
   <shortdescription>Return specified reference code</shortdescription>
   
<input>
  
     <param xsi:type="string" direction="in" name="type">
     <description>Coordinate type: "direction", "stokes", "spectral", "linear", "tabular".  Leave empty for all.</description>
     <value></value>
     </param>
  
     <param xsi:type="bool" direction="in" name="list">
     <description>List all possibilities?</description>
     <value>false</value>
     </param>
</input>
<returns xsi:type="stringArray"/>

<description>

This function <!--(short-hand name {\stff rp})--> returns the reference code
for all, or the specified coordinate type.    Examples of the reference
code are B1950 and J2000 for direction coordinates, or LSRK and BARY for
spectral coordinates.

If {\stfaf type} is left unset, then a vector of strings is returned,
one code for each coordinate type in the Coordinate System.

If you specify {\stfaf type} then select from
'direction', 'spectral', 'stokes', and 'linear'
(the first two letters will do).  However, only the first two
coordinate types will return a non-empty string.
If the Coordinate System does not contain a coordinate of
the type you specify, an exception is generated.

The argument {\stfaf list} is ignored unless you specify a specific {\stfaf type}. 
If {\stfaf list=T}, then this function returns the list of all possible
reference  codes for the specified coordinate type.  Otherwise, it just
returns the actual code current set in the Coordinate System.    

The list of all possible types is returned as a record  (it is
actually generated by the 
<link anchor="measures:measures.listcodes.function">listcodes</link> function in the
<link anchor="measures:measures">measures</link> system). This record has two
fields.  These are called 'normal' 
(containing all normal codes) and 'extra' (maybe empty, with all extra
codes like planets). 

You can set the reference code with
<link anchor="images:coordsys.setreferencecode.function">setreferencecode</link>. 
</description>

<example>
\begin{verbatim}
"""
#
print "\t----\t referencecode Ex 1 \t----"
csys = cs.newcoordsys(direction=T)
clist = csys.referencecode('dir', T)
print clist
#['J2000', 'JMEAN', 'JTRUE', 'APP', 'B1950', 'BMEAN', 'BTRUE',
# 'GALACTIC', 'HADEC', 'AZEL', 'AZELSW', 'AZELNE', 'AZELGEO',
# 'AZELSWGEO', 'AZELNEGEO', 'JNAT', 'ECLIPTIC', 'MECLIPTIC',
# 'TECLIPTIC', 'SUPERGAL', 'ITRF', 'TOPO', 'ICRS',
# 'MERCURY', 'VENUS', 'MARS', 'JUPITER', 'SATURN', 'URANUS',
# 'NEPTUNE', 'PLUTO', 'SUN', 'MOON', 'COMET']
print csys.referencecode('dir')
#J2000
#
"""
\end{verbatim}

In this example we first get the list of all possible reference codes
for a direction coordinate.<!-- (and print some of the normal ones).--> Then we
get the actual reference code for the direction coordinate in our
Coordinate System.

</example>
</method>

 
   <method type="function" name="referencepixel">
   <shortdescription>Recover the reference pixel</shortdescription>
   
<input>
  
     <param xsi:type="string" direction="in" name="type">
     <description>Coordinate type: "direction", "stokes", "spectral", "linear", "tabular".  Leave empty for all.</description>
     <value></value>
     </param>
</input>
<returns xsi:type="any"><any type="record"/></returns>

<description>

Each axis associated with the Coordinate System has a reference value,
reference pixel and an increment (per pixel).  These are used in the
mapping from pixel to world coordinate. 

This function <!--(short-hand name {\stff rp})--> returns the reference pixel
(in pixel axis order). You can recover  the reference pixel either for
all coordinates (leave {\stfaf type} unset) or for a specific coordinate
type (mimumum match of the allowed types will do).  If you ask for a
non-existent coordinate an exception is generated.

You can set the reference pixel with function
<link anchor="images:coordsys.setreferencepixel.function">setreferencepixel</link>. 

</description>
<example>
\begin{verbatim}
"""
#
print "\t----\t referencepixel Ex 1 \t----"
csys = cs.newcoordsys(spectral=T, linear=2)
csys.setreferencepixel([1.0, 2.0, 3.0])
print csys.referencepixel()
#{'ar_type': 'absolute', 'pw_type': 'pixel', 'numeric': array([ 1.,  2.,  3.])}
print csys.referencepixel('lin')
#{'ar_type': 'absolute', 'pw_type': 'pixel', 'numeric': array([ 2.,  3.])}
#
"""
\end{verbatim}
</example>
</method>

 
   <method type="function" name="referencevalue">
   <shortdescription>Recover the reference value</shortdescription>
   
<input>
  
     <param xsi:type="string" direction="in" name="format">
     <description>Format string.  Combination of "n", "q", "s", "m"</description>
     <value>n</value>
     </param>
  
     <param xsi:type="string" direction="in" name="type">
     <description>Coordinate type: "direction", "stokes", "spectral", "linear", "tabular".  Leave empty for all.</description>
     <value></value>
     </param>
</input>
<returns xsi:type="any"><any type="record"/></returns>

<description>

Each axis associated with the Coordinate System has a reference value,
reference pixel and an increment (per pixel).  These are used in the
mapping from pixel to world coordinate. 

This function <!--(short-hand name {\stff rv})--> returns the reference value
(in world axis order). You can recover  the reference value either for all
coordinates (leave {\stfaf type} unset) or for a specific coordinate
type (mimumum match of the allowed types will do).  If you ask for a
non-existent coordinate an exception is generated.

See the \htmlref{discussion}{COORDSYS:FORMATTING} regarding the
formatting possibilities available via argument {\stfaf format}.

You can set the reference value with function
<link anchor="images:coordsys.setreferencevalue.function">setreferencevalue</link>. 

</description>

<example>
\begin{verbatim}
"""
#
print "\t----\t referencevalue Ex 1 \t----"
csys = cs.newcoordsys(direction=T, spectral=T)
print csys.referencevalue(format='q')
#{'ar_type': 'absolute',
# 'pw_type': 'world',
# 'quantity': {'*1': {'unit': "'", 'value': 0.0},
#              '*2': {'unit': "'", 'value': 0.0},
#              '*3': {'unit': 'Hz', 'value': 1415000000.0}}}
print csys.referencevalue(format='n')
#{'ar_type': 'absolute',
# 'numeric': array([  0.00000000e+00,   0.00000000e+00,   1.41500000e+09]),
# 'pw_type': 'world'}
print csys.referencevalue(format='n', type='spec')
#{'ar_type': 'absolute',
# 'numeric': array([  1.41500000e+09]),
# 'pw_type': 'world'}
#
"""
\end{verbatim}
</example>
</method>

 
   <method type="function" name="reorder">
   <shortdescription>Reorder the coordinates</shortdescription>
   
<input>
  
     <param xsi:type="intArray" direction="in" name="order">
     <description>New coordinate order</description>
     </param>
</input>
<returns xsi:type="bool"/>

<description>

This function reorders the coordinates in the Coordinate System.
You specify the new order of the coordinates in terms of their old
order.

</description>

<example>
\begin{verbatim}
"""
#
print "\t----\t reorder Ex 1 \t----"
csys = cs.newcoordsys(direction=T, spectral=T, linear=2)
print csys.coordinatetype()
#['Direction', 'Spectral', 'Linear']
csys.reorder([1,2,0]);
print csys.coordinatetype()
#['Spectral', 'Linear', 'Direction']
#
"""
\end{verbatim}
</example>
</method>

 
   <method type="function" name="replace">
   <shortdescription>Replace a coordinate</shortdescription>
   
<input>
  
	<param xsi:type="any" direction="in" name="csys">
		<any type="record"/>
     <description>Coordinate System to replace from. Use
      coordsys' torecord() to generate required record.</description>
     </param>
  
     <param xsi:type="int" direction="in" name="whichin">
     <description>Index of input coordinate (0-rel)</description>
     </param>
  
     <param xsi:type="int" direction="in" name="whichout">
     <description>Index of output coordinate</description>
     </param>
</input>
<returns xsi:type="bool"/>

<description>

This function replaces one coordinate in the current Coordinate System by
one coordinate in the given Coordinate System.  The specified
coordinates must have the same number of axes.

</description>

<example>
\begin{verbatim}
"""
#
print "\t----\t replace Ex 1 \t----"
cs1 = cs.newcoordsys(direction=T, linear=1)
print cs1.coordinatetype()
#['Direction', 'Linear']
cs2 = cs.newcoordsys(spectral=T)
cs1.replace (cs2.torecord(),0,1)
print cs1.coordinatetype()
#['Direction', 'Spectral']
#
"""
\end{verbatim}
</example>
</method>

 
   <method type="function" name="restfrequency">
   <shortdescription>Recover the rest frequency</shortdescription>
   
   <returns xsi:type="any"><any type="record"/></returns>

<description>

If the Coordinate System contains a spectral coordinate, then
it has a rest frequency.  In fact, the spectral coordinate
can hold several rest frequencies (to handle for example,
an observation where the band covers many lines), although
only one is active (for velocity conversions) at a time.

This function <!--(short-hand name {\stff rf})--> recovers the rest frequencies
as a quantity vector.   The first frequency is the active one.

You can change the rest frequencies with
<link anchor="images:coordsys.setrestfrequency.function">setrestfrequency</link>. 

If the Coordinate System does not contain a frequency coordinate,
an exception is generated.

</description>

<example>
\begin{verbatim}
"""
#
print "\t----\t restfrequency Ex 1 \t----"
csys = cs.newcoordsys(spectral=T)
print csys.restfrequency()
#{'value': [1420405751.7860003], 'unit': 'Hz'}
csys.setrestfrequency (value=qa.quantity([1.2e9, 1.3e9],'Hz'), which=1, append=F)
print csys.restfrequency()
#{'value': [1300000000.0, 1200000000.0], 'unit': 'Hz'}
#
"""
\end{verbatim}
In the example, the initial spectral coordinate has 1 rest frequency.
Then we set it with two, nominating the second as the active rest frequency,
and recover them.
</example>
</method>

 
   <method type="function" name="setconversiontype">
   <shortdescription>Set extra reference conversion layer</shortdescription>
   
<input>
  
     <param xsi:type="string" direction="in" name="direction">
     <description>Reference code</description>
     <value></value>
     </param>
  
     <param xsi:type="string" direction="in" name="spectral">
     <description>Reference code</description>
     <value></value>
     </param>
</input>

<returns xsi:type="bool"/>

<description>

Some coordinates contain a reference code.  Examples of reference codes
are B1950 and J2000 for direction coordinates, or LSRK and BARY for
spectral coordinates.  When you do conversions between pixel and world
coordinate, the coordinates are in the reference frame corresponding to
these codes.   

This function allows you to specify a different reference frame which
is used when converting between world and pixel coordinate (see
function <link
anchor="images:coordsys.conversiontype.function">conversiontype</link>
to recover the conversion types).  If it returns F, it means that
although the conversion machines were successfully created, a trial
conversion failed.  This usually means the REST frame was involved
which requires a radial velocity (not yet implemented).  If this
happens, the conversion type will be left as it was. The function
fails if more blatant things are wrong like a missing coordinate, or
an incorrect reference code.

The list of possible reference codes can be obtained via function
<link anchor="images:coordsys.referencecode.function">referencecode</link>.

With this function, you specify the desired reference code.  Then,
when a conversion between pixel and world is requested, an extra
conversion is done to ({\stff toWorld}) or from ({\stff toPixel}) the
specified reference frame.

The <link anchor="images:coordsys.summary.function">summary</link>
function shows the extra conversion reference system to the right of
the native reference system (if it is different) and in parentheses.

Note that to convert between different spectral reference frames, you
need a position, epoch and direction.  The position (telescope) and
epoch (date of observation), if not in your coordinate system can be set
with functions <link anchor="images:coordsys.settelescope.function">settelescope</link> and
<link anchor="images:coordsys.setepoch.function">setepoch</link>.  The direction is the
reference direction of the {\it required} direction coordinate in the
coordinate system. 

\bigskip\goodbreak
As an example, let us say you are working with a spectral coordinate
which was constructed with the LSRK reference frame.  You want to convert
some pixel coordinates to barycentric velocities (reference code BARY).

\begin{verbatim}
"""
#
print "\t----\t setconversiontype Ex 1 \t----"
csys = cs.newcoordsys(direction=T, spectral=T); # Create coordinate system
rtn=csys.findcoordinate('spectral')             # Find spectral coordinate
wa=rtn['world']
pa=rtn['pixel']
u = csys.units()[wa]                            # Spectral unit
print csys.referencecode(type='spectral')       # Which is  in LSRK reference frame
#LSRK
p = [10,20,30]
w = csys.toworld(p, format='n')           # Convert a pixel to LSRK world
print 'pixel, world = ', p, w['numeric']
#pixel, world =  [10, 20, 30] [21589.999816660376, 20.000112822985134, 1415030000.0]
p2 = csys.topixel(w)                      # and back to pixel
print 'world, pixel = ', w['numeric'], p2
#world, pixel =  [21589.999816660376, 20.000112822985134, 1415030000.0]
# [10.00000000000248, 19.999999999999801, 30.0]
# Convert LSRK frequency to LSRK velocity
v = csys.frequencytovelocity(value=w['numeric'][wa], frequnit=u, 
                             doppler='RADIO', velunit='m/s');
print 'pixel, frequency, velocity = ', p[pa], w['numeric'][wa], v
#pixel, frequency, velocity =  30 1415030000.0 1134612.30321
csys.setconversiontype(spectral='BARY')   # Specify BARY reference code
w = csys.toworld(p, format='n')           # Convert a pixel to BARY world
print 'pixel, world = ', p, w['numeric']
#pixel, world =  [10, 20, 30] [21589.999816660376, 20.000112822985134, 1415031369.0081882]
p2 = csys.topixel(w)                      # and back to pixel
print 'world, pixel = ', w['numeric'], p2
#world, pixel =  [21589.999816660376, 20.000112822985134, 1415031369.0081882]
# [10.00000000000248, 19.999999999999801, 30.0]
# Convert BARY frequency to BARY velocity
v = csys.frequencytovelocity(value=w['numeric'][wa], frequnit=u, 
                              doppler='RADIO', velunit='m/s');
print 'pixel, frequency, velocity = ', p[pa], w['numeric'][wa], v
#pixel, frequency, velocity =  30 1415031369.01 1134323.35878
#
"""
\end{verbatim}

You must also be aware of when this extra layer is active and when it is
not.  It's a bit nasty. 

\begin{itemize}

\item - Whenever you use {\stff toWorld}, {\stff toPixel}
{\stff toWorldMany}, or {\stff toPixelMany} the layer is active.   

\item - Whenever you use {\stff convert} or {\stff convertMany}
the layer {\it may} be active.   Here are the rules !

It is only relevant to spectral and direction coordinates.

For the direction coordinate part of your conversion, if you request a
pure world or pixel conversion it is active.  Any pixel/world mix will
not invoke it (because it is ill defined). 

For the spectral coordinate part it is always active (only one axis
so must be pixel or world).

\item - This layer is irrelevant to all functions converting between
frequency and velocity, and absolute and relative.  The values are in
whatever frame you are working with. 

\end{itemize}

The <link
anchor="images:coordsys.summary.function">summary</link> function
lists the reference frame for direction and spectral coordinates.  If
you have also set a conversion reference code it also lists that (to
the right in parentheses).

</description>
</method>

 
 
   <method type="function" name="getconversiontype">
   <shortdescription>Get extra reference conversion layer
   (aka conversiontype).  </shortdescription>
   
<input>
  
     <param xsi:type="string" direction="in" name="type">
     <description>Conversion type</description>
     <value></value>
     </param>
</input>

<returns xsi:type="string"/>

<description>  See conversiontype for more complete description.
</description>

</method>



   <method type="function" name="setdirection">
   <shortdescription>Set direction coordinate values</shortdescription>
   
<input>
  
     <param xsi:type="string" direction="in" name="refcode">
     <description>Reference code.  Default is no change.</description>
     <value></value>
     </param>  
  
     <param xsi:type="string" direction="in" name="proj">
     <description>Projection type.  Default is no change.</description>
     <value></value>
     </param>
  
     <param xsi:type="doubleArray" direction="in" name="projpar">
     <description>Projection parameters.  Default is no change.</description>
     <value>-1</value>
     </param>
  
     <param xsi:type="doubleArray" direction="in" name="refpix">
     <description>Reference pixel.  Default is no change.</description>
     <value>-1</value>
     </param>
  
     <param xsi:type="any" direction="in" name="refval">
	     <any type="variant"/>
	     <description>Reference value.  Default is no change.</description>
     <value></value>
     </param>
  
     <param xsi:type="any" direction="in" name="incr">
	     <any type="variant"/>
     <description>Increment.  Default is no change.</description>
     <value></value>
     </param>
  
     <param xsi:type="any" direction="in" name="xform">
	     <any type="variant"/>
     <description>Linear transform.  Default is no change.</description>
     <value></value>
     </param>
  
     <param xsi:type="any" direction="in" name="poles">
	     <any type="variant"/>
     <description>Native poles.  Default is no change.</description>
     <value></value>
     </param>
</input>
<returns xsi:type="bool"/>

<description>

When you construct a Coordsys \tool, if you include a Direction
Coordinate, it  will have some default parameters.  
This function simply allows you to
replace the values of the Direction Coordinate.

You can also change almost all of those parameters (such as projection, reference value
etc.) via the individual functions 
<link anchor="images:coordsys.setreferencecode.function">setreferencecode</link>,
<link anchor="images:coordsys.setprojection.function">setprojection</link>,
<link anchor="images:coordsys.setreferencepixel.function">setreferencepixel</link>,
<link anchor="images:coordsys.setreferencevalue.function">setreferencevalue</link>,
<link anchor="images:coordsys.setincrement.function">setincrement</link>, and
<link anchor="images:coordsys.setlineartransform.function">setlineartransform</link>
provided by the Coordsys \tool.    See those functions for more details
about the formatting of the above function arguments.

Bear in mind, that if your Coordinate System came from a real image, then
the reference pixel is special and you should not change it.

</description>

<example>
\begin{verbatim}
"""
#
print "\t----\t setdirection Ex 1 \t----"
csys = cs.newcoordsys(direction=T);
csys.setdirection (refcode='GALACTIC', proj='SIN', projpar=[0,0],
                   refpix=[-10,20], refval="10deg -20deg");
print csys.projection()
#{'type': 'SIN', 'parameters': array([ 0.,  0.])}
print csys.referencepixel()
#{'ar_type': 'absolute', 'pw_type': 'pixel', 'numeric': array([-10.,  20.])}
print csys.referencevalue(format='s')
#{'ar_type': 'absolute', 'pw_type': 'world', 
# 'string': array(['10.00000000 deg', '-20.00000000 deg'], dtype='|S17')}
#
"""
\end{verbatim}
</example>
</method>

 
   <method type="function" name="setepoch">
   <shortdescription>Set a new epoch</shortdescription>
   
<input>
  
     <param xsi:type="any" direction="in" name="value">
     <any type="record"/>
     <description>New epoch measure</description>
     </param>
</input>

<returns xsi:type="bool"/>

<description>

This function <!--(short-hand name {\stff se})--> sets a new epoch (supplied as an
<link anchor="measures:measures.epoch.function">epoch</link> measure) of the observation. You
can get the current epoch with function
<link anchor="images:coordsys.epoch.function">epoch</link>.  

</description>

<example>
\begin{verbatim}
"""
#
print "\t----\t setepoch Ex 1 \t----"
csys = cs.newcoordsys()
ep = csys.epoch()
print ep
#{'type': 'epoch', 'm0': {'value': 54161.766782997685, 'unit': 'd'}, 'refer': 'UTC'}
ep = me.epoch('UTC', 'today')
csys.setepoch(ep)
print csys.epoch()
#{'type': 'epoch', 'm0': {'value': 54161.766782997685, 'unit': 'd'}, 'refer': 'UTC'}
#
"""
\end{verbatim}
</example>
</method>

 
   <method type="function" name="setincrement">
   <shortdescription>Set the increment</shortdescription>
   
<input>
  
     <param xsi:type="any" direction="in" name="value">
	     <any type="variant"/>
     <description>Increments</description>
     <value></value>
     </param>
  
     <param xsi:type="string" direction="in" name="type">
     <description>Coordinate type: "direction", "stokes", "spectral", "linear", "tabular". Leave empty for all</description>
     <value></value>
     </param>
</input>
<returns xsi:type="bool"/>

<description>

Each axis associated with the Coordinate System has a reference value,
reference pixel and an increment (per pixel).  These are used in the
mapping from pixel to world coordinate. 

This function <!--(short-hand name {\stff si})--> allows you to set a new
increment.   You should not do this on "stokes" axes unless you are an
adept or a big risk taker.

You can set the increments either for all axes ({\stfaf
type=unset}) or for just the axes associated with a particular
coordinate type.

You may supply the increments in all of the formats described in
the \htmlref{formatting}{COORDSYS:FORMATTING} discussion.

In addition, you can also supply the increments as  a quantity of vector
of doubles.  For example {\stfaf qa.quantity([-1,2],'arcsec')}.

You can recover the current increments with function
<link anchor="images:coordsys.increment.function">increment</link>. 

</description>

<example>
\begin{verbatim}
"""
#
print "\t----\t setincrement Ex 1 \t----"
csys=cs.newcoordsys(direction=T, spectral=T)
rv = csys.increment(format='q')
print rv
# {'ar_type': 'absolute', 'pw_type': 'world',
# 'quantity': {'*1': {'value': -1.0, 'unit': "'"},
#              '*2': {'value': 1.0, 'unit': "'"},
#              '*3': {'value': 1000.0, 'unit': 'Hz'}}}
rv2 = qa.quantity('4kHz');
csys.setincrement(value=rv2, type='spec')
print csys.increment(type='spec', format='q')
#{'ar_type': 'absolute', 'pw_type': 'world',
# 'quantity': {'*1': {'value': 4000.0, 'unit': 'Hz'}}}
csys.setincrement(value='5kHz', type='spec')
print csys.increment(type='spec', format='q')
#{'ar_type': 'absolute', 'pw_type': 'world',
# 'quantity': {'*1': {'value': 5000.0, 'unit': 'Hz'}}}
print csys.increment(format='q')
#{'ar_type': 'absolute', 'pw_type': 'world',
# 'quantity': {'*1': {'value': -1.0, 'unit': "'"},
#              '*2': {'value': 1.0, 'unit': "'"},
#              '*3': {'value': 5000.0, 'unit': 'Hz'}}}
csys.setincrement (value="-2' 2' 2e4Hz")
print csys.increment(format='q')
#{'ar_type': 'absolute', 'pw_type': 'world',
# 'quantity': {'*1': {'value': -2.0, 'unit': "'"},
#              '*2': {'value': 2.0, 'unit': "'"},
#              '*3': {'value': 20000.0, 'unit': 'Hz'}}}
#
"""
\end{verbatim}

In the example we first recover the increments as a vector of
quantities. We then create a quantity for a new value for the spectral
coordinate increment.  Note we use units of kHz whereas the spectral
coordinate is currently expressed in units of Hz.  We then set the
increment for the spectral coordinate.  We then recover the increment
again; you can see 4kHz has been converted to 4000Hz.  We also show
how to set the increment using a string interface.

</example>

</method>

 
   <method type="function" name="setlineartransform">
   <shortdescription>Set the linear transform</shortdescription>
   
<input>
  
     <param xsi:type="string" direction="in" name="type">
     <description>Coordinate type: "direction", "stokes", "spectral", "linear", "tabular".  Leave empty for all.</description>
     <value></value>
     </param>
  
     <param xsi:type="any" direction="in" name="value">
	<any type="variant"/>
     <description>Linear transform</description>
     </param>
</input>
<returns xsi:type="bool"/>

<description>

This function set the linear transform component.  For Stokes Coordinates
this function will return T but do nothing.

You can recover the current linear transform with function
<link anchor="images:coordsys.lineartransform.function">lineartransform</link>. 

</description>
<example>
\begin{verbatim}
"""
#
print "\t----\t setlineartransform Ex 1 \t----"
csys = cs.newcoordsys(spectral=T, linear=3)
xf = csys.lineartransform('lin')
print xf
#[(1.0, 0.0, 0.0), (0.0, 1.0, 0.0), (0.0, 0.0, 1.0)]
xf[0]=list(xf[0])
xf[0][1]=0.01
#xf[0]=tuple(xf[0])
csys.setlineartransform('lin',xf)
print csys.lineartransform('lin')
#[(1.0, 0.01, 0.0), (0.0, 1.0, 0.0), (0.0, 0.0, 1.0)]
"""
\end{verbatim}
</example>
</method>

 
   <method type="function" name="setnames">
   <shortdescription>Set the axis names</shortdescription>
   
<input>
  
     <param xsi:type="stringArray" direction="in" name="value">
     <description>Names</description>
     </param>
  
     <param xsi:type="string" direction="in" name="type">
	     <description>Coordinate type: "direction", "stokes", "spectral", "linear", "tabular" or leave empty for all</description>
	     <value></value>
     </param>
</input>
<returns xsi:type="bool"/>

<description>

Each axis associated with the Coordinate System has a name.
It isn't used in any fundamental way.

This function <!--(short-hand name {\stff sn})--> allows you to set
new axis names.

You can set the names either for all axes ({\stfaf
type=unset}) or for just the axes associated with a particular
coordinate type.

You can recover the current axis names with function
<link anchor="images:coordsys.names.function">names</link>. 

</description>
<example>
\begin{verbatim}
"""
#
print "\t----\t setnames Ex 1 \t----"
csys = cs.newcoordsys(spectral=T, linear=2)
csys.setnames(value="a b c")
print csys.names()
#['a', 'b', 'c']
csys.setnames("flying fish", 'lin')
print csys.names()
#['a', 'flying', 'fish']
#
"""
\end{verbatim}
</example>
</method>

 
   <method type="function" name="setobserver">
   <shortdescription>Set a new observer</shortdescription>
   
<input>
  
     <param xsi:type="string" direction="in" name="value">
     <description>New observer</description>
     </param>
</input>

<returns xsi:type="bool"/>

<description>

If you want to grab all the glory, or transfer the blame, this function
<!--(short-hand name {\stff so})--> sets a new observer of the
observation. You can get the current observer with function <link
anchor="images:coordsys.observer.function">observer</link>.  The
observer's name is not fundamental to the Coordinate System !

</description>

<example>
\begin{verbatim}
"""
#
print "\t----\t setobserver Ex 1 \t----"
csys = cs.newcoordsys()
print csys.observer()
#Karl Jansky 
csys.setobserver('Ronald Biggs')
print csys.observer()
#Ronald Biggs 
#
"""
\end{verbatim}
</example>
</method>

 
   <method type="function" name="setprojection">
   <shortdescription>Set the direction coordinate projection</shortdescription>
   
<input>
  
     <param xsi:type="string" direction="in" name="type">
     <description>Type of projection</description>
     </param>
  
     <param xsi:type="doubleArray" direction="in" name="parameters">
     <description>Projection parameters</description>
     <value>-1</value>
     </param>
</input>
<returns xsi:type="bool"/>

<description>

If the Coordinate System contains a direction coordinate, this
function <!--(short-hand name {\stff sp})--> can be used to set the
projection.  For discussion about celestial coordinate systems,
including projections, see the papers by Mark Calabretta and Eric
Greisen. The initial draft from 1996 (implemented in \casa) can be
found
\htmladdnormallink{here}{http://www.atnf.csiro.au/people/mark.calabretta/WCS}.

You can use the function <link anchor="images:coordsys.projection.function">projection</link>
to find out all the possible types of projection.  You can also use it
to find out how many parameters you need to describe a particular
projection.  See Calabretta and Greisen for details about those
parameters (see section 4 of their paper); in FITS terms these
parameters are what are labelled as PROJP. 

Some brief help here on the more common projections in astronomy.

\begin{itemize}

\item SIN has either 0 parameters or 2.  For coplanar arrays like
East-West arrays, one can use what is widely termed the NCP projection. 
This is actually a SIN projection where the parameters are 0 and
$1/tan(\delta_0)$ where $\delta_0$ is the reference declination.  Images
made from the ATNF's Compact Array with \casa\ will have such a
projection.  Otherwise, the SIN projection requires no parameters (but
you can give it two each of which is zero if you wish).

\item TAN is used widely in optical astronomy.  It requires 0
parameters.

\item ZEA (zenithal equal area) is used widely in survey work.
It requires 0 parameters.

\end{itemize}

If the Coordinate System does not contain a direction coordinate,
an exception is generated.

</description>

<example>
\begin{verbatim}
"""
#
print "\t----\t Ex setprojection 1 \t----"
im = ia.maketestimage('cena',overwrite=true)
csys = ia.coordsys()
print csys.projection()
#{'type': 'SIN', 'parameters': array([ 0.,  0.])}
print csys.projection('ZEA')
#{'nparameters': 0}
csys.setprojection('ZEA')
im2 = ia.regrid('cena.zea', csys=csys.torecord(), overwrite=true)
#
"""
\end{verbatim}
We change the projection of an image from SIN to 
ZEA (which requires no parameters).
</example>
</method>

 
   <method type="function" name="setreferencecode">
   <shortdescription>Set new reference code</shortdescription>
   
<input>
  
     <param xsi:type="string" direction="in" name="value">
     <description>Reference code</description>
     </param>
  
     <param xsi:type="string" direction="in" name="type">
     <description>Coordinate type: direction or spectral</description>
     <value>direction</value>
     </param>
  
     <param xsi:type="bool" direction="in" name="adjust">
     <description>Adjust reference value ?</description>
     <value>true</value>
     </param>
</input>
<returns xsi:type="bool"/>

<description>

This function <!--(short-hand name {\stff src})--> sets the reference
code for the specified coordinate type.  Examples of reference codes
are B1950 and J2000 for direction coordinates, or LSRK and BARY for
spectral coordinates.

You must specify {\stfaf type}, selecting from 'direction',  or
'spectral' (the first two letters will do).   If the Coordinate System
does not contain a coordinate of the type you specify, an exception is
generated.

Specify the new code with argument {\stfaf value}.  To see the list of
possible codes, use the function <link
anchor="images:coordsys.referencecode.function">referencecode</link>
(see example).

If {\stfaf adjust} is T, then the reference value is recomputed.
This is invariably the correct thing to do.  If {\stfaf adjust} is F, 
then the reference code is simply overwritten; do this very carefully.

</description>

<example>
\begin{verbatim}
"""
#
print "\t----\t Ex setreferencecode 1 \t----"
csys = cs.newcoordsys(direction=T)
clist = csys.referencecode('dir', T)      # See possibilities
print clist
#['J2000', 'JMEAN', 'JTRUE', 'APP', 'B1950', 'BMEAN', 'BTRUE', 'GALACTIC',
# 'HADEC', 'AZEL', 'AZELSW', 'AZELNE', 'AZELGEO', 'AZELSWGEO', 'AZELNEGEO',
# 'JNAT', 'ECLIPTIC', 'MECLIPTIC', 'TECLIPTIC', 'SUPERGAL', 'ITRF', 'TOPO',
# 'ICRS', 'MERCURY', 'VENUS', 'MARS', 'JUPITER', 'SATURN', 'URANUS',
# 'NEPTUNE', 'PLUTO', 'SUN', 'MOON', 'COMET']
print cs.referencecode('dir')
#J2000
cs.setreferencecode('B1950', 'dir', T)
#
"""
\end{verbatim}

In this example we first get the list of all possible reference codes
for a direction coordinate. <!-- (and print some of the normal
ones).--> Then we set the actual reference code for the direction
coordinate in our Coordinate System.

</example>

<example>
\begin{verbatim}
"""
#
print "\t----\t Ex setreferencecode 2 \t----"
ia.maketestimage('myimage.j2000',overwrite=true)      # Open image
csys = ia.coordsys()			              # Get Coordinate System
print csys.referencecode('dir', F)
#J2000
csys.setreferencecode('B1950', 'dir', T)              # Set new direction system
im2 = ia.regrid(outfile='myimage.b1950', csys=csys.torecord(),
                overwrite=true)  # Regrid and make new image
#
"""
\end{verbatim}

In this example we show how to regrid an image from J2000
to B1950.  First we recover the Coordinate System  into the Coordsys
\tool\ called {\stf cs}.  We then set a new direction reference code,
making sure we recompute the reference value.  Then the
new Coordinate System is supplied in the regridding process
(done with an Image \tool).

</example>
</method>

 
   <method type="function" name="setreferencelocation">
   <shortdescription>Set reference pixel and value</shortdescription>
   
<input>
  
     <param xsi:type="intArray" direction="in" name="pixel">
     <description>New reference pixel.  Defaults to old reference pixel.</description>
     <value>-1</value>
     </param>
  
     <param xsi:type="any" direction="in" name="world">
	     <any type="variant"/>
     <description>New reference value.  Defaults to old reference value.</description>
     <value>-1</value>
     </param>
  
     <param xsi:type="boolArray" direction="in" name="mask">
     <description>Indicates which axes to center.  Defaults to all.</description>
     <value>false</value>
     </param>
</input>

<returns xsi:type="bool"/>
<description>

This function<!--(short-hand name {\stff srl})--> sets the reference pixel and
reference value to the specified values.  The world coordinate can be
specified in any of the formats that the output world coordinate is
returned in by the <link anchor="images:coordsys.toworld.function">toworld</link> function. 

You can specify a mask (argument {\stfaf mask}) indicating which pixel
axes are set (T) and which are left unchanged (F).  This function will
refuse to change the reference location of a Stokes axis (gets you into
trouble otherwise). 

This function can be rather useful when <link anchor="images:image.regrid.function">regridding</link>
images.  It allows you to keep easily a particular feature centered in the 
regridded image.

</description>
<example>
\begin{verbatim}
"""
#
print "\t----\t setreferencelocation Ex 1 \t----"
csys = cs.newcoordsys(linear=2)
print csys.referencepixel()
#[0.0, 0.0]
print csys.referencevalue()
#{'numeric': array([ 0.,  0.])}
w = csys.toworld([19,19], format='n')
shp = [128,128]
p = [64, 64]
csys.setreferencelocation (pixel=p, world=w)
print csys.referencepixel()
#[64.0, 64.0]  
print csys.referencevalue()
#{'numeric': array([ 19.,  19.])}
#
"""
\end{verbatim}
</example>
</method>

 
   <method type="function" name="setreferencepixel">
   <shortdescription>Set the reference pixel</shortdescription>
   
<input>
  
     <param xsi:type="doubleArray" direction="in" name="value">
     <description>Reference pixel</description>
     </param>

     <param xsi:type="string" direction="in" name="type">
     <description>Coordinate type: "direction", "stokes", "spectral", "linear", "tabular" or leave unset for all</description>
     <value></value>
     </param>
  
</input>
<returns xsi:type="bool"/>

<description>

Each axis associated with the Coordinate System has a reference value,
reference pixel and an increment (per pixel).  These are used in the
mapping from pixel to world coordinate. 

This function<!--(short-hand name {\stff srp})--> allows you to set a new reference pixel.   You should not
do this on "stokes" axes unless you are an adept or a big risk taker.

You can set the reference pixel either for all axes ({\stfaf
type=unset}) or for just the axes associated with a particular
coordinate type.

Bear in mind, that if your Coordinate System came from a real image,
then the reference pixel is special and you should not change it for
Direction Coordinates. 

You can recover the current reference pixel with function
<link anchor="images:coordsys.referencepixel.function">referencepixel</link>. 

</description>
<example>
\begin{verbatim}
"""
#
print "\t----\t setreferencepixel Ex 1 \t----"
csys = cs.newcoordsys(spectral=T, linear=2)
csys.setreferencepixel(value=[1.0, 2.0, 3.0])
print csys.referencepixel()
#[1.0, 2.0, 3.0]
csys.setreferencepixel([-1, -1], 'lin')
print csys.referencepixel()
#[1.0, -1.0, -1.0]
#
"""
\end{verbatim}
</example>
</method>

 
   <method type="function" name="setreferencevalue">
   <shortdescription>Set the reference value</shortdescription>
   
<input>
  
     <param xsi:type="any" direction="in" name="value">
	     <any type="variant"/>
     <description>Reference value</description>
     </param>
  
     <param xsi:type="string" direction="in" name="type">
     <description>Coordinate type: "direction", "stokes", "spectral", "linear", "tabular" or leave empty for all.</description>
     <value></value>
     </param>
</input>
<returns xsi:type="bool"/>

<description>

Each axis associated with the Coordinate System has a reference value,
reference pixel and an increment (per pixel).  These are used in the
mapping from pixel to world coordinate. 

This function<!--(short-hand name {\stff srv})--> allows you to set a new
reference value.  You should not do this on "stokes" axes unless you
are an adept or a big risk taker.

You may supply the reference value in all of the formats described in
the \htmlref{formatting}{COORDSYS:FORMATTING} discussion.

You can recover the current reference value with function
<link anchor="images:coordsys.referencevalue.function">referencevalue</link>. 

Note that the value argument should be one of the specified
possibilitioes. Especially a {\stff measure} will be accepted, but
will have a null effect, due to the interpretation as a generic
record.

</description>
<example>
\begin{verbatim}
"""
#
print "\t----\t setreferencevalue Ex 1 \t----"
csys = cs.newcoordsys(direction=T, spectral=T)
rv = csys.referencevalue(format='q')
print rv
#{'quantity': {'*1': {'value': 0.0, 'unit': "'"},
# '*2': {'value': 0.0, 'unit': "'"}, '*3': {'value': 1415000000.0, 'unit': 'Hz'}}}
rv2 = rv['quantity']['*3']
rv2['value'] = 2.0e9
print rv2
#{'value': 2000000000.0, 'unit': 'Hz'}
csys.setreferencevalue(type='spec', value=rv2)
print csys.referencevalue(format='n')
#{'numeric': array([  0.00000000e+00,   0.00000000e+00,   2.00000000e+09])}
#
# To set a new direction reference value, the easiest way, given a
# direction measure dr would be:
dr = me.direction('j2000','30deg','40deg')
# SHOULD BE SIMPLIFIED!!!
newrv=csys.referencevalue(format='q')
newrv['quantity']['*1']=dr['m0']
newrv['quantity']['*2']=dr['m1']
csys.setreferencevalue(value=newrv)
print csys.referencevalue(format='q')
#{'ar_type': 'absolute', 'pw_type': 'world',
# 'quantity': {'*1': {'value': 1800.0, 'unit': "'"},
#              '*2': {'value': 2399.9999999999995, 'unit': "'"},
#              '*3': {'value': 1415000000.0, 'unit': 'Hz'}}}
#
"""
\end{verbatim}
</example>
In the example we first recover the reference value as a vector of quantities.
We then select out the value for the spectral coordinate, change it,
and set it back in the Coordinate System.  We then recover the
reference value again, but this time just as a vector of doubles. In
the case of the dircetion, we set the value from a known direction measure.
</method>

 
   <method type="function" name="setrestfrequency">
   <shortdescription>Set the rest frequency</shortdescription>
   
<input>
  
     <param xsi:type="any" direction="in" name="value">
     <any type="variant"/>
     <description>New rest frequencies</description>
     </param>
  
     <param xsi:type="int" direction="in" name="which">
     <description>Which is the active rest frequency</description>
     <value>0</value>
     </param>
  
     <param xsi:type="bool" direction="in" name="append">
     <description>Append this list or overwrite ?</description>
     <value>false</value>
     </param>
</input>

<returns xsi:type="bool"/>

<description>

If the Coordinate System contains a spectral coordinate, then
it has a rest frequency.  In fact, the spectral coordinate
can hold several rest frequencies (to handle for example,
an observation where the band covers many lines), although
only one is active (for velocity conversions) at a time.

This function<!--(short-hand name {\stff srf})--> allows you to set new rest
frequencies.  You can provide the rest frequency as a quantity, or as
a quantity string, or a double (units of current rest frequency assumed).

You specify whether the list of frequencies will be appended
to the current list or whether it will replace that list.
You must select which of the frequencies will become the active
one.  By default its the first in the list.  The index refers
to the final list (either appended or replaced).

You can recover the current rest frequencies with
<link anchor="images:coordsys.restfrequency.function">restfrequency</link>. 

If the Coordinate System does not contain a frequency coordinate,
an exception is generated.

</description>

<example>
\begin{verbatim}
"""
#
print "\t----\t setrestfrequency Ex 1 \t----"
csys = cs.newcoordsys(spectral=T)
print csys.restfrequency()
#{'value': array([  1.42040575e+09]), 'unit': 'Hz'}
csys.setrestfrequency(qa.quantity('1.4GHz'))
print csys.restfrequency()
#{'value': array([  1.40000000e+09]), 'unit': 'Hz'}
csys.setrestfrequency(1.3e9)
print csys.restfrequency()
#{'value': array([  1.30000000e+09]), 'unit': 'Hz'}
csys.setrestfrequency (value=[1.2e9, 1.3e9], which=1)
print csys.restfrequency()
#{'value': array([  1.30000000e+09,   1.20000000e+09]), 'unit': 'Hz'}
csys.setrestfrequency (qa.quantity([1,2],'GHz'), which=3, append=T)
print csys.restfrequency()
#{'value': array([  2.00000000e+09,   1.20000000e+09,   1.30000000e+09,
#         1.00000000e+09]), 'unit': 'Hz'}
csys.setrestfrequency ("1.4E9Hz 1667MHz")
print csys.restfrequency()
#{'value': array([  1.40000000e+09,   1.66700000e+09]), 'unit': 'Hz'}
#
"""
\end{verbatim}
</example>
</method>

 
   <method type="function" name="setspectral">
   <shortdescription>Set tabular values for the spectral coordinate</shortdescription>
   
<input>
  
     <param xsi:type="string" direction="in" name="refcode">
     <description>Reference code.  Leave unset for no change.</description>
     <value></value>
     </param>
  
     <param xsi:type="any" direction="in" name="restfreq">
     <any type="variant"/>
     <description>Rest frequency. Leave unset for no change.</description>
     <value></value>
     </param>
  
     <param xsi:type="any" direction="in" name="frequencies">
     <any type="variant"/>
     <description>Vector of frequencies. Leave unset for no change.</description>
     <value>1GHz</value>
     </param>
  
     <param xsi:type="string" direction="in" name="doppler">
     <description>Doppler type. Leave unset for no change.</description>
     <value></value>
     </param>
  
     <param xsi:type="any" direction="in" name="velocities">
     <any type="variant"/>
     <description>Vector of velocities types. Leave unset for no change.</description>
     <value>1km/s</value>
     </param>
</input>
<returns xsi:type="bool"/>

<description>

When you construct a Coordsys \tool, if you include a Spectral Coordinate, it 
will be linear in frequency.  This function allows you to replace the
Spectral Coordinate by a finite table of values.  Coordinate
conversions between pixel and world are then done by interpolation.

You may specify either a vector of frequencies or velocities.
If you specify frequencies, you can optionally specify a (new)
reference code (see function <link anchor="images:coordsys.setreferencecode.function">setreferencecode</link>
for more details) and rest frequency (else the existing ones 
will be used).

If you specify velocities, you can optionally specify a (new)
reference code and rest frequency (else the existing ones 
will be used).  You must also give the doppler type 
(see function <link anchor="images:image.summary.function">summary</link> for
more details).   The velocities
are then converted to frequency for creation of the Spectral Coordinate
(which is fundamentally described by frequency).

You may specify the rest frequency as a Quantum or a double (native units
of Spectral Coordinate used).

</description>

<example>
\begin{verbatim}
"""
#
print "\t----\t setspectral Ex 1 \t----"
csys = cs.newcoordsys(spectral=T);
f1 = [1,1.01,1.03,1.4]
fq = qa.quantity(f1, 'GHz')
csys.setspectral(frequencies=fq)
v = csys.frequencytovelocity(f1, 'GHz', 'radio', 'km/s')
print 'v=', v
#v= [88731.317461076716, 86620.706055687479, 82399.483244909003, 4306.8612455073862]
vq = qa.quantity(v, 'km/s')
csys.setspectral(velocities=vq, doppler='radio')
f2 = csys.velocitytofrequency(v, 'GHz', 'radio', 'km/s')
print 'f1 = ', f1
#f1 =  [1, 1.01, 1.03, 1.3999999999999999]
print 'f2 = ', f2
#f2 =  [1.0, 1.01, 1.03, 1.3999999999999999]
#
"""
\end{verbatim}
We make a linear Spectral Coordinate.  Then overwrite it with
a list of frequenices.  Convert those values to velocity,
then overwrite the coordinate starting with a list of
velocities. Then convert the velocities to frequency
and show we get the original result.
</example>
</method>

 
   <method type="function" name="setstokes">
   <shortdescription>Set the Stokes types</shortdescription>
   
<input>
  
     <param xsi:type="stringArray" direction="in" name="stokes">
     <description>Stokes types</description>
     </param>
</input>
<returns xsi:type="bool"/>

<description>

If the Coordinate System contains a Stokes Coordinate, this function allows
you to change the Stokes types defining it.  If there is no Stokes
Coordinate, an exception is generated.

See the <link anchor="images:coordsys.coordsys.constructor">coordsys</link> constructor
to see the possible Stokes types you can set.

You can set the Stokes types with function
<link anchor="images:coordsys.setstokes.function">setstokes</link>. 

</description>

<example>
\begin{verbatim}
"""
#
print "\t----\t setstokes Ex 1 \t----"
csys = cs.newcoordsys(stokes="I V")
print csys.stokes()
#['I', 'V']
csys.setstokes("XX RL")
print csys.stokes()
#['XX', 'RL']
#
"""
\end{verbatim}
</example>
</method>

 
   <method type="function" name="settabular">
   <shortdescription>Set tabular values for the tabular coordinate</shortdescription>
   
<input>
  
	<param xsi:type="doubleArray" direction="in" name="pixel">
     <description>Vector of (0-rel) pixel values.  Default is no change.</description>
     <value>-1</value>
     </param>
  
     <param xsi:type="doubleArray" direction="in" name="world">
     <description>Vector of world values.  Default is no change.</description>
     <value>-1</value>
     </param>
  
     <param xsi:type="int" direction="in" name="which">
     <description>Which Tabular coordinate</description>
     <value>0</value>
     </param>
</input>
<returns xsi:type="bool"/>

<description>

When you construct a Coordsys \tool, if you include a Tabular
Coordinate, it will be linear.  This function allows you to replace the
Tabular Coordinate by a finite table of values.  Coordinate conversions
between pixel and world are then done by interpolation (or extrapolation
beyond the end).  The table of values must be at least of length 2
or an exception will occur.

You may specify a vector of pixel and world values (in the current units
of the Tabular Coordinate).  These vectors must be the same length.  If
you leave one of them unset, then the old values are used, but again,
ultimately, the pixel and world vectors must be the same length. 

The new reference pixel will be the first pixel value.
The new reference value will be the first world value.

Presently, there is no way for you to recover the lookup table
once you have set it.

If you have more than one Tabular Coordinate, use argument
{\stfaf which} to specify which one you want to modify.

</description>

<example>
\begin{verbatim}
"""
#
print "\t----\t settabular Ex 1 \t----"
csys = cs.newcoordsys(tabular=T);
print csys.settabular (pixel=[1,10,15,20,100], world=[10,20,50,100,500])
#True
#
"""
\end{verbatim}
We make a linear Tabular Coordinate.  Then overwrite it with
a non-linear list of pixel and world values.
</example>
</method>

 
   <method type="function" name="settelescope">
   <shortdescription>Set a new telescope</shortdescription>
   
<input>
  
     <param xsi:type="string" direction="in" name="value">
     <description>New telescope</description>
     </param>
</input>

<returns xsi:type="bool"/>

<description>

This function<!--(short-hand name {\stff st})--> sets a new telescope of the observation.   The telescope
position may be needed for reference code conversions; this is why it is
maintained in the Coordinate System.    So it is fundamental
to the Coordinate System and should be correct.

You can find a list of the observatory names know to \casa\ with the
Measures <link anchor="measures:measures.obslist.function">obslist</link> function.

You can  get the current telescope with function
<link anchor="images:coordsys.telescope.function">telescope</link>.   

</description>

<example>
\begin{verbatim}
"""
#
print "\t----\t settelescope Ex 1 \t----"
csys = cs.newcoordsys()
print csys.telescope()
#ATCA
csys.settelescope('VLA')
print csys.telescope()
#VLA
csys.settelescope('The One In My Backyard')
#Tue Mar 6 21:41:24 2007      WARN coordsys::settelescope:
#This telescope is not known to the casapy system
#You can request that it be added
print me.obslist()
#ALMA ARECIBO ATCA BIMA CLRO DRAO DWL GB GBT GMRT IRAM PDB IRAM_PDB
# JCMT MOPRA MOST NRAO12M NRAO_GBT PKS SAO SMA VLA VLBA WSRT
#
"""
\end{verbatim}
</example>
</method>

 
   <method type="function" name="setunits">
   <shortdescription>Set the axis units</shortdescription>
   
<input>
  
     <param xsi:type="stringArray" direction="in" name="value">
     <description>Units</description>
     </param>
  
     <param xsi:type="string" direction="in" name="type">
     <description>Coordinate type: "direction", "stokes", "spectral", "linear", "tabules" or leave unset for all.</description>
     <value></value>
     </param>
  
     <param xsi:type="bool" direction="in" name="overwrite">
     <description>Overwrite linear or tabular coordinate units?</description>
     <value>false</value>
     </param>
  
     <param xsi:type="int" direction="in" name="which">
     <description>Which coordinate if more than one of same type.  Default is first.</description>
     <value>-10</value>
     </param>
</input>
<returns xsi:type="bool"/>

<description>

Each axis associated with the Coordinate System has a unit. This
function<!--(short-hand name {\stff su})--> allows you to set new axis units.    

You can set the units either for all axes ({\stfaf
type=unset}) or for just the axes associated with a particular
coordinate type.

In general, the units must be consistent with the old units. When you
change the units, the increment and reference value will be adjusted
appropriately.  However, for a linear or tabular coordinate, and only
when you specify {\stfaf type='linear'} or {\stfaf type='tabular'} 
(i.e. you supply units only for the specified linear of tabular
coordinate), and if you set {\stfaf overwrite=T}, you can just overwrite
the units with no further adjustments.   Otherwise, the {\stfaf
overwrite} argument will be silently ignored.  Use argument
{\stfaf which} to specify which coordinate if you have more
than one of the specified type.

You can recover the current axis units with function
<link anchor="images:coordsys.units.function">units</link>. 

</description>
<example>
\begin{verbatim}
"""
#
print "\t----\t setunits Ex 1 \t----"
csys = cs.newcoordsys(direction=T, spectral=T)
csys.summary()
csys.setunits(value="deg rad mHz");
csys.summary()
#
"""
\end{verbatim}
</example>
</method>

 
   <method type="function" name="stokes">
   <shortdescription>Recover the Stokes types</shortdescription>
   

<returns xsi:type="stringArray"/>

<description>

If the Coordinate System contains a Stokes Coordinate, this function recovers the
Stokes types defining it.  If there is no Stokes
Coordinate, an exception is generated.

You can set the Stokes types with function
<link anchor="images:coordsys.setstokes.function">setstokes</link>. 

</description>

<example>
\begin{verbatim}
"""
#
print "\t----\t stokes Ex 1 \t----"
csys = cs.newcoordsys(stokes=['I','V'])
print csys.stokes()
#['I', 'V']
csys = cs.newcoordsys(stokes='Q U')
print csys.stokes()
#['Q', 'U']
#
"""
\end{verbatim}
</example>
</method>

 
   <method type="function" name="summary">
   <shortdescription>Summarize basic information about the Coordinate System</shortdescription>
   
<input>
  
     <param xsi:type="string" direction="in" name="doppler">
     <description>List velocity information with this doppler definition</description>
     <value>RADIO</value>
     </param>
  
     <param xsi:type="bool" direction="in" name="list">
     <description>List to global logger</description>
     <value>true</value>
     </param>
</input>
<returns xsi:type="stringArray"/>
<description>

This function<!--(short-hand name {\stff s})--> summarizes the information
contained in the Coordinate System. 
    
For spectral coordinates, the information is listed as a velocity as well as a
frequency.  The argument {\stfaf doppler} allows you to specify what
doppler convention it is listed in.  You can choose from {\stfaf radio,
optical} and {\stfaf beta}.  Alternative names are {\stfaf z} for
{\stfaf optical}, and {\stfaf relativistic} for {\stfaf
beta}.  The default is {\stfaf radio}.  The definitions are

\begin{itemize}
\item radio: $1 - F$
\item optical: $-1 + 1/F$
\item beta: $(1 - F^2)/(1 + F^2)$
\end{itemize}
where $F = \nu/\nu_0$ and $\nu_0$ is the rest frequency.  If the rest
frequency has not been set in your image, you can set it with
the function <link anchor="images:coordsys.setrestfrequency.function">setrestfrequency</link>.

These velocity definitions are provided by the <link anchor="measures:measures">measures</link>
system via the Doppler measure (see example).

If you  set {\stfaf list=F}, then the summary will not be written
to the global logger.     However, the return value will be a vector of strings
holding the summary information, one string per line of the summary.

For direction and spectral coordinates, the reference frame (e.g.  J2000
or LSRK) is also listed.  Along side this, in parentheses, will be the
conversion reference frame as well (if it is different from the native
reference frame).  See function
<link anchor="images:coordsys.setconversion.function">setconversion</link> to see what this
means. 

</description>

<example>
\begin{verbatim}
"""
#
print "\t----\t summary Ex 1 \t----"
d = me.doppler('beta')
print me.listcodes(d)
#[normal=RADIO Z RATIO BETA GAMMA OPTICAL TRUE RELATIVISTIC, extra=] 
csys = cs.newcoordsys(direction=T, spectral=T)
print csys.summary(list=F)
#
#Direction reference : J2000
#Spectral  reference : LSRK
#Velocity  type      : RADIO
#Rest frequency      : 1.42041e+09 Hz
#Telescope           : ATCA
#Observer            : Karl Jansky
#Date observation    : 2007/07/14/04:49:31
#
#Axis Coord Type      Name             Proj   Coord value at pixel    Coord incr Units
#-------------------------------------------------------------------------------------
#0    0     Direction Right Ascension   SIN  00:00:00.000     0.00 -6.000000e+01 arcsec
#1    0     Direction Declination       SIN +00.00.00.000     0.00  6.000000e+01 arcsec
#2    1     Spectral  Frequency                 1.415e+09     0.00  1.000000e+03 Hz
#                     Velocity                    1140.94     0.00 -2.110611e-01 km/s
#
#
"""
\end{verbatim}
</example>
</method>

 
   <method type="function" name="telescope">
   <shortdescription>Return the telescope</shortdescription>
   
<returns xsi:type="string"/>
  
<description>

This function<!--(short-hand name {\stff t})--> returns the telescope
contained in the Coordinate System <!--.  It can be returned --> as a
simple string. <!-- ({\stfaf measure=F}) or as a position measure
({\stfaf measure=T}).-->

The telescope position may be needed for reference code conversions; this is
why it is maintained in the Coordinate System.  

The conversion from string to position is done with
<link anchor="measures:measures.observatory.function">Measures observatory</link>.
The example shows how.  <!--what is really going on.-->

</description>

<example>
\begin{verbatim}
"""
#
print "\t----\t telescope Ex 1 \t----"
csys = cs.newcoordsys()
print csys.telescope()
#ATCA
print me.observatory(csys.telescope())
#{'type': 'position', 'refer': 'ITRF',
# 'm1': {'value': -0.5261379196128062, 'unit': 'rad'},
# 'm0': {'value': 2.6101423190348916, 'unit': 'rad'},
# 'm2': {'value': 6372960.2577234386, 'unit': 'm'}}
#
"""
\end{verbatim}

We get the telescope as a string. <!-- and as a position measure.-->
<!-- We also show how--> The Measures system is used to convert from
the simple name to a position Measure.

</example>
</method>

 
   <method type="function" name="toabs">
   <shortdescription>Convert relative coordinate to absolute</shortdescription>
   
<input>
 
     <param xsi:type="any" direction="in" name="value">
	     <any type="variant"/>
     <description>Relative coordinate</description>
     </param>
 
     <param xsi:type="int" direction="in" name="isworld">
     <description>Is coordinate world or pixel?  Default is unset.</description>
     <value>-1</value>
     </param>
</input>
<returns xsi:type="any"><any type="record"/></returns>

<description>

This function converts a relative coordinate to an absolute coordinate.
The coordinate may be a pixel coordinate or a world coordinate.

If the coordinate is a pixel coordinate, it is supplied as a numeric
vector. If the coordinate is a world coordinate,  you may give it in all
of the formats described in the
\htmlref{formatting}{COORDSYS:FORMATTING} discussion.

If the coordinate value is supplied by a Coordsys \tool\ function (e.g.
<link anchor="images:coordsys.toworld.function">toworld</link>) then the coordinate 'knows'
whether it is world or pixel (and absolute or relative). However, you
might supply the value from some other source as a numeric vector (which
could be world or pixel) in which case you must specify whether it is a 
world or pixel coordinate via the {\stfaf isworld} argument.

</description>

<example>
\begin{verbatim}
"""
#
print "\t----\t toabs Ex 1 \t----"
csys = cs.newcoordsys(direction=T, spectral=T)
aw = csys.toworld([100,100,24], 's')
rw = csys.torel(aw)
aw2 = csys.toabs(rw)
print aw
#{'ar_type': 'absolute', 'pw_type': 'world', 
# 'string': array(['23:53:19.77415678', '+01.40.00.84648186',
#                  '1.41502400e+09 Hz'], dtype='|S19')}
print rw
#{'ar_type': 'relative', 'pw_type': 'world',
# 'string': array(['-6.00084720e+03 arcsec', '6.00084648e+03 arcsec',
#                  '2.40000000e+04 Hz'], dtype='|S23')}
print aw2
#{'ar_type': 'absolute', 'pw_type': 'world',
# 'string': array(['23:53:19.77415672', '+01.40.00.84648000',
#                  '1.41502400e+09 Hz'], dtype='|S19')}
#
"""
\end{verbatim}

This example uses world coordinates.
</example>

<example>
\begin{verbatim}
"""
#
print "\t----\t toabs Ex 2 \t----"
csys = cs.newcoordsys(direction=T, spectral=T)
ap = csys.topixel()           # Reference value
rp = csys.torel(ap)
ap2 = csys.toabs(rp)
print ap
#{'ar_type': 'absolute', 'pw_type': 'pixel', 'numeric': array([ 0.,  0.,  0.])}
print rp
#{'ar_type': 'relative', 'pw_type': 'world',
#     'numeric': array([  0.00000000e+00,   0.00000000e+00,  -1.41500000e+09])}
print ap2
#{'ar_type': 'absolute', 'pw_type': 'world', 'numeric': array([ 0.,  0.,  0.])}
#
"""
\end{verbatim}
This example uses pixel coordinates.
</example>
</method>

 
   <method type="function" name="toabsmany">
   <shortdescription>Convert many numeric relative coordinates to absolute</shortdescription>
   
<input>
 
     <param xsi:type="any" direction="in" name="value">
           <any type="variant"/>
     <description>Relative coordinates</description>
     </param>
 
     <param xsi:type="int" direction="in" name="isworld">
     <description>Is coordinate world or pixel?  Default is unset.</description>
     <value>-1</value>
     </param>
</input>
<returns xsi:type="any"><any type="record"/></returns>

<description>

This function converts many relative coordinates to absolute. It exists
so you can efficiently make many conversions (which would be rather slow
if you did them all with <link anchor="images:coordsys.toabs.function">toabs</link>). Because
speed is the object, the interface is purely in terms of numeric
matrices, rather than being able to accept strings and quanta etc. like
<link anchor="images:coordsys.toabs.function">toabs</link> can.  

When dealing with world coordinates, the units of the numeric
values must be the native units, given by function
<link anchor="images:coordsys.units.function">units</link>.

</description>

<example>
\begin{verbatim}
"""
#
print "\t----\t toabsmany Ex 1 \t----"
csys = cs.newcoordsys(direction=T, spectral=T)    # 3 axes
rv = csys.referencevalue();                 # reference value
w = csys.torel(rv)                          # make relative
inc = csys.increment();                     # increment
off=[]
for idx in range(100):
  off.append(inc['numeric'][2]*idx)         # offset for third axis
wrel = ia.makearray(0,[3,100])              # 100 conversions each of length 3
for i in range(3):
  for j in range(100):
    wrel[i][j]=w['numeric'][i]
for j in range(100):
  wrel[2][j] += off[j]                      # Make spectral axis values change
wabs  = csys.toabsmany (wrel, T)['numeric'] # Convert
print wabs[0][0],wabs[1][0],wabs[2,0]       # First absolute coordinate
#0.0 0.0 1415000000.0
print wabs[0][99],wabs[1][99],wabs[2][99]   # 100th absolute coordinate
#0.0 0.0 1415099000.0
#
"""
\end{verbatim}
This example uses world coordinates.
</example>
</method>
 
   <method type="function" name="topixel">
   <shortdescription>Convert from absolute world to pixel coordinate</shortdescription>
   
<input>

     <param xsi:type="any" direction="in" name="value">
	     <any type="variant"/>
     <description>Absolute world coordinate</description>
     </param>
</input>
<returns xsi:type="any"><any type="record"/></returns>

<description>

This function converts between world (physical) coordinate and absolute pixel
coordinate (0-rel).

The world coordinate can be provided in one of four formats via the
argument {\stfaf world}.  These match the output formats of function
<link anchor="images:coordsys.toworld.function">toworld</link>.

If you supply fewer world values than there are axes in the  Coordinate
System, your coordinate vector will be padded out with the reference
value for the missing axes. Excess values will be silently ignored.

You may supply the world coordinate in all of the formats described in
the \htmlref{formatting}{COORDSYS:FORMATTING} discussion.

</description>

<example>
\begin{verbatim}
"""
#
print "\t----\t topixel Ex 1 \t----"
csys = cs.newcoordsys(direction=T, spectral=T, stokes="I V", linear=2)
w = csys.toworld([-2,2,1,2,23,24], 'n')
print csys.topixel(w)
#{'ar_type': 'absolute', 'pw_type': 'pixel',
# 'numeric': array([ -2.,   2.,   1.,   2.,  23.,  24.])}
w = csys.toworld([-2,2,1,2,23,24], 'q')
print csys.topixel(w)
#{'ar_type': 'absolute', 'pw_type': 'pixel',
# 'numeric': array([ -2.,   2.,   1.,   2.,  23.,  24.])}
w = csys.toworld([-2,2,1,2,23,24], 'm')
print csys.topixel(w)
#{'ar_type': 'absolute', 'pw_type': 'pixel',
# 'numeric': array([ -2.,   2.,   1.,   2.,  23.,  24.])}
w = csys.toworld([-2,2,1,2,23,24], 's')
print cs.topixel(w)
#{'ar_type': 'absolute', 'pw_type': 'pixel',
# 'numeric': array([ -2.,   2.,   1.,   2.,  23.,  24.])}
w = csys.toworld([-2,2,1,2,23,24], 'mnq')
print cs.topixel(w)                            
#{'ar_type': 'absolute', 'pw_type': 'pixel',
# 'numeric': array([ -2.,   2.,   1.,   2.,  23.,  24.])}
#
"""
\end{verbatim}
</example>

\goodbreak
<example>
\begin{verbatim}
"""
#
print "\t----\t topixel Ex 2 \t----"
csys = cs.newcoordsys (stokes="I V", linear=2)
print csys.toworld([0,1,2], 's')
#{'ar_type': 'absolute', 'pw_type': 'world',
# 'string': array(['I', '1.00000000e+00 km', '2.00000000e+00 km'],
#  dtype='|S18')}
print csys.toworld([0,1,2], 'm')
#{'ar_type': 'absolute', 'pw_type': 'world',
# 'measure': {'stokes': 'I', 'linear': {'*1': {'value': 1.0, 'unit': 'km'},
# '*2': {'value': 2.0, 'unit': 'km'}}}}
print csys.toworld([0,1,2], 'q')
#{'ar_type': 'absolute', 'pw_type': 'world',
# 'quantity': {'*1': {'value': 1.0, 'unit': ''},
# '*2': {'value': 1.0, 'unit': 'km'}, '*3': {'value': 2.0, 'unit': 'km'}}}
#
"""
\end{verbatim}
</example>

\goodbreak
<example>
\begin{verbatim}
"""
#
print "\t----\t topixel Ex 3 \t----"
csys = cs.newcoordsys (spectral=T, linear=1)
print csys.toworld([0,1,2], 'q')
#{'ar_type': 'absolute', 'pw_type': 'world',
# 'quantity': {'*1': {'value': 1415000000.0, 'unit': 'Hz'},
# '*2': {'value': 1.0, 'unit': 'km'}}}
#
"""
\end{verbatim}
</example>
</method>

 
   <method type="function" name="topixelmany">
   <shortdescription>Convert many absolute numeric world coordinates to pixel</shortdescription>
   
<input>
 
     <param xsi:type="any" direction="in" name="value">
	<any type="variant"/>
     <description>Absolute world coordinates</description>
     </param>
</input>
<returns xsi:type="any">
<any type="record"/>
</returns>

<description>

This function converts many absolute world coordinates to pixel coordinates. It exists
so you can efficiently make many conversions (which would be rather slow
if you did them all with <link anchor="images:coordsys.topixel.function">topixel</link>). Because
speed is the object, the interface is purely in terms of numeric
matrices, rather than being able to accept strings and quanta etc. like
<link anchor="images:coordsys.topixel.function">topixel</link> can.  

The units of the numeric values must be the native units, given by
function <link anchor="images:coordsys.units.function">units</link>.

</description>

<example>
\begin{verbatim}
"""
#
print "\t----\t topixelmany Ex 1 \t----"
csys = cs.newcoordsys(direction=T, spectral=T)    # 3 axes
rv = csys.referencevalue();                 # reference value
inc = csys.increment();                     # increment
off = []
for idx in range(100):
  off.append(inc['numeric'][2] * idx)       # offset for third axis
wabs = ia.makearray(0, [3,100])             # 100 conversions each of length 3
for i in range(3):
  for j in range(100):
    wabs[i][j]=rv['numeric'][i]
for j in range(100):
  wabs[2][j] += off[j]                      # Make spectral axis values change
pabs  = csys.topixelmany (wabs)['numeric']; # Convert
print pabs[0][0], pabs[1][0], pabs[1][2]    # First absolute pixel coordinate
#0.0 0.0 0.0
print pabs[0][99], pabs[1][99], pabs[2][99] # 100th absolute pixel coordinate
#0.0 0.0 99.0
#
"""
\end{verbatim}
</example>
</method>
 
   <method type="function" name="torecord">
   <shortdescription>Convert Coordinate System to a record</shortdescription>
   
   <returns xsi:type="any"><any type="record"/></returns>
  
<description>

You can convert a Coordinate System to a record with this function.
There is also <link anchor="images:coordsys.fromrecord.function">fromrecord</link>
to set a Coordinate System from a record.

These functions <!-- are not for general user use; they--> allow
Coordsys \tools\ to be used as parameters in the methods of other tools.
<!-- about the \glish\ bus.-->

</description>

<example>
\begin{verbatim}
"""
#
print "\t----\t torecord Ex 1 \t----"
csys = cs.newcoordsys(direction=T, stokes="I Q")
rec = csys.torecord();
cs2 = cs.newcoordsys();
print cs2.ncoordinates()
#0
cs2.fromrecord(rec);
print csys.ncoordinates(), cs2.ncoordinates()
#2 2
#
"""
\end{verbatim}
</example>
</method>

 
   <method type="function" name="torel">
   <shortdescription>Convert absolute coordinate to relative</shortdescription>
   
<input>
 
     <param xsi:type="any" direction="in" name="value">
	     <any type="variant"/>
     <description>Absolute coordinate</description>
     </param>
 
     <param xsi:type="int" direction="in" name="isworld">
     <description>Is coordinate world or pixel?  Default is unset.</description>
     <value>-1</value>
     </param>
</input>
<returns xsi:type="any"><any type="record"/></returns>

<description>

This function converts an absolute coordinate to a relative coordinate.
The coordinate may be a pixel coordinate or a world coordinate.

Relative coordinates are relative to the reference pixel (pixel coordinates)
or the reference value (world coordinates) in the sense 
$relative = absolute - reference$.

If the coordinate is a pixel coordinate, it is supplied as a numeric
vector. If the coordinate is a world coordinate,  you may give it in all
of the formats described in the
\htmlref{formatting}{COORDSYS:FORMATTING} discussion.

If the coordinate value is supplied by a Coordsys \tool\ function (e.g.
<link anchor="images:coordsys.toworld.function">toworld</link>) then the coordinate 'knows'
whether it is world or pixel (and absolute or relative). However, you
might supply the value from some other source as a numeric vector (which
could be world or pixel) in which case you must specify whether it is a 
world or pixel coordinate via the {\stfaf isworld} argument.

</description>

<example>
\begin{verbatim}
"""
#
print "\t----\t torel Ex 1 \t----"
csys = cs.newcoordsys(direction=T, spectral=T)
aw = csys.toworld([99,99,23], 's')
rw = csys.torel(aw)
aw2 = csys.toabs(rw)
print aw
#{'ar_type': 'absolute', 'pw_type': 'world',
# 'string': array(['23:53:23.78086843', '+01.39.00.82133427',
# '1.41502300e+09 Hz'], dtype='|S19')}
print rw
#{'ar_type': 'relative', 'pw_type': 'world',
# 'string': array(['-5.94082202e+03 arcsec', '5.94082133e+03 arcsec',
# '2.30000000e+04 Hz'], dtype='|S23')}
print aw2
#{'ar_type': 'absolute', 'pw_type': 'world',
# 'string': array(['23:53:23.78086818', '+01.39.00.82133000',
# '1.41502300e+09 Hz'], dtype='|S19')}
#
"""
\end{verbatim}

This example uses world coordinates.

\begin{verbatim}
"""
#
print "\t----\t torel Ex 2 \t----"
csys = cs.newcoordsys(direction=T, spectral=T)
ap = csys.topixel()           # Reference value
rp = csys.torel(ap)
ap2 = csys.toabs(rp)
print ap
#{'ar_type': 'absolute', 'pw_type': 'pixel', 'numeric': array([ 0.,  0.,  0.])}
print rp
#{'ar_type': 'relative', 'pw_type': 'pixel', 'numeric': array([ 0.,  0.,  0.])}
print ap2
#{'ar_type': 'absolute', 'pw_type': 'pixel', 'numeric': array([ 0.,  0.,  0.])}
#
"""
\end{verbatim}
This example uses pixel coordinates.
</example>
</method>

 
   <method type="function" name="torelmany">
   <shortdescription>Convert many numeric absolute coordinates to relative</shortdescription>
   
<input>
 
     <param xsi:type="any" direction="in" name="value">
           <any type="variant"/>
     <description>Absolute coordinates</description>
     </param>
 
     <param xsi:type="int" direction="in" name="isworld">
     <description>Is coordinate world or pixel?  Default is unset.</description>
     <value>-1</value>
     </param>
</input>
<returns xsi:type="any"><any type="record"/></returns>

<description>

This function converts many absolute coordinates to relative. It exists
so you can efficiently make many conversions (which would be rather slow
if you did them all with <link anchor="images:coordsys.torel.function">torel</link>). Because
speed is the object, the interface is purely in terms of numeric
matrices, rather than being able to accept strings and quanta etc. like
<link anchor="images:coordsys.torel.function">torel</link> can.  

When dealing with world coordinates, the units of the numeric
values must be the native units, given by function
<link anchor="images:coordsys.units.function">units</link>.

</description>

<example>
\begin{verbatim}
"""
#
print "\t----\t torelmany Ex 1 \t----"
csys = cs.newcoordsys(direction=T, spectral=T)    # 3 axes
w = csys.referencevalue();                  # reference value
inc = csys.increment();                     # increment
off = []
for idx in range(100):
  off.append(inc['numeric'][2] * idx)       # offset for third axis
wabs = ia.makearray(0, [3,100])             # 100 conversions each of length 3
for i in range(3):
  for j in range(100):
    wabs[i][j] = w['numeric'][i]
for j in range(100):
  wabs[2][j] += off[j]                      # Make spectral axis values change
wrel  = cs.torelmany (wabs, T)['numeric']   # Convert
print wrel[0][0], wrel[1][0], wrel[2][0]    # First relative coordinate
#0.0 0.0 0.0
print wrel[0][99], wrel[1][99], wrel[2][99] # 100th relative coordinate
#0.0 0.0 99000.0
#
"""
\end{verbatim}
This example uses world coordinates.
</example>

</method> 

   <method type="function" name="toworld">
   <shortdescription>Convert from absolute pixel coordinate to world</shortdescription>
   
<input>
 
     <param xsi:type="any" direction="in" name="value">
     <any type="variant"/>
     <description>Absolute pixel coordinate.  Default is reference pixel.</description>
     <value></value>
     </param>
 
     <param xsi:type="string" direction="in" name="format">
     <description>Format string: combination of "n", "q", "s", "m"</description>
     <value>n</value>
     </param>
</input>
<returns xsi:type="any"><any type="record"/></returns>

<description>

This function converts between absolute pixel coordinate (0-rel)
and absolute world (physical coordinate).

If you supply fewer pixel values than there are axes in the  Coordinate
System, your coordinate vector will be padded out with the reference
pixel for the missing axes. Excess values will be silently ignored.

You may ask for the world coordinate in all of the formats described in
the \htmlref{discussion}{COORDSYS:FORMATTING} regarding the
formatting possibilities available via argument {\stfaf format}.

</description>

<example>
\begin{verbatim}
"""
#
print "\t----\t toworld Ex 1 \t----"
csys = cs.newcoordsys(direction=T, spectral=T)
print csys.toworld([-3,1,1], 'n')
#{'ar_type': 'absolute', 'pw_type': 'world',
# 'numeric': array([  3.00000051e+00,   1.00000001e+00,   1.41500100e+09])}
print csys.toworld([-3,1,1], 'q')
#{'ar_type': 'absolute', 'pw_type': 'world',
# 'quantity': {'*1': {'value': 3.0000005076962117, 'unit': "'"},
#              '*2': {'value': 1.0000000141027674, 'unit': "'"},
#              '*3': {'value': 1415001000.0, 'unit': 'Hz'}}}
print csys.toworld([-3,1,1], 'm')
#{'ar_type': 'absolute', 'pw_type': 'world', 'measure':
# {'spectral': {'radiovelocity': {'type': 'doppler', 'm0': {'value': 1140733.0762829871, 'unit': 'm/s'}, 'refer': 'RADIO'},
#             'opticalvelocity': {'type': 'doppler', 'm0': {'value': 1145090.2316004676, 'unit': 'm/s'}, 'refer': 'OPTICAL'},
#                 'frequency': {'type': 'frequency', 'm0': {'value': 1415001000.0, 'unit': 'Hz'}, 'refer': 'LSRK'},
#                'betavelocity': {'type': 'doppler', 'm0': {'value': 1142903.3485169839, 'unit': 'm/s'}, 'refer': 'TRUE'}},
# 'direction': {'type': 'direction', 'm1': {'value': 0.0002908882127680503, 'unit': 'rad'},
#                                    'm0': {'value': 0.00087266477368000634, 'unit': 'rad'}, 'refer': 'J2000'}}}
print csys.toworld([-3,1,1], 's')
#{'ar_type': 'absolute', 'pw_type': 'world',
# 'string': array(['00:00:12.00000203', '+00.01.00.00000085', '1.41500100e+09 Hz'], dtype='|S19')}
#
"""
\end{verbatim}
</example>

\goodbreak
<example>
\begin{verbatim}
"""
#
print "\t----\t toworld Ex 2 \t----"
csys = cs.newcoordsys (stokes="I V", linear=2)
print csys.toworld([0,1,2], 's')
#{'ar_type': 'absolute', 'pw_type': 'world',
# 'string': array(['I', '1.00000000e+00 km', '2.00000000e+00 km'],
#      dtype='|S18')}
print csys.toworld([0,1,2], 'm')
#{'ar_type': 'absolute', 'pw_type': 'world',
# 'measure': {'stokes': 'I', 'linear': {'*1': {'value': 1.0, 'unit': 'km'},
#                                       '*2': {'value': 2.0, 'unit': 'km'}}}}
print csys.toworld([0,1,2], 'q')
#{'ar_type': 'absolute', 'pw_type': 'world',
# 'quantity': {'*1': {'value': 1.0, 'unit': ''},
#              '*2': {'value': 1.0, 'unit': 'km'},
#              '*3': {'value': 2.0, 'unit': 'km'}}}
#
"""
\end{verbatim}
</example>

\goodbreak
<example>
\begin{verbatim}
"""
#
print "\t----\t toworld Ex 3 \t----"
csys = cs.newcoordsys (spectral=T, linear=1)
print cs.toworld([0,1,2], 'q')
#{'ar_type': 'absolute', 'pw_type': 'world',
# 'quantity': {'*1': {'value': 1415000000.0, 'unit': 'Hz'},
#              '*2': {'value': 1.0, 'unit': 'km'}}}
#
"""
\end{verbatim}
</example>
</method>

 
   <method type="function" name="toworldmany">
   <shortdescription>Convert many absolute pixel coordinates to numeric world</shortdescription>
   
<input>
 
     <param xsi:type="any" direction="in" name="value">
	<any type="variant"/>
     <description>Absolute pixel coordinates</description>
     </param>
</input>
<returns xsi:type="any"> <any type="record"/> </returns>

<description>

This function converts many absolute pixel coordinates to world coordinates. It exists
so you can efficiently make many conversions (which would be rather slow
if you did them all with <link anchor="images:coordsys.toworld.function">toworld</link>). Because
speed is the object, the interface is purely in terms of numeric
matrices, rather than being able to produce strings and quanta etc. like
<link anchor="images:coordsys.toworld.function">toworld</link> can.  

The units of the output world values are the native units given by
function <link anchor="images:coordsys.units.function">units</link>.

</description>

<example>
\begin{verbatim}
"""
#
print "\t----\t toworldmany Ex 1 \t----"
csys = cs.newcoordsys(direction=T, spectral=T)    # 3 axes
rp = csys.referencepixel()['numeric'];            # reference pixel
pabs = ia.makearray(0,[3,100])              # 100 conversions each of length 3
for i in range(3):
  for j in range(100):
    pabs[i][j] = rp[i]
for ioff in range(100):                     # offset for third axis
  pabs[2][ioff] += ioff;                    # Make spectral axis values change
wabs  = csys.toworldmany (pabs)['numeric']; # Convert
print wabs[0][0], wabs[1][0], wabs[2][0]    # First absolute pixel coordinate
#0.0 0.0 1415000000.0
print wabs[0][99], wabs[1][99], wabs[2][99] # 100th absolute pixel coordinate
#0.0 0.0 1415099000.0
#
"""
\end{verbatim}
</example>
</method>
 
   <method type="function" name="type">
   <shortdescription>Return the type of this tool</shortdescription>
   
<returns xsi:type="string"/>
  
<description>

This function returns the string `coordsys'.

</description>
</method>

 
   <method type="function" name="units">
   <shortdescription>Recover the units for each axis</shortdescription>
   
<input>
  
     <param xsi:type="string" direction="in" name="type">
     <description>Coordinate type: 'direction', 'stokes', 'spectral', 'linear' or leave unset for all</description>
     <value></value>
     </param>
</input>

<returns xsi:type="stringArray"/>

<description>

Each axis associated with the Coordinate System has a unit.
This function<!--(short-hand name {\stff u})--> returns those units
(in world axis order).

You can recover the units either for all coordinates (leave {\stfaf
type} unset) or for a specific coordinate type (mimumum match of the
allowed types will do).  If you ask for a non-existent coordinate an
exception is generated.

You can set the units with function
<link anchor="images:coordsys.setunits.function">setunits</link>.

</description>

<example>  
\begin{verbatim}
"""
#
print "\t----\t units Ex 1 \t----"
csys = cs.newcoordsys(direction=T, spectral=T)
print csys.units()
#["'", "'", 'Hz']
print csys.units('spec')
#Hz
#
"""
\end{verbatim}
</example>
</method>

 
   <method type="function" name="velocitytofrequency">
   <shortdescription>Convert velocity to frequency</shortdescription>
   
<input>  
  
     <param xsi:type="doubleArray" direction="in" name="value">
     <description>Velocity to convert</description>
     </param>
  
     <param xsi:type="string" direction="in" name="frequnit">
     <description>Unit of output frequencies.  Default is intrinisic units.</description>
     <value></value>
     </param>
  
     <param xsi:type="string" direction="in" name="doppler">
     <description>Velocity doppler definition</description>
     <value>radio</value>
     </param>   
  
     <param xsi:type="string" direction="in" name="velunit">
     <description>Unit of input velocities</description>
     <value>km/s</value>
     </param>  
</input>    
<returns xsi:type="doubleArray"/>
  
<description>

This function<!--(short-hand name {\stf vtf})--> converts velocities
to frequencies.

The input velocities are specified via a vector of numeric values, a
specified unit ({\stfaf velunit}), and a  velocity doppler definition ({\stfaf
doppler}).

The frequencies are returned in a vector for which you specify the
units ({\stfaf frequnit}).  If you don't give the unit, it is assumed that 
the units are those given by function <link anchor="images:coordsys.units.function">units</link> 
for the spectral coordinate.

This function will return a fail if there is no spectral coordinate
in the Coordinate System. See also the function
<link anchor="images:coordsys.frequencytovelocity.function">frequencytovelocity</link>.

</description>

<example>
\begin{verbatim}
"""
#
print "\t----\t velocitytofrequency Ex 1 \t----"
ia.fromshape('hcn.cube',[64,64,32,4], overwrite=true)
csys = ia.coordsys()
rtn = csys.findcoordinate('spectral')   # Find spectral axis
pixel = csys.referencepixel();          # Use reference pixel for non-spectral
pa = rtn['pixel']
wa = rtn['world']
nFreq = ia.shape()[pa]                  # Length of spectral axis
freq = []
for i in range(nFreq):
  pixel[pa] = i;                        # Assign value for spectral axis of pixel coordinate
  w = csys.toworld(value=pixel, format='n')# Convert pixel to world
  freq.append(w['numeric'][wa])         # Fish out frequency
print "freq=", freq
vel = csys.frequencytovelocity(value=freq, doppler='optical', velunit='km/s')
freq2 = csys.velocitytofrequency(value=vel, doppler='optical', velunit='km/s')
print "vel=",vel
print "freq2=",freq2
csys.done()
#
exit() # This is last example so exit casapy if you wish.
#
"""
\end{verbatim}

In this example, we find the optical velocity in km/s of every pixel
along the spectral axis of our image.  First we  obtain the Coordinate
System from the image.  Then we find which axis of the Coordinate System
(image) pertain to the spectral coordinate.  Then we loop over each
pixel of the spectral axis, and convert a pixel coordinate (one for each
axis of the image) to world.  We obtain the value for the spectral axis
from that world vector, and add it to the vector of frequencies.  Then
we convert that vector of frequencies to velocity.  Then we convert it
back to frequency.  They better agree.

</example>
</method>


   <method type="function" name="parentname">
   <shortdescription>Get parent image name.</shortdescription>
   
<returns xsi:type="string"/>
  
<description>

This function returns the parent image name for `coordsys'.

</description>
</method>

 
   <method type="function" name="setparentname">
   <shortdescription>Set the parent image name (normally not needed by end-users)</shortdescription>
   
<input>
  
     <param xsi:type="string" direction="in" name="imagename">
     <description>String named parent image</description>
     </param>
</input>

<returns xsi:type="bool"/>

</method>

 


</tool>
</casaxml>
"""
