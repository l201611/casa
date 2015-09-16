from __future__ import absolute_import
import os
import os.path

import pipeline.infrastructure as infrastructure
import pipeline.infrastructure.basetask as basetask
import pipeline.infrastructure.casatools as casatools
import pipeline.infrastructure.callibrary as callibrary

from pipeline.hif.tasks import importdata
from pipeline.hif.tasks import gaincal
from ... import heuristics
from pipeline.hif.tasks.common import commonfluxresults
from pipeline.hif.tasks.fluxscale import fluxscale
from pipeline.hif.tasks.setmodel import setjy

LOG = infrastructure.get_logger(__name__)


class GcorFluxscaleInputs(fluxscale.FluxscaleInputs):
    @basetask.log_equivalent_CASA_call
    def __init__(self, context, output_dir=None, vis=None, caltable=None,
                 fluxtable=None, reference=None, transfer=None, 
                 refspwmap=None, refintent=None, transintent=None,
                 solint=None, phaseupsolint=None, minsnr=None, refant=None,
                 hm_resolvedcals=None, antenna=None, uvrange=None, peak_fraction=None):
        self._init_properties(vars())

    @property
    def solint(self):
        if self._solint is None:
            return 'inf'
        return self._solint
    
    @solint.setter
    def solint(self, value):
        self._solint = value

    @property
    def phaseupsolint(self):
        if self._phaseupsolint is None:
            return 'int'
        return self._phaseupsolint

    @phaseupsolint.setter
    def phaseupsolint(self, value):
        self._phaseupsolint = value

    @property
    def minsnr (self):
        if self._minsnr is None:
            return 2.0
        return self._minsnr

    @minsnr.setter
    def minsnr(self, value):
        self._minsnr = value

    @property
    def refant(self):
        if self._refant is None:
            return ''
        return self._refant

    @refant.setter
    def refant(self, value):
        self._refant = value

    @property
    def refspwmap(self):
        if self._refspwmap is None:
            return []
        return self._refspwmap

    @refspwmap.setter
    def refspwmap(self, value):
        self._refspwmap = value

    @property
    def hm_resolvedcals(self):
        if self._hm_resolvedcals is None:
            return 'automatic'
        return self._hm_resolvedcals

    @hm_resolvedcals.setter
    def hm_resolvedcals(self, value):
        self._hm_resolvedcals = value

    @property
    def antenna(self):
        if self._antenna is None:
            return ''
        return self._antenna

    @antenna.setter
    def antenna(self, value):
        self._antenna = value

    @property
    def uvrange(self):
        if self._uvrange is None:
            return ''
        return self._uvrange

    @uvrange.setter
    def uvrange(self, value):
        self._uvrange = value

    @property
    def peak_fraction(self):
        if self._peak_fraction is None:
            return 0.2
        return self._peak_fraction

    @peak_fraction.setter
    def peak_fraction(self, value):
        self._peak_fraction = value

        
