import os
import sys
import shutil
import inspect
import re
from __main__ import default
from tasks import *
from taskinit import *
import unittest
import sha
import time
import numpy

from tsdsave import tsdsave
import asap as sd

# Unit test of tsdsave task.
# 
# Currently, the test only examine if supported types of data can be
# read, and if supporeted types of data can be written.
# 
# The test on data selection and data averaging will not be done.

###
# Base class for all testing classes
###
class tsdsave_unittest_base:
    """
    Base class for testing classes.
    Implements several methods to compare the results.
    """
    taskname='tsdsave'
    datapath=os.environ.get('CASAPATH').split()[0] + '/data/regression/unittest/sdsave/'
    basefile='OrionS_rawACSmod_cal2123.asap'
    summaryStr = None
    firstSpec = None
    nrow = None
    ifno = None
    cycleno = None
    scanno = None

    def _checkfile( self, name ):
        isthere=os.path.exists(name)
        self.assertEqual(isthere,True,
                         msg='output file %s was not created because of the task failure'%(name))

    def _setAttributes(self):
        """
        Set summary string from the original data.
        """
        tb.open(self.basefile)
        #s=sd.scantable(self.basefile,False)
        #self.summaryStr=s._summary()
        #self.firstSpec=numpy.array(s._getspectrum(0))
        #self.nrow=s.nrow()
        self.firstSpec=tb.getcell('SPECTRA',0)
        self.nrow=tb.nrows()
        self.scanno=tb.getcell('SCANNO',0)
        self.ifno=tb.getcell('IFNO',0)
        self.cycleno=tb.getcell('CYCLENO',0)
        self.npol=tb.getkeyword('nPol')
        #del s
        tb.close()


    def _compare(self,filename):
        """
        Compare results

           - check number of rows
           - check first spectrum
        """
        [nrow,sp0] = self._get(filename)
        #casalog.post('nrow=%s'%nrow)
        #casalog.post('maxdiff=%s'%((abs(self.firstSpec-sp0)).max()))
        if nrow != self.nrow:
            return False
        if any((abs(self.firstSpec-sp0))>1.0e-6):
            return False
        return True

    def _get(self,filename):
        """
        """
        n=None
        st=filename.split('.')
        extension=st[-1]
        #casalog.post('filename='+filename)
        if extension == 'asap' or extension == 'ms' or extension == 'fits':
            self._checkfile(filename)
            s=sd.scantable(filename,False)
            n=s.nrow()
            sp=numpy.array(s._getspectrum(0))
            del s
        else:
            import commands
            wcout=commands.getoutput('ls '+st[0]+'*.txt'+' | wc')
            n=int(wcout.split()[0])*self.npol
            filein=st[0]+'_SCAN%d_CYCLE%d_IF%d.txt'%(self.scanno,self.cycleno,self.ifno)
            self._checkfile(filein)
            f=open(filein)
            sp=[]
            line = f.readline()
            while ( line != '' ):
                if line[0] != '#' and line[0] != 'x':
                    lines = line.split()
                    sp.append(float(lines[1]))
                line = f.readline()
            sp = numpy.array(sp)
            f.close()
        return [n,sp]            


###
# Test on bad parameter settings, data selection, data averaging, ...
###
class tsdsave_test0(unittest.TestCase,tsdsave_unittest_base):
    """
    Test on data selection, data averaging...
    """
    # Input and output names
    infile='OrionS_rawACSmod_cal2123.asap'
    prefix=tsdsave_unittest_base.taskname+'Test0'
    outfile=prefix+'.asap'

    def setUp(self):
        self.res=None
        if (not os.path.exists(self.infile)):
            shutil.copytree(self.datapath+self.infile, self.infile)

        default(tsdsave)

    def tearDown(self):
        if (os.path.exists(self.infile)):
            shutil.rmtree(self.infile)
        os.system( 'rm -rf '+self.prefix+'*' )

    def test000(self):
        """Test 000: Default parameters"""
        # argument verification error
        self.res=tsdsave()
        self.assertFalse(self.res)        


###
# Test to read scantable and write various types of format
###
class tsdsave_test1(unittest.TestCase,tsdsave_unittest_base):
    """
    Read scantable data, write various types of format.
    """
    # Input and output names
    infile='OrionS_rawACSmod_cal2123.asap'
    prefix=tsdsave_unittest_base.taskname+'Test1'
    outfile0=prefix+'.asap'
    outfile1=prefix+'.ms'
    outfile2=prefix+'.fits'
    outfile3=prefix

    def setUp(self):
        self.res=None
        if (not os.path.exists(self.infile)):
            shutil.copytree(self.datapath+self.infile, self.infile)
        if (not os.path.exists(self.basefile)):
            shutil.copytree(self.datapath+self.basefile, self.basefile)

        default(tsdsave)
        self._setAttributes()

    def tearDown(self):
        if (os.path.exists(self.infile)):
            shutil.rmtree(self.infile)
        if (os.path.exists(self.basefile)):
            shutil.rmtree(self.basefile)
        os.system( 'rm -rf '+self.prefix+'*' )

    def test100(self):
        """Test 100: test to read scantable and to write as scantable"""
        self.res=tsdsave(infile=self.infile,outfile=self.outfile0,outform='ASAP')
        self.assertEqual(self.res,None)
        self.assertTrue(self._compare(self.outfile0))

    def test101(self):
        """Test 101: test to read scantable and to write as MS"""
        self.res=tsdsave(infile=self.infile,outfile=self.outfile1,outform='MS2')
        self.assertEqual(self.res,None)
        self.assertTrue(self._compare(self.outfile1))
        
    def test102(self):
        """Test 102: test to read scantable and to write as SDFITS"""
        self.res=tsdsave(infile=self.infile,outfile=self.outfile2,outform='SDFITS')
        self.assertEqual(self.res,None)
        self.assertTrue(self._compare(self.outfile2))

    def test103(self):
        """Test 103: test to read scantable and to write as ASCII"""
        self.res=tsdsave(infile=self.infile,outfile=self.outfile3,outform='ASCII')
        self.assertEqual(self.res,None)
        self.assertTrue(self._compare(self.outfile3))
        

###
# Test to read MS and write various types of format
###
class tsdsave_test2(unittest.TestCase,tsdsave_unittest_base):
    """
    Read MS data, write various types of format.
    """
    # Input and output names
    infile='OrionS_rawACSmod_cal2123.ms'
    prefix=tsdsave_unittest_base.taskname+'Test2'
    outfile0=prefix+'.asap'
    outfile1=prefix+'.ms'
    outfile2=prefix+'.fits'
    outfile3=prefix

    def setUp(self):
        self.res=None
        if (not os.path.exists(self.infile)):
            shutil.copytree(self.datapath+self.infile, self.infile)
        if (not os.path.exists(self.basefile)):
            shutil.copytree(self.datapath+self.basefile, self.basefile)

        default(tsdsave)
        self._setAttributes()
        self.scanno=1

    def tearDown(self):
        if (os.path.exists(self.infile)):
            shutil.rmtree(self.infile)
        if (os.path.exists(self.basefile)):
            shutil.rmtree(self.basefile)
        os.system( 'rm -rf '+self.prefix+'*' )

    def test200(self):
        """Test 200: test to read MS and to write as scantable"""
        self.res=tsdsave(infile=self.infile,outfile=self.outfile0,outform='ASAP')
        self.assertEqual(self.res,None)
        self.assertTrue(self._compare(self.outfile0))
        
    def test201(self):
        """Test 201: test to read MS and to write as MS"""
        self.res=tsdsave(infile=self.infile,outfile=self.outfile1,outform='MS2')
        self.assertEqual(self.res,None)
        self.assertTrue(self._compare(self.outfile1))
        
    def test202(self):
        """Test 202: test to read MS and to write as SDFITS"""
        self.res=tsdsave(infile=self.infile,outfile=self.outfile2,outform='SDFITS')
        self.assertEqual(self.res,None)
        self.assertTrue(self._compare(self.outfile2))

    def test203(self):
        """Test 203: test to read MS and to write as ASCII"""
        self.res=tsdsave(infile=self.infile,outfile=self.outfile3,outform='ASCII')
        self.assertEqual(self.res,None)
        self.assertTrue(self._compare(self.outfile3))

    def test204(self):
        """Test 204: test failure case that unexisting antenna is specified"""
        try:
            self.res=tsdsave(infile=self.infile,antenna='ROSWELL',outfile=self.outfile0,outform='ASAP')
            self.assertTrue(False,
                            msg='The task must throw exception')
        except Exception, e:
            # the task failed to import data so that failed to open output file
            pos=str(e).find('Failed to open file')
            self.assertNotEqual(pos,-1,
                                msg='Unexpected exception was thrown: %s'%(str(e)))

    def test205(self):
        """Test 205: test to read USB spectral window"""
        self.__spwtest()
        
    def test206(self):
        """Test 206: test to read LSB spectral window"""
        tb.open('%s/SPECTRAL_WINDOW'%(self.infile),nomodify=False)
        chanw = tb.getcol('CHAN_WIDTH')
        chanf = tb.getcol('CHAN_FREQ')
        chanw *= -1.0
        chanf = numpy.flipud(chanf)
        tb.putcol('CHAN_WIDTH',chanw)
        tb.putcol('CHAN_FREQ',chanf)
        netsb = numpy.ones( tb.nrows(), int )
        tb.putcol('NET_SIDEBAND', netsb )
        tb.close()
        self.__spwtest()

    def __spwtest(self):
        self.res=tsdsave(infile=self.infile,outfile=self.outfile0,outform='ASAP')
        self.assertFalse(self.res,False)
        self.__compareIncrement( self.outfile0, self.infile )
        self.res=tsdsave(infile=self.outfile0,outfile=self.outfile1,outform='MS2')
        self.assertFalse(self.res,False)
        self.__compareIncrement( self.outfile0, self.outfile1 )        

    def __compareIncrement(self,stdata,msdata):
        tb.open('%s/FREQUENCIES'%(stdata))
        incr=tb.getcol('INCREMENT')
        tb.close()
        tb.open('%s/SPECTRAL_WINDOW'%(msdata))
        chanw=tb.getcol('CHAN_WIDTH')
        tb.close()
        for i in xrange(len(incr)):
            #print 'incr[%s]=%s,chanw[0][%s]=%s(diff=%s)'%(i,incr[i],i,chanw[0][i],(incr[i]-chanw[0][i]))
            self.assertEqual(incr[i],chanw[0][i])
        
