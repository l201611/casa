import shutil
import unittest
import os
import filecmp
import numpy
from tasks import *
from taskinit import *
from __main__ import default
"""
Unit tests for task setjy.

Features tested:
  1. Does setjy(modimage=modelimu, fluxdensity=0) NOT scale the model image's
     flux density?
  2. Does setjy(modimage=modelimu) scale the model image's flux density?
  3. Solar system (Uranus) flux density calibration.
"""

datapath = os.environ.get('CASAPATH').split()[0] + '/data/regression/'

class SetjyUnitTestBase(unittest.TestCase):

    def setUpMS(self,MS):

        self.inpms = MS

        # Create working area
        if not os.path.exists('unittest/setjy'):
            print "\nCreate working area..."
            os.system('mkdir -p unittest/setjy')

        # Create a new fresh copy of the MS
        print "\nCreate a new local copy of the MS..."
        os.system('cp -rf ' + os.environ.get('CASAPATH').split()[0] + "/data/regression/" + self.inpms + ' unittest/setjy/')

    def resetMS(self):

        if os.path.exists(self.inpms):
            print "\nRemove local copy of MS from previous test..."
            ret = os.system('rm -rf unittest/setjy/*')

    def get_last_history_line(self,vis, origin='setjy::imager::setjy()',
                              nback=0, maxnback=20, hint=''):
        """
        Finding the right history line is a bit tricky...it helps to filter
        by origin and read from the back to remain unaffected by changes
        elsewhere.
        
        This reads up to maxnback lines with origin origin until it finds one
        including hint in the message, going backwards from nback lines from the
        end.
        
        Returns 'JUNK' on failure.
        """
        retline = 'JUNK'
        try:
            tblocal = tbtool()
            tblocal.open(vis + '/HISTORY')
            st = tblocal.query('ORIGIN == "%s"' % origin, columns='MESSAGE')
            nstrows = st.nrows()
            startrow = st.nrows() - 1 - nback
            # don't go back more than selected rows
            if maxnback > nstrows:
                maxnback = nstrows - 1
            stoprow = startrow - maxnback
            print hint
            for linenum in xrange(startrow, stoprow - 1, -1):
                curline = st.getcell('MESSAGE', linenum)
                if hint in curline:
                    retline = curline
                    break
            st.close()
            tblocal.close()
        except Exception, e:
            print "\nError getting last history line"
            tblocal.close()
            raise e

        return retline

    def check_history(self,histline, items):
        isok = True
        for item in items:
            if item not in histline:
                isok = False
                break
        if not isok:
            errmsg = "%s not found in %s.\n" % (items, histline)
            errmsg += "It could be that a change to HISTORY caused the wrong line to be selected."
            raise AssertionError, errmsg
        return isok

    def check_eq(self,val, expval, tol=None):
        """Checks that val matches expval within tol."""
        if type(val) == dict:
            for k in val:
                check_eq(val[k], expval[k], tol)
        else:
            try:
                if tol and hasattr(val, '__rsub__'):
                    are_eq = abs(val - expval) < tol
                else:
                    are_eq = val == expval
                if hasattr(are_eq, 'all'):
                    are_eq = are_eq.all()
                if not are_eq:
                    raise ValueError, '!='
            except ValueError:
                errmsg = "%r != %r" % (val, expval)
                if (len(errmsg) > 66): # 66 = 78 - len('ValueError: ')
                    errmsg = "\n%r\n!=\n%r" % (val, expval)
                raise ValueError, errmsg
            except Exception, e:
                print "Error comparing", val, "to", expval
                raise e


