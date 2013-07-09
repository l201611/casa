from __future__ import absolute_import

import pipeline.infrastructure.basetask as basetask
from pipeline.infrastructure import casa_tasks
import pipeline.infrastructure.casatools as casatools
import pipeline.domain.measures as measures
import pipeline.infrastructure as infrastructure
import pipeline.infrastructure.callibrary as callibrary

import itertools

from pipeline.hif.tasks import gaincal
from pipeline.hif.tasks import bandpass
from pipeline.hif.tasks import applycal
from pipeline.vla.heuristics import getCalFlaggedSoln, getBCalStatistics

LOG = infrastructure.get_logger(__name__)

#Utility from the EVLA scripted pipeline
def uniq(inlist):
   uniques = []
   for item in inlist:
      if item not in uniques:
         uniques.append(item)
   return uniques


class UncalspwInputs(basetask.StandardInputs):
    def __init__(self, context, vis=None):
        # set the properties to the values given as input arguments
        self._init_properties(vars())

class UncalspwResults(basetask.Results):
    def __init__(self, jobs=[]):
        super(UncalspwResults, self).__init__()

        self.jobs=jobs
        
    def __repr__(self):
        s = 'Uncalspw results:\n'
        for job in self.jobs:
            s += '%s performed. Statistics to follow?' % str(job)
        return s 

class Uncalspw(basetask.StandardTaskTemplate):
    Inputs = UncalspwInputs
    
    def prepare(self):
        
        method_args = {'delaycaltable' : 'testdelay.k',
                       'bpcaltable' : 'testBPcal.b'}
        
        uncalspw_result = self._do_uncalspw(**method_args)
        
        return uncalspw_result
        
    def _do_uncalspw(self, delaycaltable=None, bpcaltable=None):
        
        m = self.inputs.context.observing_run.measurement_sets[0]
        myscans = self.inputs.context.evla['msinfo'][m.name].scandict
        
        myspw = []
        for idd in myscans['DataDescription'].keys():
            ispw = myscans['DataDescription'][idd]['spw']
            if myspw.count(ispw)<1:
                myspw.append(ispw)
        
        calflagresult = getCalFlaggedSoln(delaycaltable)
        goodspw = []
        for ispw in calflagresult['spw'].keys():
            tot = 0.0
            flagd = 0.0
            for ipol in calflagresult['spw'][ispw].keys():
                tot += calflagresult['spw'][ispw][ipol]['total']
                flagd += calflagresult['spw'][ispw][ipol]['flagged']
            if tot>0:
                fract = flagd/tot
                if fract<1.0:
                    goodspw.append(ispw)
        
        flagspwlist = []
        flagspw = ''
        for ispw in myspw:
            if goodspw.count(ispw)<1:
                flagspwlist.append(ispw)
                if flagspw=='':
                    flagspw = str(ispw)
                else:
                    flagspw += ','+str(ispw)
        
        calflagresult = getCalFlaggedSoln(bpcaltable)
        goodspw = []
        for ispw in calflagresult['spw'].keys():
            tot = 0.0
            flagd = 0.0
            for ipol in calflagresult['spw'][ispw].keys():
                tot += calflagresult['spw'][ispw][ipol]['total']
                flagd += calflagresult['spw'][ispw][ipol]['flagged']
            if tot>0:
                fract = flagd/tot
                if fract<1.0:
                    goodspw.append(ispw)
        
        for ispw in myspw:
            if goodspw.count(ispw)<1:
                flagspwlist.append(ispw)
                if flagspw=='':
                    flagspw = str(ispw)
                else:
                    flagspw += ','+str(ispw)
        
        flagspw1 = ','.join(["%s" % ii for ii in uniq(flagspwlist)])
        
        if (flagspw1 == ''):
            LOG.info("All spws have calibration")
            return None
        else:
            LOG.info("No calibration found for spw(s) "+flagspw1+", flagging these spws in the ms")
            
            task_args = {'vis'        : self.inputs.vis,
                         'mode'       : 'list',
                         'action'     : 'apply',                     
                         'spw'    : flagspw1,
                         'savepars'   : True,
                         'flagbackup' : True}
            
            job = casa_tasks.flagdata(**task_args)
            
            self._executor.execute(job)
                
            return UncalspwResults([job])

        
    def analyse(self, results):
	return results