###
# Test to read ATNF SDFITS and write various types of format
###
class tsdsave_test3(unittest.TestCase,tsdsave_unittest_base):
    """
    Read ATNF SDFITS data, write various types of format.
    """
    # Input and output names
    infile='OrionS_rawACSmod_cal2123.fits'
    prefix=tsdsave_unittest_base.taskname+'Test3'
    outfile0=prefix+'.asap'
    outfile1=prefix+'.ms'
    outfile2=prefix+'.fits'
    outfile3=prefix

    def setUp(self):
        self.res=None
        if (not os.path.exists(self.infile)):
            shutil.copy(self.datapath+self.infile, self.infile)
        if (not os.path.exists(self.basefile)):
            shutil.copytree(self.datapath+self.basefile, self.basefile)

        default(tsdsave)
        self._setAttributes()
        self.scanno=0

    def tearDown(self):
        if (os.path.exists(self.infile)):
            os.system( 'rm -f '+self.infile )
        if (os.path.exists(self.basefile)):
            shutil.rmtree(self.basefile)
        os.system( 'rm -rf '+self.prefix+'*' )

    def test300(self):
        """Test 300: test to read ATNF SDFITS and to write as scantable"""
        self.res=tsdsave(infile=self.infile,outfile=self.outfile0,outform='ASAP')
        self.assertEqual(self.res,None)
        self.assertTrue(self._compare(self.outfile0))

    def test301(self):
        """Test 301: test to read ATNF SDFITS and to write as MS"""
        self.res=tsdsave(infile=self.infile,outfile=self.outfile1,outform='MS2')
        self.assertEqual(self.res,None)
        self.assertTrue(self._compare(self.outfile1))
        
    def test302(self):
        """Test 302: test to read ATNF SDFITS and to write as SDFITS"""
        self.res=tsdsave(infile=self.infile,outfile=self.outfile2,outform='SDFITS')
        self.assertEqual(self.res,None)
        self.assertTrue(self._compare(self.outfile2))

    def test303(self):
        """Test 303: test to read ATNF SDFITS and to write as ASCII"""
        self.res=tsdsave(infile=self.infile,outfile=self.outfile3,outform='ASCII')
        self.assertEqual(self.res,None)
        self.assertTrue(self._compare(self.outfile3))
        

###
# Test to read GBT SDFITS and write various types of format
###
class tsdsave_test4(unittest.TestCase,tsdsave_unittest_base):
    """
    Read GBT SDFITS data, write various types of format.
    """
    # Input and output names
    infile='AGBT06A_sliced.fits'
    prefix=tsdsave_unittest_base.taskname+'Test4'
    outfile0=prefix+'.asap'
    outfile1=prefix+'.ms'
    outfile2=prefix+'.fits'
    outfile3=prefix

    def setUp(self):
        self.res=None
        if (not os.path.exists(self.infile)):
            shutil.copy(self.datapath+self.infile, self.infile)

        default(tsdsave)
        #self._setAttributes()

    def tearDown(self):
        if (os.path.exists(self.infile)):
            os.system( 'rm -f '+self.infile )
        os.system( 'rm -rf '+self.prefix+'*' )

    def test400(self):
        """Test 400: test to read GBT SDFITS and to write as scantable"""
        self.res=tsdsave(infile=self.infile,outfile=self.outfile0,outform='ASAP')
        self.assertEqual(self.res,None)
        #self.assertTrue(self._compare(self.outfile0))
        self.assertTrue(self._compare())

    def test401(self):
        """Test 401: test to read GBT SDFITS and to write as MS"""
        self.res=tsdsave(infile=self.infile,outfile=self.outfile1,outform='MS2')
        self.assertEqual(self.res,None)
        #self.assertTrue(self._compare(self.outfile1))
        
    def test402(self):
        """Test 402: test to read GBT SDFITS and to write as SDFITS"""
        self.res=tsdsave(infile=self.infile,outfile=self.outfile2,outform='SDFITS')
        self.assertEqual(self.res,None)
        #self.assertTrue(self._compare(self.outfile2))

    def test403(self):
        """Test 403: test to read GBT SDFITS and to write as ASCII"""
        self.res=tsdsave(infile=self.infile,outfile=self.outfile3,outform='ASCII')
        self.assertEqual(self.res,None)
        #self.assertTrue(self._compare(self.outfile3))

    def _compare(self,filename=''):
        """
        Check a few things for the data.
        """
        s=sd.scantable(self.infile,False)
        if ( s.nrow() != 48 ):
            return False
        if ( s.nif() != 6 ):
            return False
        if ( s.nchan(0) != 4096 ):
            return False
        if ( s.nchan(2) != 8192 ):
            return False
        if ( s.npol() != 1 ):
            return False
        return True
        
###
# Test to read NROFITS and write various types of format
###
class tsdsave_test5(unittest.TestCase,tsdsave_unittest_base):
    """
    Read NROFITS data, write various types of format.
    """
    # Input and output names
    infile='B68test.nro'
    prefix=tsdsave_unittest_base.taskname+'Test5'
    outfile0=prefix+'.asap'
    outfile1=prefix+'.ms'
    outfile2=prefix+'.fits'
    outfile3=prefix

    def setUp(self):
        self.res=None
        if (not os.path.exists(self.infile)):
            shutil.copy(self.datapath+self.infile, self.infile)

        default(tsdsave)
        #self._setAttributes()

    def tearDown(self):
        if (os.path.exists(self.infile)):
            os.system( 'rm -f '+self.infile )
        os.system( 'rm -rf '+self.prefix+'*' )

    def test500(self):
        """Test 500: test to read NROFITS and to write as scantable"""
        self.res=tsdsave(infile=self.infile,outfile=self.outfile0,outform='ASAP')
        self.assertEqual(self.res,None)
        #self.assertTrue(self._compare(self.outfile0))
        self.assertTrue(self._compare())

    def test501(self):
        """Test 501: test to read NROFITS and to write as MS"""
        self.res=tsdsave(infile=self.infile,outfile=self.outfile1,outform='MS2')
        self.assertEqual(self.res,None)
        #self.assertTrue(self._compare(self.outfile1))
        
    def test502(self):
        """Test 502: test to read NROFITS and to write as SDFITS"""
        self.res=tsdsave(infile=self.infile,outfile=self.outfile2,outform='SDFITS')
        self.assertEqual(self.res,None)
        #self.assertTrue(self._compare(self.outfile2))

    def test503(self):
        """Test 503: test to read NROFITS and to write as ASCII"""
        self.res=tsdsave(infile=self.infile,outfile=self.outfile3,outform='ASCII')
        self.assertEqual(self.res,None)
        #self.assertTrue(self._compare(self.outfile3))
        
    def _compare(self,filename=''):
        """
        Check a few things for the data.
        """
        s=sd.scantable(self.infile,False)
        if ( s.nrow() != 36 ):
            return False
        if ( s.nif() != 4 ):
            return False
        if ( s.nchan() != 2048 ):
            return False
        if ( s.npol() != 1 ):
            return False
        return True        


