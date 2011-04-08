      SUBROUTINE iau_PVDPV ( A, B, ADB )
*+
*  - - - - - - - - - -
*   i a u _ P V D P V
*  - - - - - - - - - -
*
*  Inner (=scalar=dot) product of two pv-vectors.
*
*  This routine is part of the International Astronomical Union's
*  SOFA (Standards of Fundamental Astronomy) software collection.
*
*  Status:  vector/matrix support routine.
*
*  Given:
*     A        d(3,2)      first pv-vector
*     B        d(3,2)      second pv-vector
*
*  Returned:
*     ADB      d(2)        A . B (see note)
*
*  Note:
*
*     If the position and velocity components of the two pv-vectors are
*     ( Ap, Av ) and ( Bp, Bv ), the result, A . B, is the pair of
*     numbers ( Ap . Bp , Ap . Bv + Av . Bp ).  The two numbers are the
*     dot-product of the two p-vectors and its derivative.
*
*  Called:
*     iau_PDP      inner product of two p-vectors
*
*  This revision:  2003 January 14
*
*  Copyright (C) 2005 IAU SOFA Review Board.  See notes at end.
*
*-----------------------------------------------------------------------

      IMPLICIT NONE

      DOUBLE PRECISION A(3,2), B(3,2), ADB(2)

      DOUBLE PRECISION ADBD, ADDB

* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

*  A . B = constant part of result.
      CALL iau_PDP ( A(1,1), B(1,1), ADB(1) )

*  A . Bdot
      CALL iau_PDP ( A(1,1), B(1,2), ADBD )

*  Adot . B
      CALL iau_PDP ( A(1,2), B(1,1), ADDB )

*  Velocity part of result.
      ADB(2) = ADBD + ADDB

*  Finished.

*+----------------------------------------------------------------------
*
*  Copyright (C) 2005
*  Standards Of Fundamental Astronomy Review Board
*  of the International Astronomical Union.
*
*  =====================
*  SOFA Software License
*  =====================
*
*  NOTICE TO USER:
*
*  BY USING THIS SOFTWARE YOU ACCEPT THE FOLLOWING TERMS AND CONDITIONS
*  WHICH APPLY TO ITS USE.
*
*  1. The Software is owned by the IAU SOFA Review Board ("the Board").
*
*  2. The Software is made available free of charge for use by:
*
*     a) private individuals for non-profit research; and
*
*     b) non-profit educational, academic and research institutions.
*
*  3. Commercial use of the Software is specifically excluded from the
*     terms and conditions of this license.  Commercial use of the
*     Software is subject to the prior written agreement of the Board on
*     terms to be agreed.
*
*  4. The provision of any version of the Software under the terms and
*     conditions specified herein does not imply that future versions
*     will also be made available under the same terms and conditions.
*
*  5. The user may modify the Software for his/her own purposes.  The
*     user may distribute the modified software provided that the Board
*     is informed and that a copy of the modified software is made
*     available to the Board on request.  All modifications made by the
*     user shall be clearly identified to show how the modified software
*     differs from the original Software, and the name(s) of the
*     affected routine(s) shall be changed.  The original SOFA Software
*     License text must be present.
*
*  6. In any published work produced by the user and which includes
*     results achieved by using the Software, the user shall acknowledge
*     that the Software was used in producing the information contained
*     in such publication.
*
*  7. The user may incorporate or embed the Software into other software
*     products which he/she may then give away free of charge but not
*     sell provided the user makes due acknowledgement of the use which
*     he/she has made of the Software in creating such software
*     products.  Any redistribution of the Software in this way shall be
*     made under the same terms and conditions under which the user
*     received it from the SOFA Center.
*
*  8. The user shall not cause the Software to be brought into
*     disrepute, either by misuse, or use for inappropriate tasks, or by
*     inappropriate modification.
*
*  9. The Software is provided to the user "as is" and the Board makes
*     no warranty as to its use or performance.   The Board does not and
*     cannot warrant the performance or results which the user may
*     obtain by using the Software.  The Board makes no warranties,
*     express or implied, as to non-infringement of third party rights,
*     merchantability, or fitness for any particular purpose.  In no
*     event will the Board be liable to the user for any consequential,
*     incidental, or special damages, including any lost profits or lost
*     savings, even if a Board representative has been advised of such
*     damages, or for any claim by any third party.
*
*  Correspondence concerning SOFA software should be addressed as
*  follows:
*
*     Internet email: sofa@rl.ac.uk
*     Postal address: IAU SOFA Center
*                     Rutherford Appleton Laboratory
*                     Chilton, Didcot, Oxon OX11 0QX
*                     United Kingdom
*
*
*-----------------------------------------------------------------------

      END