class GcorFluxscale(basetask.StandardTaskTemplate):
    Inputs = GcorFluxscaleInputs

    def prepare(self, **parameters):
        inputs = self.inputs
        ms = inputs.ms

        # Initialize results.
        result = commonfluxresults.FluxCalibrationResults(inputs.vis,
            resantenna='', uvrange='')
        
        # Check that the measurement set does have an amplitude calibrator.
        if inputs.reference == '':
            # No point carrying on if not.
            LOG.error('%s has no data with reference intent %s'
                      '' % (ms.basename, inputs.refintent))
            return result

        # Run setjy for sources in the reference list which have transfer intents.
        if inputs.ms.get_fields(inputs.reference, intent=inputs.transintent):
            setjy_result = self._do_setjy(reffile=None, field=inputs.reference)
        else:
            LOG.info('Flux calibrator field(s) \'%s\' in %s have no data with intent %s' % 
                        (inputs.reference, inputs.ms.basename, inputs.transintent))

        refant = inputs.refant
        if refant == '':
            # Get the reference antenna for this measurement set from the 
            # context. This comes back as a string containing a ranked
            # list of antenna names. Choose the first one.
            refant = ms.reference_antenna
            # if no reference antenna was found in the context for this measurement
            # (refant equals either None or an empty string), then raise an exception.
            if not (refant and refant.strip()):
                msg = ('No reference antenna specified and none found in '
                       'context for %s' % ms.basename)
                LOG.error(msg)
                raise Exception(msg)
            refant = refant.split(',')
            refant = refant[0]
        LOG.trace('refant:%s' % refant)

	# Get the reference spwmap for flux scaling
        refspwmap = inputs.refspwmap
        if not refspwmap:
            refspwmap = ms.reference_spwmap
            if not refspwmap:
                refspwmap = [-1]

	# Get the phaseupspwmap. The result will be
	# None if it has not been set or is equivalent
	# to the default one to one  mapping.
	phaseupspwmap = ms.phaseup_spwmap
                   
        # Resolved source heuristics.
        #    Needs improvement if users start specifying the input antennas.
	#    For the time being force minblperant to be 2 instead of None to
        #    avoid ACA and Tsys flagging issues.  

        allantenna = inputs.antenna
        minblperant = 2
        hm_resolvedcals = inputs.hm_resolvedcals

        if hm_resolvedcals == 'automatic':

            # Get the antennas to be used in the gaincals, limiting
            # the range if the reference calibrator is resolved.
            resantenna, uvrange = heuristics.fluxscale.antenna(ms=ms,
              refsource=inputs.reference, refant=refant,
              peak_frac=inputs.peak_fraction)

            # Do nothing if the source is unresolved.
            # If the source is resolved but the number of
            # antennas equals the total number of antennas
            # use all the antennas but pass along the uvrange
            # limit.
            if resantenna == '' and uvrange == '':
                pass
            else:
                nant = len(ms.antennas)
                nresant = len(resantenna.split(',')) 
                if nresant >= nant:
                    resantenna = allantenna
        else:
            resantenna = allantenna
            uvrange = inputs.uvrange

        result.resantenna = resantenna
        result.uvrange = uvrange

        # Do a phase-only gaincal on the flux calibrator using a restricted
        # set of antennas
        r = self._do_gaincal(field=inputs.reference, intent=inputs.refintent,
	    gaintype='G', calmode='p', solint=inputs.phaseupsolint,
	    antenna=resantenna, uvrange=uvrange, refant=refant, minblperant=minblperant,
	    phaseupspwmap=None, append=False, merge=False)

	# Test for the existence of the caltable
	try:
            caltable = r.final.pop().gaintable
	except:
            caltable = r.error.pop().gaintable
            LOG.warn('Cannot compute phase solution table %s for the flux calibrator' % (os.path.basename(caltable)))

        # Do a phase-only gaincal on the remaining calibrators using the full
        # set of antennas
	if os.path.exists(caltable):
            r = self._do_gaincal(caltable=caltable, field=inputs.transfer,
	        intent=inputs.transintent, gaintype='G', calmode='p', 
                solint=inputs.phaseupsolint, antenna=allantenna, uvrange='', 
                minblperant=None, refant=refant, phaseupspwmap=phaseupspwmap,
	        append=True, merge=True)
	else:
            r = self._do_gaincal(caltable=caltable, field=inputs.transfer,
	        intent=inputs.transintent, gaintype='G', calmode='p', 
                solint=inputs.phaseupsolint, antenna=allantenna, uvrange='', 
                minblperant=None, refant=refant, phaseupspwmap=phaseupspwmap,
	        append=False, merge=True)

        # Now do the amplitude-only gaincal. This will produce the caltable
        # that fluxscale will analyse
	try:
            caltable = r.final.pop().gaintable
            r = self._do_gaincal(field=inputs.transfer + ',' + inputs.reference,
                intent=inputs.transintent + ',' + inputs.refintent, gaintype='T',
	        calmode='a', solint=inputs.solint, antenna=allantenna, uvrange='',
                refant=refant, minblperant=minblperant, phaseupspwmap=None,
                append=False, merge=True)

            # Get the gaincal caltable from the results
	    try:
                caltable = r.final.pop().gaintable
	    except:
                caltable = r.error.pop().gaintable
                LOG.warn('Cannot compute compute the flux scaling table %s' % (os.path.basename(caltable)))

            # To make the following fluxscale reliable the caltable
            # should contain gains for the the same set of antennas for 
            # each of the amplitude and phase calibrators - looking
            # at each spw separately.
	    if os.path.exists(caltable):
                check_ok = self._check_caltable(caltable=caltable,
                    ms=ms, reference=inputs.reference, transfer=inputs.transfer) 
	    else:
	        check_ok = False

	except:
            caltable = r.error.pop().gaintable
            LOG.warn('Cannot compute phase solution table %s for the phase and bandpass calibrator' % (os.path.basename(caltable)))
	    check_ok = False

        if check_ok:
            # Schedule a fluxscale job using this caltable. This is the result
            # that contains the flux measurements for the context.

            # We need to write the fluxscale-derived flux densities to a file,
            # which can then be used as input for the subsequent setjy task.
            # This is the name of that file.
            reffile = os.path.join(inputs.context.output_dir,
                                   'fluxscale_s%s.csv' % inputs.context.stage)

            try:
                fluxscale_result = self._do_fluxscale(caltable, 
                                                      refspwmap=refspwmap)

                importdata.importdata.export_flux_from_result(fluxscale_result,
                                                              inputs.context,
                                                              reffile)

                # and finally, do a setjy, add its setjy_settings
                # to the main result
                setjy_result = self._do_setjy(reffile=reffile, field=inputs.transfer)

                # use the fluxscale measurements to get the uncertainties too.
                # This makes the (big) assumption that setjy executed exactly
                # what we passed in as arguments
                result.measurements.update(fluxscale_result.measurements)
                
            except Exception, e:
                # Something has gone wrong, return an empty result
                LOG.error('Unable to complete flux scaling operation for MS %s' % (os.path.basename(inputs.vis)))
                #LOG.exception(e)
                return result

            finally:
                # clean up temporary file
                if os.path.exists(reffile):
                    os.remove(reffile)

        else:
            LOG.error('Unable to complete flux scaling operation for MS %s' % (os.path.basename(inputs.vis)))
            return result 

        return result

    def analyse(self, result):
        return result

    def _check_caltable(self, caltable, ms, reference, transfer):
        """Check that the give caltable is well-formed so that a 'fluxscale'
        will run successfully on it:
          1. Check that the caltable contains results for the reference and
             transfer fields.
        """
        # get the ids of the reference source and phase source(s)
        ref_fieldid = set([field.id for field in ms.fields if
          field.name==reference])
        transfer_fieldids = set([field.id for field in ms.fields if
          field.name in transfer])

        with casatools.TableReader(caltable, nomodify=False) as table:
            spwids = table.getcol('SPECTRAL_WINDOW_ID')
            spwids = set(spwids)

            fieldids = table.getcol('FIELD_ID')
            fieldids = set(fieldids)

            # check that fieldids contains the amplitude and phase calibrators
            if fieldids.isdisjoint(ref_fieldid):
                LOG.warning(
                  '%s contains ambiguous reference calibrator field names' % 
                  os.path.basename(caltable))
                #return False
                return True
            if not fieldids.issuperset(transfer_fieldids):
                LOG.warning(
                  '%s does not contain results for all transfer calibrators' %
                  os.path.basename(caltable))
                #return False
                return True

        return True
                                
    def _do_gaincal(self, caltable=None, field=None, intent=None, gaintype='G',
        calmode=None, solint=None, antenna=None, uvrange='', refant=None,
	minblperant=None, phaseupspwmap=None, append=None, merge=True):

        inputs = self.inputs

        # Get the science spws
        sci_spwids = [spw.id for spw in inputs.ms.get_spectral_windows(science_windows_only=True)]

	# Use only valid spws
	spw_ids = []
	fieldlist = inputs.ms.get_fields(task_arg=field)
	for fld in fieldlist:
	   for spw in fld.valid_spws:
               if spw.id not in sci_spwids:
                   continue
	       spw_ids.append(str(spw.id))
	spw_ids = ','.join(list(set(spw_ids)))

        task_args = {'output_dir'  : inputs.output_dir,
                     'vis'         : inputs.vis,
                     'caltable'    : caltable,
                     'field'       : field,
                     'intent'      : intent,
		     'spw'         : spw_ids,
                     'solint'      : solint,
                     'gaintype'    : gaintype,
                     'calmode'     : calmode,
                     'minsnr'      : inputs.minsnr,
                     'combine'     : '',
                     'refant'      : refant,
                     'antenna'     : antenna,
                     'uvrange'     : uvrange,
                     'minblperant' : minblperant,
                     'solnorm'     : False,
                     'append'      : append}

        # Note that field and antenna task there default values for the
        # purpose of setting up the calto object.
        task_inputs = gaincal.GTypeGaincal.Inputs(inputs.context, **task_args)
        task = gaincal.GTypeGaincal(task_inputs)

	# Execute task
        result = self._executor.execute(task)

	# Merge
	if merge:
	    # Adjust the spwmap
	    if phaseupspwmap: 
                self._mod_last_spwmap(result.pool[0], phaseupspwmap)
	        self._mod_last_spwmap(result.final[0], phaseupspwmap)
	    # Merge result to the local context
	    result.accept(inputs.context)

	return result

    def _do_fluxscale(self, caltable=None, refspwmap=None):
        inputs = self.inputs
        
        task_args = {'output_dir' : inputs.output_dir,
                     'vis'        : inputs.vis,
                     'caltable'   : caltable,
                     'reference'  : inputs.reference,
                     'transfer'   : inputs.transfer,
                     'refspwmap'  : refspwmap}
        
        task_inputs = fluxscale.Fluxscale.Inputs(inputs.context, **task_args)
        task = fluxscale.Fluxscale(task_inputs)
        
        return self._executor.execute(task, merge=True)

    def _do_setjy(self, reffile=None, field=None):
        inputs = self.inputs
        
        task_args = {'output_dir' : inputs.output_dir,
                     'vis'        : inputs.vis,
                     'field'      : field,
                     'intent'     : inputs.transintent,
                     'reffile'    : reffile}

        task_inputs = setjy.Setjy.Inputs(inputs.context, **task_args)
        task = setjy.Setjy(task_inputs)
        
        return self._executor.execute(task, merge=True)

    def _mod_last_spwmap(self, l, spwmap):
        l.calfrom[-1] = self._copy_with_spwmap(l.calfrom[-1], spwmap)

    def _copy_with_spwmap(self, old_calfrom, spwmap):
        return callibrary.CalFrom(gaintable=old_calfrom.gaintable,
                                  gainfield=old_calfrom.gainfield,
                                  interp=old_calfrom.interp,
                                  spwmap=spwmap,
                                  caltype=old_calfrom.caltype,
                                  calwt=old_calfrom.calwt)