###
# Test getpt parameter
###
class tsdsave_test6( unittest.TestCase, tsdsave_unittest_base ):
    """
    Test getpt parameter

    1) import MS to Scantable format with getpt=True 
       1-1) check POINTING table keyword is missing
       1-2) export Scantable to MS format
       1-3) compare POINTING table
    2) import MS to Scantable format with getpt=False
       1-1) check POINTING table keyword exists
       1-2) export Scantable to MS format
       1-3) compare POINTING table

    """
    # Input and output names
    infile='OrionS_rawACSmod_cal2123.ms'
    prefix=tsdsave_unittest_base.taskname+'Test6'
    outfile0=prefix+'.asap'
    outfile1=prefix+'.ms'

    def setUp(self):
        self.res=None
        if (not os.path.exists(self.infile)):
            shutil.copytree(self.datapath+self.infile, self.infile)

        default(tsdsave)
        #self._setAttributes()

    def tearDown(self):
        if (os.path.exists(self.infile)):
            os.system( 'rm -rf '+self.infile )
        os.system( 'rm -rf '+self.prefix+'*' )

    def test600(self):
        """Test 600: test getpt=True"""
        self.res=tsdsave(infile=self.infile,getpt=True,outfile=self.outfile0,outform='ASAP')
        self.assertEqual(self.res,None)
        self.assertFalse(self._pointingKeywordExists())
        self.res=tsdsave(infile=self.outfile0,outfile=self.outfile1,outform='MS2')
        self.assertTrue(self._compare())

    def test601(self):
        """Test 601: test getpt=False"""
        self.res=tsdsave(infile=self.infile,getpt=False,outfile=self.outfile0,outform='ASAP')
        self.assertEqual(self.res,None)
        self.assertTrue(self._pointingKeywordExists())
        self.res=tsdsave(infile=self.outfile0,outfile=self.outfile1,outform='MS2')
        self.assertTrue(self._compare())

    def _pointingKeywordExists(self):
        _tb=tbtool()
        _tb.open(self.outfile0)
        keys=_tb.getkeywords()
        _tb.close()
        del _tb
        return 'POINTING' in keys

    def _compare(self):
        ret = True
        _tb1=tbtool()
        _tb2=tbtool()
        _tb1.open(self.infile)
        #ptab1=_tb1.getkeyword('POINTING').split()[-1]
        ptab1=_tb1.getkeyword('POINTING').lstrip('Table: ')
        _tb1.close()
        _tb1.open(ptab1)
        _tb2.open(self.outfile1)
        #ptab2=_tb2.getkeyword('POINTING').split()[-1]
        ptab2=_tb2.getkeyword('POINTING').lstrip('Table: ')
        _tb2.close()
        _tb2.open(ptab1)
        badcols = []
        for col in _tb1.colnames():
            if not all(_tb1.getcol(col).flatten()==_tb2.getcol(col).flatten()):
                badcols.append( col )
        _tb1.close()
        _tb2.close()
        del _tb1, _tb2
        if len(badcols) != 0:
            print 'Bad column: %s'%(badcols)
            ret = False
        return ret

###
# Test rest frequency
###
class tsdsave_test7( tsdsave_unittest_base, unittest.TestCase ):
    """
    Unit tests for task tsdsave. Test scantable available restfreq

    The list of tests:
    test701-704  --- a value (float, int, quantity w/ and w/o a unit)
    test711-715  --- a list [length=1]
                     (float, int, quantity w/ and w/o a unit, dict)
    test721-725  --- a list [length>1]
                     (float, int, quantity w/ and w/o a unit, dict)
    test731-733  --- a bad quantity unit (a value, a list)
    """
    # Input and output names
    infile = 'OrionS_rawACSmod_cal2123.asap'
    outname = tsdsave_unittest_base.taskname+'_test'
    iflist = [1,2]
    spw = '1~2'
    frf = [45.301e9,44.075e9]
    irf = [45301000000,44075000000]
    qurf = ['45301.MHz','44.075GHz']
    qrf = [str(frf[0]), str(irf[1])]
    drf = [{'name': "IF1 Rest", 'value': frf[0]}, \
           {'name': "IF2 Rest", 'value': qurf[1]}]
    badq = ['45301.km','44.075bad']

    def setUp( self ):
        # copy input scantables
        if os.path.exists(self.infile):
            shutil.rmtree(self.infile)
        shutil.copytree(self.datapath+self.infile, self.infile)

        default(tsdsave)

    def tearDown( self ):
        if (os.path.exists(self.infile)):
            shutil.rmtree(self.infile)


    # Helper functions for testing
    def _check_restfreq( self, outfile, restfreq, iflist ):
        self._checkfile(outfile)
        #print "To be checked 0 : ifnos =", iflist, ", restfreqs = ", restfreq
        if not (type(restfreq) in (list, tuple, numpy.ndarray)):
            restfreq = [restfreq] * len(iflist)
        elif len(restfreq) == 1:
            restfreq = restfreq * len(iflist)
        #print "To be checked: ifnos =", iflist, ", restfreqs = ", restfreq
        scan = sd.scantable(outfile, average = False)
        for i in range(len(restfreq)):
            ifno = iflist[i]
            rf = restfreq[i]
            if type(rf) == dict:
                rf = rf['value']
            if qa.isquantity(rf):
                rfv = qa.convert(rf,'Hz')['value']
            else:
                rfv = float(rf)
            scan.set_selection(ifs=[ifno])
            molid = scan.getmolnos()
            self.assertTrue(len(molid)==1,msg="IFNO=%d has multiple MOLECULE_IDs: %s" % (ifno,str(molid)))
            newrf = scan.get_restfreqs(molid[0])
            #self.assertTrue(len(newrf)==1,msg="IFNO=%d has multiple rest frequencies: %s" % (ifno,str(newrf)))
            self.assertEqual(newrf[0], rfv,\
                             msg="Rest frequency differs(IFNO=%d): %f (expected: %f)" % (ifno, newrf[0], rfv))
            print "Rest frequency (IFNO=%d): %f (expected: %f)" % (ifno, newrf[0], rfv)
    
    # Actual tests
    def test701( self ):
        """Test 701: restfreq (a float value)"""
        tid = "701"
        infile = self.infile
        outfile = self.outname+tid
        iflist = self.iflist
        restfreq = self.frf[1]

        print "Setting restfreq = %s (%s)" % (str(restfreq), str(type(restfreq)))
        result = tsdsave(infile=infile,outfile=outfile,\
                        spw=self.spw,restfreq=restfreq)

        self.assertEqual(result,None)
        self.assertTrue(os.path.exists(outfile),msg="No output written")
        print "Testing rest frequencies of output scantable"
        self._check_restfreq(outfile, restfreq, iflist)

    def test702( self ):
        """Test 702: restfreq (an int value)"""
        tid = "702"
        infile = self.infile
        outfile = self.outname+tid
        iflist = self.iflist
        restfreq = self.irf[1]

        print "Setting restfreq = %s (%s)" % (str(restfreq), str(type(restfreq)))
        result = tsdsave(infile=infile,outfile=outfile,\
                        spw=self.spw,restfreq=restfreq)

        self.assertEqual(result,None)
        self.assertTrue(os.path.exists(outfile),msg="No output written")
        print "Testing rest frequencies of output scantable"
        self._check_restfreq(outfile, restfreq, iflist)

    def test703( self ):
        """Test 703: restfreq (a quantity with unit)"""
        tid = "703"
        infile = self.infile
        outfile = self.outname+tid
        iflist = self.iflist
        restfreq = self.qurf[1]

        print "Setting restfreq = %s" % (str(restfreq))
        result = tsdsave(infile=infile,outfile=outfile,\
                        spw=self.spw,restfreq=restfreq)

        self.assertEqual(result,None)
        self.assertTrue(os.path.exists(outfile),msg="No output written")
        print "Testing rest frequencies of output scantable"
        self._check_restfreq(outfile, restfreq, iflist)

    def test704( self ):
        """Test 704: restfreq (a quantity withOUT unit)"""
        tid = "704"
        infile = self.infile
        outfile = self.outname+tid
        iflist = self.iflist
        restfreq = self.qrf[1]

        print "Setting restfreq = %s (%s)" % (str(restfreq), str(type(restfreq)))
        result = tsdsave(infile=infile,outfile=outfile,\
                        spw=self.spw,restfreq=restfreq)

        self.assertEqual(result,None)
        self.assertTrue(os.path.exists(outfile),msg="No output written")
        print "Testing rest frequencies of output scantable"
        self._check_restfreq(outfile, restfreq, iflist)

    def test711( self ):
        """Test 711: restfreq (a list of float value [length = 1])"""
        tid = "711"
        infile = self.infile
        outfile = self.outname+tid
        iflist = self.iflist
        restfreq = [ self.frf[1] ]

        print "Setting restfreq = %s" % (str(restfreq))
        result = tsdsave(infile=infile,outfile=outfile,\
                        spw=self.spw,restfreq=restfreq)

        self.assertEqual(result,None)
        self.assertTrue(os.path.exists(outfile),msg="No output written")
        print "Testing rest frequencies of output scantable"
        self._check_restfreq(outfile, restfreq, iflist)

    def test712( self ):
        """Test 712: restfreq (a list of int value [length = 1])"""
        tid = "712"
        infile = self.infile
        outfile = self.outname+tid
        iflist = self.iflist
        restfreq = [ self.irf[1] ]

        print "Setting restfreq = %s" % (str(restfreq))
        result = tsdsave(infile=infile,outfile=outfile,\
                        spw=self.spw,restfreq=restfreq)

        self.assertEqual(result,None)
        self.assertTrue(os.path.exists(outfile),msg="No output written")
        print "Testing rest frequencies of output scantable"
        self._check_restfreq(outfile, restfreq, iflist)

    def test713( self ):
        """Test 713: restfreq (a list of quantity with unit [length = 1])"""
        tid = "713"
        infile = self.infile
        outfile = self.outname+tid
        iflist = self.iflist
        restfreq = [ self.qurf[1] ]

        print "Setting restfreq = %s" % (str(restfreq))
        result = tsdsave(infile=infile,outfile=outfile,\
                        spw=self.spw,restfreq=restfreq)

        self.assertEqual(result,None)
        self.assertTrue(os.path.exists(outfile),msg="No output written")
        print "Testing rest frequencies of output scantable"
        self._check_restfreq(outfile, restfreq, iflist)

    def test714( self ):
        """Test 714: restfreq (a list of quantity withOUT unit [length = 1])"""
        tid = "714"
        infile = self.infile
        outfile = self.outname+tid
        iflist = self.iflist
        restfreq = [ self.qrf[1] ]

        print "Setting restfreq = %s" % (str(restfreq))
        result = tsdsave(infile=infile,outfile=outfile,\
                        spw=self.spw,restfreq=restfreq)

        self.assertEqual(result,None)
        self.assertTrue(os.path.exists(outfile),msg="No output written")
        print "Testing rest frequencies of output scantable"
        self._check_restfreq(outfile, restfreq, iflist)

    def test715( self ):
        """Test 715: restfreq (a list of dictionary [length = 1])"""
        tid = "715"
        infile = self.infile
        outfile = self.outname+tid
        iflist = self.iflist
        restfreq = [ self.drf[1] ]

        print "Setting restfreq = %s" % (str(restfreq))
        result = tsdsave(infile=infile,outfile=outfile,\
                        spw=self.spw,restfreq=restfreq)

        self.assertEqual(result,None)
        self.assertTrue(os.path.exists(outfile),msg="No output written")
        print "Testing rest frequencies of output scantable"
        self._check_restfreq(outfile, restfreq, iflist)

    def test721( self ):
        """Test 721: restfreq (a list of float value [length > 1])"""
        tid = "721"
        infile = self.infile
        outfile = self.outname+tid
        iflist = self.iflist
        restfreq = self.frf

        print "Setting restfreq = %s" % (str(restfreq))
        result = tsdsave(infile=infile,outfile=outfile,\
                        spw=self.spw,restfreq=restfreq)

        self.assertEqual(result,None)
        self.assertTrue(os.path.exists(outfile),msg="No output written")
        print "Testing rest frequencies of output scantable"
        self._check_restfreq(outfile, restfreq, iflist)

    def test722( self ):
        """Test 722: restfreq (a list of int value [length > 1])"""
        tid = "722"
        infile = self.infile
        outfile = self.outname+tid
        iflist = self.iflist
        restfreq = self.irf

        print "Setting restfreq = %s" % (str(restfreq))
        result = tsdsave(infile=infile,outfile=outfile,\
                        spw=self.spw,restfreq=restfreq)

        self.assertEqual(result,None)
        self.assertTrue(os.path.exists(outfile),msg="No output written")
        print "Testing rest frequencies of output scantable"
        self._check_restfreq(outfile, restfreq, iflist)

    def test723( self ):
        """Test 723: restfreq (a list of quantity with unit [length > 1])"""
        tid = "723"
        infile = self.infile
        outfile = self.outname+tid
        iflist = self.iflist
        restfreq = self.qurf

        print "Setting restfreq = %s" % (str(restfreq))
        result = tsdsave(infile=infile,outfile=outfile,\
                        spw=self.spw,restfreq=restfreq)

        self.assertEqual(result,None)
        self.assertTrue(os.path.exists(outfile),msg="No output written")
        print "Testing rest frequencies of output scantable"
        self._check_restfreq(outfile, restfreq, iflist)

    def test724( self ):
        """Test 724: restfreq (a list of quantity withOUT unit [length > 1])"""
        tid = "724"
        infile = self.infile
        outfile = self.outname+tid
        iflist = self.iflist
        restfreq = self.qrf

        print "Setting restfreq = %s" % (str(restfreq))
        result = tsdsave(infile=infile,outfile=outfile,\
                        spw=self.spw,restfreq=restfreq)

        self.assertEqual(result,None)
        self.assertTrue(os.path.exists(outfile),msg="No output written")
        print "Testing rest frequencies of output scantable"
        self._check_restfreq(outfile, restfreq, iflist)

    def test725( self ):
        """Test 725: restfreq (a list of dictionary [length > 1])"""
        tid = "725"
        infile = self.infile
        outfile = self.outname+tid
        iflist = self.iflist
        restfreq = self.drf

        print "Setting restfreq = %s" % (str(restfreq))
        result = tsdsave(infile=infile,outfile=outfile,\
                        spw=self.spw,restfreq=restfreq)

        self.assertEqual(result,None)
        self.assertTrue(os.path.exists(outfile),msg="No output written")
        print "Testing rest frequencies of output scantable"
        self._check_restfreq(outfile, restfreq[0], iflist)

    def test731( self ):
        """Test 731: restfreq (a BAD quantity unit)"""
        tid = "731"
        infile = self.infile
        outfile = self.outname+tid
        iflist = self.iflist
        restfreq = self.badq[0]

        print "Setting restfreq = %s" % (str(restfreq))
        try:
            result = tsdsave(infile=infile,outfile=outfile,\
                                spw=self.spw,restfreq=restfreq)
            self.assertTrue(False,
                            msg='The task must throw exception')
        except Exception, e:
            pos=str(e).find('wrong unit of restfreq')
            self.assertNotEqual(pos,-1,
                                msg='Unexpected exception was thrown: %s'%(str(e)))

    def test732( self ):
        """Test 732: restfreq (a list of BAD quantity unit [length = 1])"""
        tid = "732"
        infile = self.infile
        outfile = self.outname+tid
        iflist = self.iflist
        restfreq = [ self.badq[1] ]

        print "Setting restfreq = %s" % (str(restfreq))
        try:
            result = tsdsave(infile=infile,outfile=outfile,\
                                spw=self.spw,restfreq=restfreq)
            self.assertTrue(False,
                            msg='The task must throw exception')
        except Exception, e:
            pos=str(e).find('Input value is not a quantity: ')
            self.assertNotEqual(pos,-1,
                                msg='Unexpected exception was thrown: %s'%(str(e)))


    def test733( self ):
        """Test 733: restfreq (a list of BAD quantity unit [length > 1])"""
        tid = "733"
        infile = self.infile
        outfile = self.outname+tid
        iflist = self.iflist
        restfreq = self.badq

        print "Setting restfreq = %s" % (str(restfreq))
        try:
            result = tsdsave(infile=infile,outfile=outfile,\
                                spw=self.spw,restfreq=restfreq)

            self.assertTrue(False,
                            msg='The task must throw exception')
        except Exception, e:
            pos=str(e).find('wrong unit of restfreq')
            self.assertNotEqual(pos,-1,
                                msg='Unexpected exception was thrown: %s'%(str(e)))