class test_SingleObservation(SetjyUnitTestBase):
    """Test single observation MS"""

    def setUp(self):
        self.setUpMS("unittest/setjy/2528.ms")         # Uranus

    def tearDown(self):
        self.resetMS()

    def test1_SingleObservationOldModel(self):
        """ Test vs an MS with one single observation using the Butler-JPL-Horizons 2010 model"""

        os.system("mv " + self.inpms + " " + self.inpms + ".test1")
        self.inpms += ".test1"
        record = {}

        tblocal = tbtool()
        tblocal.open(self.inpms)
        cols = tblocal.colnames()
        tblocal.close()
        if 'MODEL_DATA' in cols:
            raise ValueError, "The input MS, " + self.inpms + " already has a MODEL_DATA col" + str(cols)

        try:
            print "\nRunning setjy(field='Uranus')."
            sjran = setjy(vis=self.inpms, field='Uranus', spw='', modimage='',
                          scalebychan=False, fluxdensity=-1,
                          standard='Butler-JPL-Horizons 2010', usescratch=True)
        except Exception, e:
            print "\nError running setjy(field='Uranus')"
            raise e

        try:
            tblocal.open(self.inpms)
            cols = tblocal.colnames()
            if 'MODEL_DATA' not in cols:
                raise AssertionError, "setjy(field='Uranus') did not add a MODEL_DATA column"
            else:
                record['wvr'] = tblocal.getcell('MODEL_DATA', 0)
                record['auto3'] = tblocal.getcell('MODEL_DATA', 10)
                record['long3'] = tblocal.getcell('MODEL_DATA', 11)
                record['auto4'] = tblocal.getcell('MODEL_DATA', 2)
                record['med4'] = tblocal.getcell('MODEL_DATA', 4)
                record['long4'] = tblocal.getcell('MODEL_DATA', 3)
                tblocal.close()
                record['history'] = self.get_last_history_line(self.inpms, hint='Uranus')
                self.result = record
        except AssertionError, e:
            print "\nError accesing MODEL_DATA"
            tblocal.close()
            raise e

        """Flux density in HISTORY (Uranus)?"""
        self.check_history(self.result['history'], ["Uranus", "V=0] Jy"])
        """WVR spw"""
        self.check_eq(self.result['wvr'], numpy.array([[26.40653229+0.j,26.40653229+0.j]]),0.0001)
        """Zero spacing of spw 3"""
        self.check_eq(self.result['auto3'], numpy.array([[65.80638885+0.j],[65.80638885+0.j]]),0.0001)
        """Long spacing of spw 3"""
        self.check_eq(self.result['long3'], numpy.array([[4.76111794+0.j],[4.76111794+0.j]]),0.0001)
        """Zero spacing of spw 4"""
        self.check_eq(self.result['auto4'], numpy.array([[69.33396912+0.j],[69.33396912+0.j]]),0.0001)
        """Medium spacing of spw 4"""
        self.check_eq(self.result['med4'], numpy.array([[38.01076126+0.j],[38.01076126+0.j]]),0.0001)
        """Long spacing of spw 4"""
        self.check_eq(self.result['long4'], numpy.array([[2.83933783+0.j],[2.83933783+0.j]]),0.0001)

        return sjran

    def test2_SingleObservationSelectByChan(self):
        """ Test vs an MS with one single observation using the Butler-JPL-Horizons 2010 model and selecting by channel"""

        os.system("mv " + self.inpms + " " + self.inpms + ".test2")
        self.inpms += ".test2"
        record = {}

        tblocal = tbtool()
        tblocal.open(self.inpms)
        cols = tblocal.colnames()
        tblocal.close()
        if 'MODEL_DATA' in cols:
            raise ValueError, "The input MS, " + self.inpms + " already has a MODEL_DATA col" + str(cols)

        try:
            print "\nRunning setjy(field='Uranus')."
            sjran = setjy(vis=self.inpms, field='Uranus', spw='', modimage='',
                          scalebychan=True, fluxdensity=-1,
                          standard='Butler-JPL-Horizons 2010', usescratch=True)
        except Exception, e:
            print "\nError running setjy(field='Uranus')"
            raise e
        try:
            tblocal.open(self.inpms)
            cols = tblocal.colnames()
            if 'MODEL_DATA' not in cols:
                raise AssertionError, "setjy(field='Uranus') did not add a MODEL_DATA column"
            else:
                record['wvr'] = tblocal.getcell('MODEL_DATA', 0)
                record['auto1'] = tblocal.getcell('MODEL_DATA', 18)
                record['long1'] = tblocal.getcell('MODEL_DATA', 19)
                record['auto4'] = tblocal.getcell('MODEL_DATA', 2)
                record['long4'] = tblocal.getcell('MODEL_DATA', 3)
                tblocal.close()
                record['history'] = self.get_last_history_line(self.inpms, hint="V=0] Jy")
                self.result = record
        except AssertionError, e:
            print "\nError accesing MODEL_DATA"
            tblocal.close()
            raise e

        """Flux density in HISTORY (scalebychan)?"""
        self.check_history(self.result['history'], ["Uranus", "V=0] Jy"])
        """WVR spw with scalebychan"""
        self.check_eq(self.result['wvr'], numpy.array([[25.93320656+0.j,
                                                        26.88228607+0.j]]),
                 0.003)
        """Zero spacing of spw 1 with scalebychan"""
        # 8 (decreasing freq!) chans, XX & YY.
        self.check_eq(self.result['auto1'],
                 numpy.array([[65.49415588+0.j, 65.42105865+0.j,
                               65.34798431+0.j, 65.27491760+0.j,
                               65.20187378+0.j, 65.12883759+0.j,
                               65.05581665+0.j, 64.98281097+0.j],
                              [65.49415588+0.j, 65.42105865+0.j,
                               65.34798431+0.j, 65.27491760+0.j,
                               65.20187378+0.j, 65.12883759+0.j,
                               65.05581665+0.j, 64.98281097+0.j]]),0.0001)
        """Long spacing of spw 1 with scalebychan"""
        self.check_eq(self.result['long1'],
                 numpy.array([[4.92902184+0.j, 4.96826363+0.j,
                               5.00747252+0.j, 5.04664850+0.j,
                               5.08579159+0.j, 5.12490082+0.j,
                               5.16397619+0.j, 5.20301771+0.j],
                              [4.92902184+0.j, 4.96826363+0.j,
                               5.00747252+0.j, 5.04664850+0.j,
                               5.08579159+0.j, 5.12490082+0.j,
                               5.16397619+0.j, 5.20301771+0.j]]),0.0001)
        # spw 4 only has 1 chan, so it should be the same as without scalebychan.
        """Zero spacing of spw 4 with scalebychan"""
        self.check_eq(self.result['auto4'], numpy.array([[69.33396912+0.j],[69.33396912+0.j]]),0.0001)
        """Long spacing of spw 4 with scalebychan"""
        self.check_eq(self.result['long4'], numpy.array([[2.83933783+0.j],[2.83933783+0.j]]),0.0001)

        return sjran

    def test3_SingleObservationNewModel(self):
        """ Test vs an MS with one single observation using the Butler-JPL-Horizons 2012 model"""

        # print out some values for debugging
        debug=False

        os.system("mv " + self.inpms + " " + self.inpms + ".test3")
        self.inpms += ".test3"
        record = {}

        tblocal = tbtool()
        tblocal.open(self.inpms)
        cols = tblocal.colnames()
        tblocal.close()
        if 'MODEL_DATA' in cols:
            raise ValueError, "The input MS, " + self.inpms + " already has a MODEL_DATA col" + str(cols)

        try:
            print "\nRunning setjy(field='Uranus')."
            sjran = setjy(vis=self.inpms, field='Uranus', spw='', modimage='',
                          scalebychan=False, fluxdensity=-1,
                          standard='Butler-JPL-Horizons 2012', usescratch=True)
        except Exception, e:
            print "\nError running setjy(field='Uranus')"
            raise e
        try:
            tblocal.open(self.inpms)
            cols = tblocal.colnames()
            if 'MODEL_DATA' not in cols:
                raise AssertionError, "setjy(field='Uranus') did not add a MODEL_DATA column"
            else:
                record['wvr'] = tblocal.getcell('MODEL_DATA', 0)
                record['auto3'] = tblocal.getcell('MODEL_DATA', 10)
                record['long3'] = tblocal.getcell('MODEL_DATA', 11)
                record['auto4'] = tblocal.getcell('MODEL_DATA', 2)
                record['med4'] = tblocal.getcell('MODEL_DATA', 4)
                record['long4'] = tblocal.getcell('MODEL_DATA', 3)
                tblocal.close()
                record['history'] = self.get_last_history_line(self.inpms, origin='setjy', hint='Uranus')
                self.result = record
        except AssertionError, e:
            print "\nError accesing MODEL_DATA"
            tblocal.close()
            raise e

        if debug:
          print "self.result['history']=",self.result['history']
          print "self.result['wvr']=",self.result['wvr']
          print "self.result['auto3']=",self.result['auto3']
          print "self.result['auto4']=",self.result['auto4']

        """Flux density in HISTORY (Uranus)?"""
        self.check_history(self.result['history'], ["Uranus:", "V=0.0] Jy"])
        """WVR spw"""
        #self.check_eq(self.result['wvr'], numpy.array([[ 25.33798409+0.j,25.33798409+0.j]]),0.0001)
        # new value after code and ephemeris data update 2012-10-03
        self.check_eq(self.result['wvr'], numpy.array([[ 25.31591988+0.j,  25.31591988+0.j]]),0.0001)
        """Zero spacing of spw 3"""
	#self.check_eq(self.result['auto3'], numpy.array([[ 66.72530365+0.j],[ 66.72530365+0.j]]),0.0001)
        # new value after code and ephemeris data update 2012-10-03
	self.check_eq(self.result['auto3'], numpy.array([[ 66.81645203+0.j], [ 66.81645203+0.j]]),0.0001)
        """Zero spacing of spw 4"""
        #self.check_eq(self.result['auto4'], numpy.array([[ 70.40153503+0.j],[ 70.40153503+0.j]]),0.0001)
        # new value after code and ephemeris data update 2012-10-03
        self.check_eq(self.result['auto4'], numpy.array([[ 70.48959351+0.j], [ 70.48959351+0.j]]), 0.0001)

        return sjran


