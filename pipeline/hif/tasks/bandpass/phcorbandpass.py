from __future__ import absolute_import

import pipeline.infrastructure.basetask as basetask
import pipeline.infrastructure.casatools as casatools
import pipeline.domain.measures as measures
import pipeline.infrastructure.logging as logging
from . import bandpassworker
from . import bandpassmode
from .. import gaincal

LOG = logging.get_logger(__name__)


class PhcorBandpassInputs(bandpassmode.BandpassModeInputs):
    phaseup       = basetask.property_with_default('phaseup', True)
    phaseupbw     = basetask.property_with_default('phaseupbw', '500MHz')
    phaseupsolint = basetask.property_with_default('phaseupsolint', 'int')
    solint        = basetask.property_with_default('solint', 'inf,7.8125MHz')
    
    def __init__(self,
                 # parameters for BandpassModeInputs:
                 context, mode='channel',
                 # parameters specific to this task:
                 phaseup=None, phaseupbw=None, phaseupsolint=None,
                 # parameters overridden by this task:
                 solint=None,
                 # other parameters to be passed to child bandpass task
                 **parameters):
        super(PhcorBandpassInputs, self).__init__(context, mode=mode,
            phaseup=phaseup, phaseupbw=phaseupbw, phaseupsolint=phaseupsolint,
            solint=solint, **parameters)


class PhcorBandpass(bandpassworker.BandpassWorker):
    Inputs = PhcorBandpassInputs

    def prepare(self, **parameters):
        inputs = self.inputs
        
        # if requested, execute a phaseup job. This will add the resulting
        # caltable to the on-the-fly calibration context, so we don't need any
        # subsequent gaintable manipulation
        if inputs.phaseup: 
            phaseup_result = self._do_phaseup()

        # Now perform the bandpass
        result = self._do_bandpass()
        
        # Attach the preparatory result to the final result so we have a
        # complete log of all the executed tasks. 
        if inputs.phaseup:
            result.preceding.append(phaseup_result.final)

        return result

    def _do_phaseup(self):
        inputs = self.inputs

        phaseup_inputs = gaincal.GTypeGaincal.Inputs(inputs.context,
            vis      = inputs.vis,
            field    = inputs.field,
            spw      = self._get_phaseup_spw(),
            antenna  = inputs.antenna,
            intent   = inputs.intent,
            solint   = inputs.phaseupsolint,
            refant   = inputs.refant,
            calmode  = 'p',
            minsnr   = inputs.minsnr)

        phaseup_task = gaincal.GTypeGaincal(phaseup_inputs)
        
        return self._executor.execute(phaseup_task, merge=True)

    def _do_bandpass(self):
        orig_run_qa2 = self.inputs.run_qa2
        try:
            self.inputs.run_qa2 = False
            bandpass_task = bandpassmode.BandpassMode(self.inputs)
            return self._executor.execute(bandpass_task)
        finally:
            self.inputs.run_qa2 = orig_run_qa2

    def _get_phaseup_spw(self):
        '''
	           ms -- measurement set object 
	       spwstr -- comma delimited list of spw ids
	    bandwidth -- bandwidth in Hz of central channels used to
	                 phaseup
    	'''
        inputs = self.inputs

        # Add the channel ranges in. Note that this currently assumes no prior
        # channel selection.
        if inputs.phaseupbw == '':
            return inputs.spw

        # Convert bandwidth input to CASA quantity and then on to pipeline
        # domain Frequency object
        quanta = casatools.quanta
        bw_quantity = quanta.convert(quanta.quantity(inputs.phaseupbw), 'Hz')        
        bandwidth = measures.Frequency(quanta.getvalue(bw_quantity)[0], 
                                       measures.FrequencyUnits.HERTZ)
    
        # Loop over the spws creating a new list with channel ranges
        outspw = []
        for spw in self.inputs.ms.get_spectral_windows(self.inputs.spw):
            cen_freq = spw.centre_frequency 
            lo_freq = cen_freq - bandwidth / 2.0  
            hi_freq = cen_freq + bandwidth / 2.0
            minchan, maxchan = spw.channel_range(lo_freq, hi_freq)
            cmd = '{0}:{1}~{2}'.format(spw.id, minchan, maxchan)
            outspw.append(cmd)
    
        return ','.join(outspw)