###
# Test combinations of scantable.storage='disk'/'memory' and insitu=T/F
###
class tsdsave_storageTest( tsdsave_unittest_base, unittest.TestCase ):
    """
    Unit tests for task tsdsave. Test scantable sotrage and insitu
    parameters

    The list of tests:
    testMT  --- storage = 'memory', insitu = True
    testMF  --- storage = 'memory', insitu = False
    testDT  --- storage = 'disk', insitu = True
    testDF  --- storage = 'disk', insitu = False

    Note on handlings of disk storage:
       Task script restores MOLECULE_ID column.

    Tested items:
       1. Number of rows in tables and list of IDs of output scantable.
       2. Units and coordinates of output scantable.
       3. units and coordinates of input scantables before/after run.
    """
    # Input and output names
    infile = 'OrionS_rawACSmod_cal2123.asap'
    outname = tsdsave_unittest_base.taskname+'_test'
    pollist = [1]
    pol = '1'
    iflist = [2]
    spw = '2:100~200'
    restfreq = [44.075e9]
    # Reference data of output scantable
    refout = {"nRow": 8, "SCANNOS": [21,23], "POLNOS": pollist,\
              "IFNOS": iflist, "MOLNOS": [1], "RestFreq": restfreq}

    def setUp( self ):
        # copy input scantables
        if os.path.exists(self.infile):
            shutil.rmtree(self.infile)
        shutil.copytree(self.datapath+self.infile, self.infile)
        # back up the original settings
        self.storage = sd.rcParams['scantable.storage']
        self.insitu = sd.rcParams['insitu']

        default(tsdsave)

    def tearDown( self ):
        # restore settings
        sd.rcParams['scantable.storage'] = self.storage
        sd.rcParams['insitu'] = self.insitu
        if (os.path.exists(self.infile)):
            shutil.rmtree(self.infile)


    # Helper functions for testing
    def _get_scantable_params( self, scanname ):
        self._checkfile(scanname)
        res = {}
        testvals = ["scannos", "polnos", "ifnos", "molnos"]
        scan = sd.scantable(scanname,average=False)
        res['nRow'] = scan.nrow()
        for val in testvals:
            res[val.upper()] =  getattr(scan,"get"+val)()
        # rest frequencies
        rflist = []
        for molno in res["MOLNOS"]:
            rflist.append(scan.get_restfreqs(molno)[0])
        res["RestFreq"] = rflist
        del scan
        return res

    def _compare_scantable_params( self, test , refval):
        if type(test) == str:
            testval = self._get_scantable_params(test)
        elif type(test) == dict:
            testval = test
        else:
            msg = "Invalid test value (should be either dict or file name)."
            raise Exception, msg
        #print "Test data = ", testval
        #print "Ref data =  ", refval
        if not type(refval) == dict:
            raise Exception, "The reference data should be a dictionary"
        for key, rval in refval.iteritems():
            if not testval.has_key(key):
                raise KeyError, "Test data does not have key, '%s'" % key
            if type(rval) in [list, tuple, numpy.ndarray]:
                self.assertEqual(len(testval[key]), len(rval), \
                                 msg = "Number of elements in '%s' differs." % key)
                for i in range(len(rval)):
                    rv = rval[i]
                    if type(rv) == float:
                        self.assertAlmostEqual(testval[key][i], rv, \
                                               msg = "%s[%d] differs: %s (expected: %s) "\
                                               % (key, i, str(testval[key][i]), str(rv)))
                    else:
                        self.assertEqual(testval[key][i], rv, \
                                         msg = "%s[%d] differs: %s (expected: %s) "\
                                         % (key, i, str(testval[key][i]), str(rv)))
            else:
                if type(rval) == float:
                    self.assertAlmostEqual(testval[key], rval, \
                                     msg = "%s differs: %s (expected: %s)" \
                                     % (key, str(testval[key]), rval))
                else:
                    self.assertEqual(testval[key], rval, \
                                     msg = "%s differs: %s (expected: %s)" \
                                     % (key, str(testval[key]), rval))
    

    # Actual tests
    def testMT( self ):
        """Storage Test MT: tsdsave on storage='memory' and insitu=T"""
        tid = "MT"
        infile = self.infile
        outfile = self.outname+tid
        #iflist = self.iflist
        #pollist = self.pollist
        restfreq = self.restfreq

        # Backup units and coords of input scantable before run.
        initval = self._get_scantable_params(infile)

        sd.rcParams['scantable.storage'] = 'memory'
        sd.rcParams['insitu'] = True
        print "Running test with storage='%s' and insitu=%s" % \
              (sd.rcParams['scantable.storage'], str(sd.rcParams['insitu']))
        result = tsdsave(infile=infile,outfile=outfile,\
                        spw=self.spw,pol=self.pol,restfreq=restfreq)

        self.assertEqual(result,None)
        self.assertTrue(os.path.exists(outfile),msg="No output written")
        print "Testing output scantable"
        self._compare_scantable_params(outfile,self.refout)

        print "Comparing input scantable before/after run"
        self._compare_scantable_params(infile,initval)


    def testMF( self ):
        """Storage Test MF: tsdsave on storage='memory' and insitu=F"""
        tid = "MF"
        infile = self.infile
        outfile = self.outname+tid
        #iflist = self.iflist
        #pollist = self.pollist
        restfreq = self.restfreq

        # Backup units and coords of input scantable before run.
        initval = self._get_scantable_params(infile)

        sd.rcParams['scantable.storage'] = 'memory'
        sd.rcParams['insitu'] = False
        print "Running test with storage='%s' and insitu=%s" % \
              (sd.rcParams['scantable.storage'], str(sd.rcParams['insitu']))
        result = tsdsave(infile=infile,outfile=outfile,\
                        spw=self.spw,pol=self.pol,restfreq=restfreq)

        self.assertEqual(result,None)
        self.assertTrue(os.path.exists(outfile),msg="No output written")
        print "Testing output scantable"
        self._compare_scantable_params(outfile,self.refout)

        print "Comparing input scantable before/after run"
        self._compare_scantable_params(infile,initval)


    def testDT( self ):
        """Storage Test DT: tsdsave on storage='disk' and insitu=T"""
        tid = "DT"
        infile = self.infile
        outfile = self.outname+tid
        #iflist = self.iflist
        #pollist = self.pollist
        restfreq = self.restfreq

        # Backup units and coords of input scantable before run.
        initval = self._get_scantable_params(infile)

        sd.rcParams['scantable.storage'] = 'disk'
        sd.rcParams['insitu'] = True
        print "Running test with storage='%s' and insitu=%s" % \
              (sd.rcParams['scantable.storage'], str(sd.rcParams['insitu']))
        result = tsdsave(infile=infile,outfile=outfile,\
                        spw=self.spw,pol=self.pol,restfreq=restfreq)

        self.assertEqual(result,None)
        self.assertTrue(os.path.exists(outfile),msg="No output written")
        print "Testing output scantable"
        self._compare_scantable_params(outfile,self.refout)

        print "Comparing input scantable before/after run"
        self._compare_scantable_params(infile,initval)


    def testDF( self ):
        """Storage Test DF: tsdsave on storage='disk' and insitu=F"""
        tid = "DF"
        infile = self.infile
        outfile = self.outname+tid
        #iflist = self.iflist
        #pollist = self.pollist
        restfreq = self.restfreq

        # Backup units and coords of input scantable before run.
        initval = self._get_scantable_params(infile)

        sd.rcParams['scantable.storage'] = 'disk'
        sd.rcParams['insitu'] = False
        print "Running test with storage='%s' and insitu=%s" % \
              (sd.rcParams['scantable.storage'], str(sd.rcParams['insitu']))
        result = tsdsave(infile=infile,outfile=outfile,\
                        spw=self.spw,pol=self.pol,restfreq=restfreq)

        self.assertEqual(result,None)
        self.assertTrue(os.path.exists(outfile),msg="No output written")
        print "Testing output scantable"
        self._compare_scantable_params(outfile,self.refout)

        print "Comparing input scantable before/after run"
        self._compare_scantable_params(infile,initval)