class test_MultipleObservations(SetjyUnitTestBase):
    """Test multiple observation MS (CAS-3320)"""

    def setUp(self):
        self.setUpMS("unittest/setjy/multiobs.ms")         # Titan

    def tearDown(self):
        self.resetMS()

    def test1_MultipleObservationOldModel(self):
        """ Test vs an MS with multiple observations using the Butler-JPL-Horizons 2010 model"""

        os.system("mv " + self.inpms + " " + self.inpms + ".test1")
        self.inpms += ".test1"
        record = {}

        tblocal = tbtool()
        tblocal.open(self.inpms)
        cols = tblocal.colnames()
        tblocal.close()
        if 'MODEL_DATA' in cols:
            raise ValueError, "The input MS, " + self.inpms + " already has a MODEL_DATA col" + str(cols)

        try:
            print "\nRunning setjy(field='Uranus')."
            sjran = setjy(self.inpms, field='Titan', spw='',
                          selectdata=True, observation=1, 
                          modimage='',
                          scalebychan=False, fluxdensity=-1,
                          standard='Butler-JPL-Horizons 2010', usescratch=True)
        except Exception, e:
            print "\nError running setjy(field='Uranus')"
            raise e

        try:
            tblocal.open(self.inpms)
            cols = tblocal.colnames()
            if 'MODEL_DATA' not in cols:
                raise AssertionError, "setjy(field='Uranus') did not add a MODEL_DATA column"
            else:
                record[0] = tblocal.getcell('MODEL_DATA', 0)[0, 0]
                record[1] = tblocal.getcell('MODEL_DATA', 666)[0]
                record[2] = tblocal.getcell('MODEL_DATA', 950)[0, 0]
                tblocal.close()
                self.result = record
        except AssertionError, e:
            print "\nError accesing MODEL_DATA"
            tblocal.close()
            raise e

        """Was obsID 0 left alone?"""
        self.check_eq(self.result[0], 1.0+0.0j, 0.003)
        """Was obsID 1 set?"""
        self.check_eq(self.result[1],
                 numpy.array([1.40439999+0.j, 1.40436542+0.j,
                              1.40433097+0.j, 1.40429640+0.j]), 0.003)
        """Was obsID 2 left alone?"""
        self.check_eq(self.result[2], 1.0+0.0j, 0.003)

    def test2_MultipleObservationOldModel(self):
        """ Test vs an MS with multiple observations using the Butler-JPL-Horizons 2012 model"""

        os.system("mv " + self.inpms + " " + self.inpms + ".test2")
        self.inpms += ".test2"
        record = {}

        tblocal = tbtool()
        tblocal.open(self.inpms)
        cols = tblocal.colnames()
        tblocal.close()
        if 'MODEL_DATA' in cols:
            raise ValueError, "The input MS, " + self.inpms + " already has a MODEL_DATA col" + str(cols)

        try:
            print "\nRunning setjy(field='Uranus')."
            sjran = setjy(self.inpms, field='Titan', spw='',
                          selectdata=True, observation=1, 
                          modimage='',
                          scalebychan=False, fluxdensity=-1,
                          standard='Butler-JPL-Horizons 2012', usescratch=True)
        except Exception, e:
            print "\nError running setjy(field='Uranus')"
            raise e

        try:
            tblocal.open(self.inpms)
            cols = tblocal.colnames()
            if 'MODEL_DATA' not in cols:
                raise AssertionError, "setjy(field='Uranus') did not add a MODEL_DATA column"
            else:
                record[0] = tblocal.getcell('MODEL_DATA', 0)[0, 0]
                record[1] = tblocal.getcell('MODEL_DATA', 666)[0]
                record[1] = tblocal.getcell('MODEL_DATA', 671)[0]
                record[2] = tblocal.getcell('MODEL_DATA', 950)[0, 0]
                tblocal.close()
                self.result = record
        except AssertionError, e:
            print "\nError accesing MODEL_DATA"
            tblocal.close()
            raise e

        """Was obsID 0 left alone?"""
        self.check_eq(self.result[0], 1.0+0.0j, 0.003)
        """Was obsID 1 set?"""
        self.check_eq(self.result[1],
	#        numpy.array([ 1.21551239-0.33617234j,  1.19003308-0.41755155j,
	#                      1.15911222-0.49702403j,  1.12289071-0.57422638j]),
	        numpy.array([ 1.26114714+0.j,  1.26116526+0.j, 1.26118350+0.j,  1.26120162+0.j]),
			      0.003)
        """Was obsID 2 left alone?"""
        self.check_eq(self.result[2], 1.0+0.0j, 0.003)


