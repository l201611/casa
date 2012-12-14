      subroutine faccumulateFromGrid(nvalue, grid, convFuncV,
     $     wVal, scaledSupport, scaledSampling, 
     $     off, convOrigin, cfShape, loc,
     $     igrdpos,  sinDPA, cosDPA,
     $     finitePointingOffset,
     $     phaseGrad,
     $     phasor,
     $     imNX, imNY, imNP, imNC,
     $     cfNX, cfNY, cfNP, cfNC,
     $     phNX, phNY)

      implicit none
      integer imNX, imNY, imNC, imNP,
     $     vNX, vNY, vNC, vNP,
     $     cfNX, cfNY, cfNP, cfNC,
     $     phNX, phNY

      complex grid(imNX, imNY, imNP, imNC)
      complex convFuncV(cfNX, cfNY, cfNP, cfNC)
      complex nvalue
      double precision wVal
      integer scaledSupport(2)
      real scaledSampling(2)
      double precision off(2)
      integer convOrigin(4), cfShape(4), loc(4), igrdpos(4)
      double precision sinDPA, cosDPA
      integer finitePointingOffset
      double precision norm
      complex phaseGrad(phNX, phNY),phasor

      integer l_phaseGradOriginX, l_phaseGradOriginY
      integer iloc(4), iCFPos(4)
      complex wt
      integer ix,iy
      integer l_igrdpos(4)

      data iloc/1,1,1,1/, iCFPos/1,1,1,1/
      l_igrdpos(3) = igrdpos(3)+1
      l_igrdpos(4) = igrdpos(4)+1
      norm=0.0
      l_phaseGradOriginX=phNX/2 + 1
      l_phaseGradOriginY=phNY/2 + 1


      do iy=-scaledSupport(2),scaledSupport(2) 
         iloc(2)=nint(scaledSampling(2)*iy+off(2))
         iCFPos(2)=iloc(2)+convOrigin(2)+1
         l_igrdpos(2) = loc(2)+iy+1
         do ix=-scaledSupport(1),scaledSupport(1)
            iloc(1)=nint(scaledSampling(1)*ix+off(1))
            iCFPos(1) = iloc(1) + convOrigin(1) + 1
            l_igrdpos(1) = loc(1) + ix + 1
            
            wt = convFuncV(iCFPos(1), iCFPos(2), 
     $           iCFPos(3), iCFPos(4))
            if (wVal > 0.0)  wt = conjg(wt)

            norm = norm + real(wt)

            if (finitePointingOffset .eq. 1) then
               wt = wt * phaseGrad(iloc(1) + l_phaseGradOriginX, 
     $              iloc(2) + l_phaseGradOriginY)
            endif

            nvalue = nvalue + wt *grid(l_igrdpos(1), l_igrdpos(2), 
     $           l_igrdpos(3), l_igrdpos(4))
         enddo
      enddo
      nvalue = nvalue *conjg(phasor)/norm
      end