###
# Test for frequency labeling
###
class tsdsave_freq_labeling(unittest.TestCase,tsdsave_unittest_base):
    """
    Read MS data, write various types of format.
    """
    # Input and output names
    infile='OrionS_rawACSmod_cal2123.ms'
    prefix=tsdsave_unittest_base.taskname+'Test2'
    outfile=prefix+'.ms'

    def setUp(self):
        self.res=None
        if (not os.path.exists(self.infile)):
            shutil.copytree(self.datapath+self.infile, self.infile)
        if (not os.path.exists(self.basefile)):
            shutil.copytree(self.datapath+self.basefile, self.basefile)

        default(tsdsave)
        self._setAttributes()
        self.scanno=0

    def tearDown(self):
        if (os.path.exists(self.infile)):
            shutil.rmtree(self.infile)
        if (os.path.exists(self.basefile)):
            shutil.rmtree(self.basefile)
        os.system( 'rm -rf '+self.prefix+'*' )

    def test_freq_labeling01(self):
        """test_freq_labeling01: test for frequency labeling, by importing MS and exporting data to MS and compare frequency label with original MS"""
        # edit SPECTRAL_WINDOW table
        tb.open(self.infile+'/SPECTRAL_WINDOW',nomodify=False)
        mfr=tb.getcol('MEAS_FREQ_REF')
        mfr[:]=5 # TOPO
        cf_ref=tb.getvarcol('CHAN_FREQ')
        tb.putcol('MEAS_FREQ_REF',mfr)
        tb.close()
        
        self.res=tsdsave(infile=self.infile,outfile=self.outfile,outform='MS2')
        self.assertEqual(self.res,None)

        # compare frequency label
        tb.open(self.outfile+'/SPECTRAL_WINDOW')
        cf=tb.getvarcol('CHAN_FREQ')
        tb.close()

        for k in cf.keys():
            v=cf[k]
            r=cf_ref[k]
            maxdiff=abs((v-r)/r).max()
            self.assertEqual(len(v), len(r),
                             msg="spw %s: length mismatch"%(int(k[1:])))
            self.assertTrue(maxdiff < 1.0e-15,
                            msg="spw %s: frequency labels differ"%(int(k[1:])))

###
# Test for handling flags in MSWriter
###
class tsdsave_flagging(unittest.TestCase,tsdsave_unittest_base):
    """
    Read Scantable data, modify flags in various ways, and write as MS.
    """
    # Input and output names
    infile='OrionS_rawACSmod_cal2123.asap'
    prefix=tsdsave_unittest_base.taskname+'Test2'
    outfile=prefix+'.ms'

    def setUp(self):
        self.res=None
        if (not os.path.exists(self.infile)):
            shutil.copytree(self.datapath+self.infile, self.infile)
        if (not os.path.exists(self.basefile)):
            shutil.copytree(self.datapath+self.basefile, self.basefile)

        default(tsdsave)
        self._setAttributes()
        self.scanno=0

    def tearDown(self):
        if (os.path.exists(self.infile)):
            shutil.rmtree(self.infile)
        if (os.path.exists(self.basefile)):
            shutil.rmtree(self.basefile)
        os.system( 'rm -rf '+self.prefix+'*' )

    def test_noflag(self):
        """
        test_noflag: test for unflagged data. All the flags must be False.
        """
        # execute task
        tsdsave(infile=self.infile, outfile=self.outfile, outform='MS2')

        # verification
        try:
            tb.open(self.outfile)
            for irow in xrange(tb.nrows()):
                flag_row = tb.getcell('FLAG_ROW', irow)
                flag = tb.getcell('FLAG', irow)
                self.assertFalse(flag_row,
                                 msg='FLAG_ROW must be False in row %s'%(irow))
                self.assertTrue(all(flag.flatten()==False),
                                msg='all FLAG values must be False in row %s'%(irow))
        finally:
            tb.close()
        
    def test_channelflag(self):
        """
        test_channelflag: test for channel flagged data. All the flags must be transferred properly.
        """
        # put channel flag to the first row
        tb.open(self.infile, nomodify=False)
        flagtra = tb.getcell('FLAGTRA', 0)
        flagtra[:] = 1
        tb.putcell('FLAGTRA', 0, flagtra)
        tb.close()

        # execute task
        tsdsave(infile=self.infile, outfile=self.outfile, outform='MS2')

        # verification
        try:
            tb.open(self.outfile)
            # all channels in first row must be flagged
            irow = 0
            flag_row = tb.getcell('FLAG_ROW', irow)
            flag = tb.getcell('FLAG', irow)
            self.assertFalse(flag_row,
                             msg='FLAG_ROW must be False in row %s'%(irow))
            self.assertTrue(all(flag[0]==True),
                            msg='FLAG values for first polarization must be True in row %s'%(irow))
            self.assertTrue(all(flag[1:].flatten()==False),
                            msg='FLAG values for other polarizations must be False in row %s'%(irow))
            for irow in xrange(1, tb.nrows()):
                flag_row = tb.getcell('FLAG_ROW', irow)
                flag = tb.getcell('FLAG', irow)
                self.assertFalse(flag_row,
                                 msg='FLAG_ROW must be False in row %s'%(irow))
                self.assertTrue(all(flag.flatten()==False),
                                msg='all FLAG values must be False in row %s'%(irow))
        finally:
            tb.close()        

    def test_rowflag(self):
        """
        test_rowflag: test for row flagged data. In this case, channel flags have to be set while row flags should not be set. 
        """
        # put row flag to the first row
        tb.open(self.infile, nomodify=False)
        tb.putcell('FLAGROW', 0, 1)
        tb.close()        
        
        # execute task
        tsdsave(infile=self.infile, outfile=self.outfile, outform='MS2')

        # verification
        try:
            tb.open(self.outfile)
            # all channels in first row must be flagged
            irow = 0
            flag_row = tb.getcell('FLAG_ROW', irow)
            flag = tb.getcell('FLAG', irow)
            self.assertFalse(flag_row,
                            msg='FLAG_ROW must be False in row %s'%(irow))
            self.assertTrue(all(flag[0]==True),
                            msg='FLAG values for first polarization must be True in row %s'%(irow))
            self.assertTrue(all(flag[1:].flatten()==False),
                            msg='FLAG values for other polarizations must be False in row %s'%(irow))

            for irow in xrange(1, tb.nrows()):
                flag_row = tb.getcell('FLAG_ROW', irow)
                flag = tb.getcell('FLAG', irow)
                self.assertFalse(flag_row,
                                 msg='FLAG_ROW must be False in row %s'%(irow))
                self.assertTrue(all(flag.flatten()==False),
                                msg='all FLAG values must be False in row %s'%(irow))
        finally:
            tb.close()        