class test_ModImage(SetjyUnitTestBase):

    def setUp(self):

        self.inpuvf = datapath + '/ATST2/NGC1333/N1333_1.UVFITS'
        self.inpms = 'unittest/setjy/n1333_1.ms'
        self.field = '0542+498_1'
        self.modelim = '3C147_U.im'
        self.result = {}

        if not os.path.exists(self.inpuvf):
            raise EnvironmentError, "Missing input UVFITS file: " + datapath + self.inpuvf

        try:
            print "Importing", self.inpuvf, "to an MS."
            importuvfits(fitsfile=self.inpuvf, vis=self.inpms,antnamescheme="new")
        except Exception, e:
            print "importuvfits error:"
            raise e
    
    def test1_UBandModelQithQBandMS(self):
        """ Test U-Band model with W-Band data to see impact of flus density scale """

        # The MS is in Q band, so deliberately choose the U band model so that the structure
        # is not too far off, but whether or not its flux density is scaled makes a difference.

        for use_standard in [False, True]:
            self.result[use_standard] = self.run_setjy(use_standard)

        self.result['fluxdens'] = self.run_setjy(False, 1234.0)
        self.result['spix'] = self.run_setjy(False,1234.0 * (43.42064/35.0)**0.7,-0.7,"35.0GHz")

        """Flux density in HISTORY (standard)?"""
        # Don't bother checking it without scaling - it won't be there and isn't interesting.
        self.check_history(self.result[True]['history'],["Scaling spw 1's model image to I ="])
        """Flux density in HISTORY (fluxdensity)?"""
        self.check_history(self.result['fluxdens']['history'],["Scaling spw 1's model image to I ="])
        """Flux density in HISTORY (fluxdensity)?"""
        self.check_history(self.result['fluxdens']['history'],["Scaling spw 1's model image to I ="])
        """Flux density in HISTORY (spix)?"""
        self.check_history(self.result['spix']['history'],["Scaling spw 1's model image to I ="])
        """modimage != '' and fluxdensity == 0 -> no scaling?"""
        self.check_eq(self.result[False]['short'], 2.712631, 0.05)
        self.check_eq(self.result[False]['long'],  2.4080808, 0.05)
        """modimage != '' and default fluxdensity -> scaling?"""
        self.check_eq(self.result[True]['short'], 0.911185, 0.025)
        self.check_eq(self.result[True]['long'],  0.808885, 0.025)
        """modimage != '' and fluxdensity > 0"""
        self.check_eq(self.result['fluxdens']['short'], 1233.7, 0.05)
        self.check_eq(self.result['fluxdens']['long'],  1095.2, 0.05)
        """modimage != '', fluxdensity > 0, and spix = -0.7"""
        self.check_eq(self.result['spix']['short'], 1233.7, 0.5)
        self.check_eq(self.result['spix']['long'],  1095.2, 0.5)

        return True

    def run_setjy(self, use_standard, fluxdens=0, spix=0, reffreq="1GHz"):
        record = {'setjyran': False}
        try:
            if use_standard:
                record['setjyran'] = setjy(vis=self.inpms, field=self.field,
                                           modimage=self.modelim,
                                           scalebychan=False,
                                           standard='Perley-Taylor 99',
                                           usescratch=True,
                                           async=False)
            else:
                record['setjyran'] = setjy(vis=self.inpms, field=self.field,
                                           modimage=self.modelim,
                                           scalebychan=False,
                                           fluxdensity=fluxdens,
                                           spix=spix, reffreq=reffreq,
                                           usescratch=True,
                                           async=False)
            record['history'] = self.get_last_history_line(self.inpms,
                                                           origin='imager::setjy()',
                                                           hint='model image to I')
            ms.open(self.inpms)
            record['short'] = ms.statistics(column='MODEL',
                                            complex_value='amp',
                                            field='0542+498_1',
                                            baseline='2&9',
                                            time='2003/05/02/19:53:30.0',
                                            correlation='rr')['MODEL']['mean']
            record['long']  = ms.statistics(column='MODEL',
                                            complex_value='amp',
                                            field='0542+498_1',
                                            baseline='21&24',
                                            time='2003/05/02/19:53:30.0',
                                            correlation='ll')['MODEL']['mean']
            ms.close()
        except Exception, e:
            print "Error from setjy or ms.statistics()"
            raise e

        return record
    
class test_inputs(SetjyUnitTestBase):
    """Test input parameter checking"""
    def setUp(self):
        self.setUpMS("unittest/setjy/2528.ms")         # Uranus

    def tearDown(self):
        self.resetMS()

    def test_vischeck(self):
        """ Test input vis check"""
        self.inpms='wrong.ms'
        if os.path.exists(self.inpms):
            shutil.rmtree(self.inpms) 


        # test by temporarily setting __rethrow_casa_exceptions
        sjran=None
        try:
            myf = sys._getframe(len(inspect.stack()) - 1).f_globals
            original_rethrow_setting=myf.get('__rethrow_casa_exceptions',False)
            myf['__rethrow_casa_exceptions']=True
            print "\nRunning setjy with a non-existant vis"
            sjran = setjy(vis=self.inpms,listmodels=False)
        except Exception, setjyUTerr:
            msg = setjyUTerr.message
            self.assertNotEqual(msg.find("%s does not exist" % self.inpms), -1,
                                'wrong type of exception is thrown')
        finally:
            # put back original rethrow setting
            myf['__rethrow_casa_exceptions']=original_rethrow_setting
        self.assertEqual(sjran,None,"Failed to raise exception.") 
     
    def test_listmodels(self):
        """ Test listmodels mode """
        self.inpms=''
        print "\nRunning setjy in listmodels mode ...."
        sjran = setjy(vis=self.inpms,listmodels=True)
        self.assertTrue(sjran)
      
        
  
def suite():
    return [test_SingleObservation,test_MultipleObservations,test_ModImage, test_inputs]