###
# Test for scan number (CAS-5841)
###
class tsdsave_scan_number(unittest.TestCase,tsdsave_unittest_base):
    """
    Check scan number
    """
    # Input and output names
    infile='OrionS_rawACSmod_cal2123.asap'
    vis='OrionS_rawACSmod_cal2123.ms'
    prefix=tsdsave_unittest_base.taskname+'Test2'
    outfile=prefix+'.asap'
    outvis=prefix+'.ms'

    def setUp(self):
        self.res=None
        if (not os.path.exists(self.infile)):
            shutil.copytree(self.datapath+self.infile, self.infile)
        if (not os.path.exists(self.vis)):
            shutil.copytree(self.datapath+self.vis, self.vis)

        default(tsdsave)
        #self._setAttributes()
        #self.scanno=0

    def tearDown(self):
        if (os.path.exists(self.infile)):
            shutil.rmtree(self.infile)
        if (os.path.exists(self.vis)):
            shutil.rmtree(self.vis)
        os.system( 'rm -rf '+self.prefix+'*' )

    def test_import(self):
        """Test if SCANNO is consistent with original MS."""
        self.res = tsdsave(infile=self.vis, outfile=self.outfile,
                          outform='ASAP')

        # compare scan number
        tb.open(self.vis)
        scan_number_org = numpy.unique(tb.getcol('SCAN_NUMBER'))
        tb.close()
        tb.open(self.outfile)
        scan_number = numpy.unique(tb.getcol('SCANNO'))
        tb.close()
        print 'scan_number_org=', scan_number_org
        print 'scan_number=', scan_number
        self.assertEqual(len(scan_number_org), len(scan_number))
        self.assertTrue(all(scan_number_org == scan_number))

    def test_export(self):
        """Test if SCAN_NUMBER is consistent with original Scantable."""
        self.res = tsdsave(infile=self.infile, outfile=self.outvis,
                          outform='MS2')

        # compare scan number
        tb.open(self.infile)
        scan_number_org = numpy.unique(tb.getcol('SCANNO'))
        tb.close()
        tb.open(self.outvis)
        scan_number = numpy.unique(tb.getcol('SCAN_NUMBER'))
        tb.close()
        print 'scan_number_org=', scan_number_org
        print 'scan_number=', scan_number
        self.assertEqual(len(scan_number_org), len(scan_number))
        self.assertTrue(all(scan_number_org == scan_number))

###
# Test for splitant (CAS-5842)
###
class tsdsave_test_splitant(unittest.TestCase,tsdsave_unittest_base):
    """
    Read MS data, write various types of format.
    """
    # Input and output names
    infile='uid___A002_X6321c5_X3a7.ms'
    prefix=tsdsave_unittest_base.taskname+'TestSplitant'
    outfile=prefix+'.asap'

    def setUp(self):
        self.res=None
        if (not os.path.exists(self.infile)):
            shutil.copytree(self.datapath+self.infile, self.infile)
        if (not os.path.exists(self.basefile)):
            shutil.copytree(self.datapath+self.basefile, self.basefile)

        default(tsdsave)
        self._setAttributes()
        self.scanno=1

    def tearDown(self):
        if (os.path.exists(self.infile)):
            shutil.rmtree(self.infile)
        if (os.path.exists(self.basefile)):
            shutil.rmtree(self.basefile)
        os.system( 'rm -rf '+self.prefix+'*' )

    def testSplitant(self):
        """Test Splitant: test for splitant"""
        self.res=tsdsave(infile=self.infile,splitant=True,outfile=self.outfile,outform='ASAP')
        outsplitfile1 = self.prefix+'_PM01.asap'
        outsplitfile2 = self.prefix+'_PM04.asap'
        if (os.path.exists(outsplitfile1)) and (os.path.exists(outsplitfile2)):
            s0 = sd.scantable(self.infile, False)
            s1 = sd.scantable(outsplitfile1, False)
            s2 = sd.scantable(outsplitfile2, False)
            
            self.assertEqual(s0.nrow(), s1.nrow())
            self.assertEqual(s0.nrow(), s2.nrow())

            s0sp0 = s0.get_spectrum(0)
            s1sp0 = s1.get_spectrum(0)
            for i in range(len(s0sp0)):
                self.assertEqual(s0sp0[i], s1sp0[i])
            
            del s0, s1, s2

###
# Test data selection
###
class tsdsave_data_selection(tsdsave_unittest_base, unittest.TestCase):
    infile = 'data_selection.asap'
    prefix = 'selected.asap'
    
    def setUp(self):
        if (not os.path.exists(self.infile)):
            shutil.copytree(self.datapath+self.infile, self.infile)
        default(tsdsave)

    def tearDown(self):
        if (os.path.exists(self.infile)):
            shutil.rmtree(self.infile)
        os.system( 'rm -rf '+self.prefix+'*' )

    def __exec_complex_test(self, params, exprs, values, columns, expected_nrow):
        num_param = len(params)
        stack = inspect.stack()
        x = 0
        while re.match('^test_.*[0-9]$',stack[x][3]) is None and x < len(stack):
            x += 1
        test_name = inspect.stack()[x][3]
        outfile = '.'.join([self.prefix, test_name])
        #print 'outfile=%s'%(outfile)
        casalog.post('%s: %s'%(test_name, ','.join(['%s = \'%s\''%(params[i],exprs[i]) for i in xrange(num_param)])))
        kwargs = {'infile': self.infile,
                  'outfile': outfile,
                  'overwrite': True}
        for i in xrange(num_param):
            kwargs[params[i]] = exprs[i]
        tsdsave(**kwargs)

        tb.open(outfile)
        cols = [tb.getcol(columns[i]) for i in xrange(num_param)]
        nrow = tb.nrows()
        tb.close()
        casalog.post('expected nrow = %s, actual nrow = %s'%(expected_nrow, nrow))
        self.assertEqual(expected_nrow, nrow)
        for i in xrange(num_param):
            casalog.post('expected values = %s, actual values = %s'%(set(values[i]), set(cols[i])))
            self.assertTrue(set(values[i]) == set(cols[i]))
        return outfile
        
    def __exec_simple_test(self, param, expr, value_list, column, expected_nrow):
        return self.__exec_complex_test([param], [expr], [value_list], [column],
                                        expected_nrow)
        
    def exec_spw_test(self, iflist, spw, expected_nrow):
        self.__exec_simple_test('spw', spw, iflist, 'IFNO', expected_nrow)

    def exec_scan_test(self, scanlist, scan, expected_nrow):
        self.__exec_simple_test('scan', scan, scanlist, 'SCANNO', expected_nrow)

    def exec_pol_test(self, pollist, pol, expected_nrow):
        self.__exec_simple_test('pol', pol, pollist, 'POLNO', expected_nrow)
                
    def exec_beam_test(self, beamlist, beam, expected_nrow):
        self.__exec_simple_test('beam', beam, beamlist, 'BEAMNO', expected_nrow)

    def exec_field_test(self, fieldlist, field, expected_nrow):
        self.__exec_simple_test('field', field, fieldlist, 'FIELDNAME', expected_nrow)

    def exec_timerange_test(self, timelist, timerange, expected_nrow):
        self.__exec_simple_test('timerange', timerange, timelist, 'TIME', expected_nrow)

    def exec_channelrange_test(self, iflist, channelrange, spw, expected_nrow):
        outfile = self.__exec_simple_test('spw', spw, iflist, 'IFNO',
                                          expected_nrow)
        s_org = sd.scantable(self.infile, average=True)
        s = sd.scantable(outfile, average=False)
        expected_nchan = channelrange[1] - channelrange[0] + 1
        self.assertEqual(expected_nchan, s.nchan(iflist[0]))
        u_org_org = s_org.get_unit()
        u_org = s.get_unit()
        s_org.set_unit('GHz')
        s.set_unit('GHz')
        sel = sd.selector()
        sel.set_ifs(iflist)
        s_org.set_selection(sel)
        f_org = s_org._getabcissa(0)
        f = s._getabcissa(0)
        s_org.set_selection()
        s_org.set_unit(u_org_org)
        s.set_unit(u_org)
        casalog.post('left edge expected: %s, actual: %s'%(f_org[channelrange[0]],f[0]))
        casalog.post('right edge expected: %s, actual: %s'%(f_org[channelrange[1]],f[-1]))
        self.assertEqual(f_org[channelrange[0]], f[0])
        self.assertEqual(f_org[channelrange[1]], f[-1])
        
    def test_spw0(self):
        """test_spw0: Test spw selection by syntax 'N'"""
        iflist = [1]
        spw = '%s'%(iflist[0])
        expected_nrow = 16
        
        self.exec_spw_test(iflist, spw, expected_nrow)
        
    def test_spw1(self):
        """test_spw1: Test spw selection by syntax 'N~M'"""
        iflist = [0,1,2]
        spw = '%s~%s'%(min(iflist),max(iflist))
        expected_nrow = 48

        self.exec_spw_test(iflist, spw, expected_nrow)

    def test_spw2(self):
        """test_spw2: Test spw selection by syntax '<N'"""
        iflist = [0,1]
        spw = '<%s'%(max(iflist)+1)
        expected_nrow = 32

        self.exec_spw_test(iflist, spw, expected_nrow)

    def test_spw3(self):
        """test_spw3: Test spw selection by syntax '>N'"""
        iflist = [2,3]
        spw = '>%s'%(min(iflist)-1)
        expected_nrow = 32

        self.exec_spw_test(iflist, spw, expected_nrow)

    #def test_spw4(self):
    #    """test_spw4: Test spw selection by syntax '<=N'"""
    #    iflist = [0,1]
    #    spw = '<=%s'%(max(iflist))
    #    expected_nrow = 32

    #    self.exec_spw_test(iflist, spw, expected_nrow)

    #def test_spw5(self):
    #    """test_spw5: Test spw selection by syntax '>=N'"""
    #    iflist = [2,3]
    #    spw = '>=%s'%(min(iflist))
    #    expected_nrow = 32

    #    self.exec_spw_test(iflist, spw, expected_nrow)
        
    def test_spw6(self):
        """test_spw6: Test spw selection by syntax 'N,M'"""
        iflist = [1,2,3]
        spw = ','.join(map(str,iflist))
        expected_nrow = 48

        self.exec_spw_test(iflist, spw, expected_nrow)

    #def test_spw7(self):
    #    """test_spw7: Test spw selection by syntax 'N;M'"""
    #    iflist = [1,2,3]
    #    spw = ';'.join(map(str,iflist))
    #    expected_nrow = 48

    #    self.exec_spw_test(iflist, spw, expected_nrow)

    def test_spw8(self):
        """test_spw8: Test spw selection by syntax 'EXP0,EXP1'"""
        iflist = [0,2,3]
        spw='0,2~3'
        expected_nrow = 48

        self.exec_spw_test(iflist, spw, expected_nrow)

    def test_scan0(self):
        """test_scan0: Test scan selection by syntax 'N'"""
        scanlist = [1]
        scan = '%s'%(scanlist[0])
        expected_nrow = 16
        
        self.exec_scan_test(scanlist, scan, expected_nrow)
        
    def test_scan1(self):
        """test_scan1: Test scan selection by syntax 'N~M'"""
        scanlist = [0,1,2]
        scan = '%s~%s'%(min(scanlist),max(scanlist))
        expected_nrow = 48

        self.exec_scan_test(scanlist, scan, expected_nrow)

    def test_scan2(self):
        """test_scan2: Test scan selection by syntax '<N'"""
        scanlist = [0,1]
        scan = '<%s'%(max(scanlist)+1)
        expected_nrow = 32

        self.exec_scan_test(scanlist, scan, expected_nrow)

    def test_scan3(self):
        """test_scan3: Test scan selection by syntax '>N'"""
        scanlist = [2,3]
        scan = '>%s'%(min(scanlist)-1)
        expected_nrow = 32

        self.exec_scan_test(scanlist, scan, expected_nrow)

    def test_scan4(self):
        """test_scan4: Test scan selection by syntax '<=N'"""
        scanlist = [0,1]
        scan = '<=%s'%(max(scanlist))
        expected_nrow = 32

        self.exec_scan_test(scanlist, scan, expected_nrow)

    def test_scan5(self):
        """test_scan5: Test scan selection by syntax '>=N'"""
        scanlist = [2,3]
        scan = '>=%s'%(min(scanlist))
        expected_nrow = 32

        self.exec_scan_test(scanlist, scan, expected_nrow)
        
    def test_scan6(self):
        """test_scan6: Test scan selection by syntax 'N,M'"""
        scanlist = [1,2,3]
        scan = ','.join(map(str,scanlist))
        expected_nrow = 48

        self.exec_scan_test(scanlist, scan, expected_nrow)

    def test_scan7(self):
        """test_scan7: Test scan selection by syntax 'EXP0,EXP1'"""
        scanlist = [0,2,3]
        scan='0,2~3'
        expected_nrow = 48

        self.exec_scan_test(scanlist, scan, expected_nrow)

    def test_pol0(self):
        """test_pol0: Test pol selection by syntax 'N'"""
        pollist = [1]
        pol = '%s'%(pollist[0])
        expected_nrow = 16
        
        self.exec_pol_test(pollist, pol, expected_nrow)
        
    def test_pol1(self):
        """test_pol1: Test pol selection by syntax 'N~M'"""
        pollist = [0,1,2]
        pol = '%s~%s'%(min(pollist),max(pollist))
        expected_nrow = 48

        self.exec_pol_test(pollist, pol, expected_nrow)

    def test_pol2(self):
        """test_pol2: Test pol selection by syntax '<N'"""
        pollist = [0,1]
        pol = '<%s'%(max(pollist)+1)
        expected_nrow = 32

        self.exec_pol_test(pollist, pol, expected_nrow)

    def test_pol3(self):
        """test_pol3: Test pol selection by syntax '>N'"""
        pollist = [2,3]
        pol = '>%s'%(min(pollist)-1)
        expected_nrow = 32

        self.exec_pol_test(pollist, pol, expected_nrow)

    def test_pol4(self):
        """test_pol4: Test pol selection by syntax '<=N'"""
        pollist = [0,1]
        pol = '<=%s'%(max(pollist))
        expected_nrow = 32

        self.exec_pol_test(pollist, pol, expected_nrow)

    def test_pol5(self):
        """test_pol5: Test pol selection by syntax '>=N'"""
        pollist = [2,3]
        pol = '>=%s'%(min(pollist))
        expected_nrow = 32

        self.exec_pol_test(pollist, pol, expected_nrow)
        
    def test_pol6(self):
        """test_pol6: Test pol selection by syntax 'N,M'"""
        pollist = [1,2,3]
        pol = ','.join(map(str,pollist))
        expected_nrow = 48

        self.exec_pol_test(pollist, pol, expected_nrow)

    def test_pol7(self):
        """test_pol7: Test pol selection by syntax 'EXP0,EXP1'"""
        pollist = [0,2,3]
        pol='0,2~3'
        expected_nrow = 48

    def test_beam0(self):
        """test_beam0: Test beam selection by syntax 'N'"""
        beamlist = [1]
        beam = '%s'%(beamlist[0])
        expected_nrow = 16
        
        self.exec_beam_test(beamlist, beam, expected_nrow)
        
    def test_beam1(self):
        """test_beam1: Test beam selection by syntax 'N~M'"""
        beamlist = [0,1,2]
        beam = '%s~%s'%(min(beamlist),max(beamlist))
        expected_nrow = 48

        self.exec_beam_test(beamlist, beam, expected_nrow)

    def test_beam2(self):
        """test_beam2: Test beam selection by syntax '<N'"""
        beamlist = [0,1]
        beam = '<%s'%(max(beamlist)+1)
        expected_nrow = 32

        self.exec_beam_test(beamlist, beam, expected_nrow)

    def test_beam3(self):
        """test_beam3: Test beam selection by syntax '>N'"""
        beamlist = [2,3]
        beam = '>%s'%(min(beamlist)-1)
        expected_nrow = 32

        self.exec_beam_test(beamlist, beam, expected_nrow)

    def test_beam4(self):
        """test_beam4: Test beam selection by syntax '<=N'"""
        beamlist = [0,1]
        beam = '<=%s'%(max(beamlist))
        expected_nrow = 32

        self.exec_beam_test(beamlist, beam, expected_nrow)

    def test_beam5(self):
        """test_beam5: Test beam selection by syntax '>=N'"""
        beamlist = [2,3]
        beam = '>=%s'%(min(beamlist))
        expected_nrow = 32

        self.exec_beam_test(beamlist, beam, expected_nrow)
        
    def test_beam6(self):
        """test_beam6: Test beam selection by syntax 'N,M'"""
        beamlist = [1,2,3]
        beam = ','.join(map(str,beamlist))
        expected_nrow = 48

        self.exec_beam_test(beamlist, beam, expected_nrow)

    def test_beam7(self):
        """test_beam7: Test beam selection by syntax 'EXP0,EXP1'"""
        beamlist = [0,2,3]
        beam='0,2~3'
        expected_nrow = 48

        self.exec_beam_test(beamlist, beam, expected_nrow)

    def test_field0(self):
        """test_field0: Test field selection by name"""
        field = 'M100'
        expected_nrow = 48
        fieldlist = ['M100__0', 'M100__1']

        self.exec_field_test(fieldlist, field, expected_nrow)

    def test_field1(self):
        """test_field1: Test field selection by wildcard"""
        field = 'M*'
        expected_nrow = 64
        fieldlist = ['M100__0', 'M100__1', 'M42__2']

        self.exec_field_test(fieldlist, field, expected_nrow)

    def test_field2(self):
        """test_field2: Test field selection by id ('N')"""
        field = '1'
        expected_nrow = 16
        fieldlist = ['M100__1']

        self.exec_field_test(fieldlist, field, expected_nrow)

    def test_field3(self):
        """test_field3: Test field selection by id ('N~M')"""
        field = '1~2'
        expected_nrow = 32
        fieldlist = ['M100__1', 'M42__2']

        self.exec_field_test(fieldlist, field, expected_nrow)

    def test_field4(self):
        """test_field4: Test field selection by id ('<N')"""
        field = '<1'
        expected_nrow = 32
        fieldlist = ['M100__0']

        self.exec_field_test(fieldlist, field, expected_nrow)

    def test_field5(self):
        """test_field5: Test field selection by id ('>N')"""
        field = '>1'
        expected_nrow = 16
        fieldlist = ['M42__2']

        self.exec_field_test(fieldlist, field, expected_nrow)

    def test_field6(self):
        """test_field6: Test field selection by id ('<=N')"""
        field = '<=0'
        expected_nrow = 32
        fieldlist = ['M100__0']

        self.exec_field_test(fieldlist, field, expected_nrow)

    def test_field7(self):
        """test_field7: Test field selection by id ('>=N')"""
        field = '>=2'
        expected_nrow = 16
        fieldlist = ['M42__2']

        self.exec_field_test(fieldlist, field, expected_nrow)

    def test_field8(self):
        """test_field8: Test field selection by id ('N,M')"""
        field = '0,2'
        expected_nrow = 48
        fieldlist = ['M100__0', 'M42__2']

        self.exec_field_test(fieldlist, field, expected_nrow)

    def test_field9(self):
        """test_field3: Test field selection by id ('EXP0,EXP1')"""
        field = '0,>=2'
        expected_nrow = 48
        fieldlist = ['M100__0', 'M42__2']

        self.exec_field_test(fieldlist, field, expected_nrow)

    def test_field10(self):
        """test_field3: Test field selection by name and id"""
        field = '0,M4*'
        expected_nrow = 48
        fieldlist = ['M100__0', 'M42__2']

        self.exec_field_test(fieldlist, field, expected_nrow)

    def test_timerange0(self):
        """test_timerange0: Test timerange selection by syntax 'T0~T1'"""
        timerange = '2010/04/24/23:13:00~2010/04/24/23:43:49'
        expected_nrow = 32
        timelist = [55310.967928148144, 55310.988761481472]

        self.exec_timerange_test(timelist, timerange, expected_nrow)
        
    def test_timerange1(self):
        """test_timerange1: Test timerange selection by syntax 'T0'"""
        timerange = '2010/04/24/23:43:48.5'
        expected_nrow = 16
        timelist = [55310.988761481472]

        self.exec_timerange_test(timelist, timerange, expected_nrow)
 
    def test_timerange2(self):
        """test_timerange2: Test timerange selection by syntax 'T0+dT'"""
        timerange = '2010/04/24/22:43:30+0:0:30'
        expected_nrow = 16
        timelist = [55310.947094814808]

        self.exec_timerange_test(timelist, timerange, expected_nrow)

    def test_timerange3(self):
        """test_timerange3: Test timerange selection by syntax '>T0'"""
        timerange = '>2010/04/24/23:14:00'
        expected_nrow = 32
        timelist = [55310.988761481472, 55311.009594814808]

        self.exec_timerange_test(timelist, timerange, expected_nrow)

    def test_timerange4(self):
        """test_timerange4: Test timerange selection by syntax '<T0'"""
        timerange = '<2010/04/24/23:13:00'
        expected_nrow = 16
        timelist = [55310.947094814808]
        
        self.exec_timerange_test(timelist, timerange, expected_nrow)

    def test_simultaneous0(self):
        """test_simultaneous0: Test simultaneous selection (spw and scan)"""
        iflist = [1]
        spw = '%s'%(iflist[0])
        scanlist = [0,2]
        scan = '0,2'
        expected_nrow = 8

        self.__exec_complex_test(['spw', 'scan'], [spw, scan],
                                 [iflist, scanlist], ['IFNO', 'SCANNO'],
                                 expected_nrow)

    def test_simultaneous1(self):
        """test_simultaneous1: Test simultaneous selection (spw and field)"""
        iflist = [1,2]
        spw = '1~2'
        fieldlist = ['M100__0', 'M42__2']
        field = '0,2'
        expected_nrow = 24

        self.__exec_complex_test(['spw', 'field'], [spw, field],
                                 [iflist, fieldlist], ['IFNO', 'FIELDNAME'],
                                 expected_nrow)

    def test_simultaneous2(self):
        """test_simultaneous2: Test simultaneous selection (spw and timerange)"""
        iflist = [1,2]
        spw = '1~2'
        timelist = [55310.967928148144, 55310.988761481472]
        timerange = '2010/04/24/23:13:00~2010/04/24/23:43:49'
        expected_nrow = 16

        self.__exec_complex_test(['spw', 'timerange'], [spw, timerange],
                                 [iflist, timelist], ['IFNO', 'TIME'],
                                 expected_nrow)


    def test_simultaneous3(self):
        """test_simultaneous3: Test simultaneous selection (field and timerange)"""
        fieldlist = ['M100__0']
        field = '<1'
        timelist = [55310.947094814808]
        timerange = '<2010/04/24/23:00:00'
        expected_nrow = 16

        self.__exec_complex_test(['field', 'timerange'], [field, timerange],
                                 [fieldlist, timelist], ['FIELDNAME', 'TIME'],
                                 expected_nrow)
        

    def test_simultaneous4(self):
        """test_simultaneous4: Test simultaneous selection (spw, field and timerange)"""
        iflist = [1,3]
        spw = '1,>2'
        fieldlist = ['M100__0']
        field = '<1'
        timelist = [55310.947094814808]
        timerange = '<2010/04/24/23:00:00'
        expected_nrow = 8

        self.__exec_complex_test(['spw', 'field', 'timerange'],
                                 [spw, field, timerange],
                                 [iflist, fieldlist, timelist],
                                 ['IFNO', 'FIELDNAME', 'TIME'],
                                 expected_nrow)

    def test_channelrange0(self):
        """test_channelrange0: Test channel range selection '0~N'"""
        iflist = [2]
        channelrange = [0,100]
        spw = '2:0~100'
        expected_nrow = 16

        self.exec_channelrange_test(iflist, channelrange, spw, expected_nrow)

    def test_channelrange1(self):
        """test_channelrange1: Test channel range selection 'N~M'"""
        iflist = [2]
        channelrange = [200,400]
        spw = '2:200~400'
        expected_nrow = 16

        self.exec_channelrange_test(iflist, channelrange, spw, expected_nrow)

    def test_channelrange2(self):
        """test_channelrange2: Test channel range selection by frequency 'X~YGHz'"""
        iflist = [2]
        channelrange = [200,400]
        spw = '2:114.5802~114.6290GHz'
        expected_nrow = 16

        self.exec_channelrange_test(iflist, channelrange, spw, expected_nrow)

    def test_channelrange3(self):
        """test_channelrange3: Test multiple channel range for one spw (causes error)"""
        test_name = inspect.stack()[0][3]
        outfile = '.'.join([self.prefix, test_name])
        spw = '2:0~100;200~400'

        try:
            tsdsave(infile=self.infile, spw=spw, outfile=outfile, overwrite=True)
            self.assertTrue(False,
                            msg='The task must throw exception')
        except Exception, e:
            self.assertTrue(isinstance(e, SyntaxError),
                            msg='Unexpected exception was thrown: %s'%(str(e)))
            pos = str(e).find('tsdsave doesn\'t support multiple channel range selection for spw.')
            self.assertNotEqual(pos, -1,
                                msg='Unexpected exception was thrown: %s'%(str(e)))

        
def suite():
    return [tsdsave_test0,tsdsave_test1,tsdsave_test2,
            tsdsave_test3,tsdsave_test4,tsdsave_test5,
            tsdsave_test6,tsdsave_test7,tsdsave_storageTest,
            tsdsave_freq_labeling,tsdsave_flagging,
            tsdsave_scan_number, tsdsave_test_splitant,
            tsdsave_data_selection]