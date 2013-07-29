# -*- coding: utf-8 -*-
#######################################################################3
#  task_imhead.py
#
#
# Copyright (C) 2008
# Associated Universities, Inc. Washington DC, USA.
#
# This library is free software; you can redistribute it and/or modify it
# under the terms of the GNU Library General Public License as published by
# the Free Software Foundation; either version 2 of the License, or (at your
# option) any later version.
#
# This library is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
# License for more details.
#
# You should have received a copy of the GNU Library General Public License
# along with this library; if not, write to the Free Software Foundation,
# Inc., 675 Massachusetts Ave, Cambridge, MA 02139, USA.
#
# Correspondence concerning AIPS++ should be adressed as follows:
#        Internet email: aips2-request@nrao.edu.
#        Postal address: AIPS++ Project Office
#                        National Radio Astronomy Observatory
#                        520 Edgemont Road
#                        Charlottesville, VA 22903-2475 USA
#
# <author>
# Shannon Jaeger (University of Calgary)
# </author>
#
# <summary>
# CASA task for reading/writing/listing the CASA Image header
# contents
# </summary>
#
# <reviewed reviwer="" date="" tests="" demos="">
# </reviewed
#
# <etymology>
# imhead stands for image header
# </etymology>
#
# <synopsis>
# task_imhead.py is a Python script providing an easy to use task
# for adding, removing, listing and updating the contents of a CASA
# image.  This task is very useful for fixing mistakes made in the
# importing of FITS files into CASA images, as well as seeing what
# checking the header to see what type of data is in the image file.
#
# NOTE: This task does not edit FITS files, but FITS files may
#       be created with exportuvfits task
#
# </synopsis>
#
# <example>
# <srcblock>
## The following code lists the keyword/value pairs found in
## the header of the CASA image file ngc133.clean.image.  The information
## is stored in Python variable hdr_info in a Python dictionary.
## The information is also listed in the CASA logger.  The observation
## date is #printed out from the hdr_info variable.
# hdr_info = imhead( 'ngc4826.clean.image', 'list' )
##print "Observation Date: ", hdr_info['date-obs']
#
## The following exmple displays the CASA images history in the CASA logger.
# imhead( 'ngc4826.clean.image', 'history' )
#
## The following example adds a new, user-defined keyword to the
## CASA image ngc4826.clean.image
# imhead( 'ngc4826.clean.image', 'add', 'observer 1', 'Joe Smith'  )
# imhead( 'ngc4826.clean.image', 'add', 'observer 2', 'Daniel Boulanger'  )
#
## The following example changes the name of the observer keyword,
## OBSERVE, to ALMA
# imhead( 'ngc4826.clean.image', 'put', 'telescope', 'ALMA' )
# </srblock>
# </example>
#
# <motivation>
# To provide headering modification and reading tools to the CASA users.
# </motivation>
#
# <todo>
# </todo>

import numpy
import os

from taskinit import *

not_known = ' Not Known '


# TODO Holy moly is this code screaming to be refactored.


def imhead(
    imagename, mode, hdkey, hdvalue,
    hdtype, hdcomment, verbose
):
    # Some debugging info.
    casalog.origin('imhead')
    casalog.post('parameter imagename: ' + imagename, 'DEBUG1')
    casalog.post('parameter mode:      ' + mode, 'DEBUG1')
    casalog.post('parameter hdkey:     ' + hdkey, 'DEBUG1')
    casalog.post('parameter hdvalue:   ' + str(hdvalue), 'DEBUG1')
    casalog.post('parameter hdtype:    ' + hdtype, 'DEBUG1')
    casalog.post('parameter hdcomment: ' + hdcomment, 'DEBUG1')

    # Initialization stuff, If we are here the user has
    # specified an imagename and mode and we don't need to do
    # checks on them.  The CASA task infrustructure will
    # have done them.  But to make the script insensitive we'll
    # make everything lower case.
    mode = mode.lower()
    hdkey = hdkey.lower()
    # if ( isinstance( hdvalue, str ) ):
    # hdvalue   = hdvalue.lower()
    hdtype = hdtype.lower()
    hdcomment = hdcomment.lower()
    # ############################################################
    #                 HISTORY Mode
    # ############################################################
    # History mode, List the history information for the
    # CASA image file.
    myia = iatool()
    try:
        if mode.startswith('his'):
            myia.open(imagename)
            myia.history()
            myia.done()
            return True
    except Exception, instance:
        casalog.post(str('*** Error ***') + str(instance), 'SEVERE')
        return False

    # ############################################################
    #                 Summary Mode
    # ############################################################
    # Summary mode, likely to become obsolete.  When
    # Image Analysis summary output looks like the task
    # output
    try:
        if mode.startswith('sum'):
            myia.open(imagename)
            myia.summary(verbose=verbose)
            myia.done()
            return True
    except Exception, instance:
        casalog.post(str('*** Error ***') + str(instance), 'SEVERE')
        return False
    # ############################################################
    # Some variables used to keep track of header iformation
    # as follows:
    #    hd_keys:     Full list of header keys
    #    hd_values:   The values associated with the keywords
    #    hd_type:
    #    hd_comment:
    #
    #    axes:        List of lists with axis information.
    #                 Order is dir1, dir2, spec, stokes
    #
    # NOTE: There is extra work done here for put, add, get,
    #       and del modes.  As we read the full header into
    #       local variables.  However, it makes the code
    #       cleaner to read.
    #
    # ############################################################
    axes = []
    try:
        axes = getimaxes(imagename)
    except Exception, instance:
        casalog.post(str('*** Error ***') + str(instance), 'SEVERE')
        return False

    tbkeys = ['imtype', 'object', 'equinox']

    csyskeys = [
        'date-obs',
        'equinox',
        'observer',
        'projection',
        'restfreq',
        'reffreqtype',
        'telescope'
        ]
    imkeys = [
        'beammajor',
        'beamminor',
        'beampa',
        'bmaj',
        'bmin',
        'bpa',
        'bunit',
        'masks',
        'shape'
        ]
    crdkeys = ['ctype', 'crpix', 'crval', 'cdelt', 'cunit']
    statkeys = [
        'datamin',
        'datamax',
        'minpos',
        'minpixpos',
        'maxpos',
        'maxpixpos'
        ]

    hd_keys = tbkeys + csyskeys + imkeys + statkeys
    for key in crdkeys:
        for axis in range(len(axes)):
            hd_keys.append(key + str(axis + 1))

    hd_values = {}
    hd_types = {}
    hd_units = {}
    hd_comments = {}

    # For each header keyword initialize the dictionaries
    # as follows:
    #   value     "Not Known"
    #   type:     "Not Known"
    #   unit:     ""
    #   comment:  ""
    for hd_field in hd_keys:
        hd_values[hd_field] = not_known
        hd_types[hd_field] = not_known
        hd_units[hd_field] = ''
        hd_comments[hd_field] = ''

    # ############################################################
    #            Read the header contents.
    #
    # Needed for get and list mode only.
    #
    # TODO or consider.  Some of the coordsys() functions take
    # a format type.  It might save us a lot of time and trouble
    # if we retrieve the information as a string.  For example
    #   csys.refernecevalue( format='s')
    # ############################################################

    # # Read the information from the TABLE keywords
    mykeywords = {}
    try:
        tb.open(imagename)
        mykeywords = tb.getkeywords()
        tb.close()
    except:
        casalog.post(str('*** Error *** Unable to open image file ')
                     + imagename, 'SEVERE')
        return False

    # Now update our header dictionary.
    if mykeywords.has_key('imageinfo') and mykeywords['imageinfo'
            ].has_key('objectname'):
        hd_values['object'] = mykeywords['imageinfo']['objectname']
        hd_types['object'] = 'string'
        hd_units['object'] = ''
        hd_comments['object'] = ''

    if mykeywords.has_key('imageinfo') and mykeywords['imageinfo'
            ].has_key('imagetype'):
        hd_values['imtype'] = mykeywords['imageinfo']['imagetype']
        hd_types['imtype'] = 'string'
        hd_units['imtype'] = ''
        hd_comments['imtype'] = ''

    # Use ia.summary to gather some of the header information
    #     ia.stats for the min and max
    #     ia.coordsys() to get a coordsys object to retrieve
    #     the coordinate information.
    #     Also find the units the data is stored in, for storing
    hd_dict = {}
    stats = {}
    csys = None
    misc_info = {}
    data_unit = ''
    try:
        myia.open(imagename)
        csys = myia.coordsys()
        myia.done()
    except Exception, instance:
        casalog.post(str('*** Error *** Unable to get coordinate syatem info ')
                     + imagename, 'SEVERE')
        raise instance
    # Find some of the general information from COORD SYS object:
    #     OBSERVER, TELESCOPE, RESTFREQUENCY, PROJECTION, EPOCH, and
    #     EQUINOX
    try:
        hd_values['observer'] = csys.observer()
        hd_types['observer'] = 'string'
        hd_units['observer'] = ''
        hd_comments['observer'] = ''
    except:
        no_op = 'noop'
    try:
        hd_values['telescope'] = csys.telescope()
        hd_types['telescope'] = 'string'
        hd_units['telesceope'] = ''
        hd_comments['telescope'] = ''
    except:
        no_op = 'noop'

    try:
        tmp = csys.restfrequency()
        if tmp.has_key('value'):
            hd_values['restfreq'] = list(tmp['value'])
        hd_types['restfreq'] = 'list'
        if tmp.has_key('unit'):
            hd_units['restfreq'] = tmp['unit']
        hd_comments['restfreq'] = ''
    except:
        no_op = 'noop'

    try:
        # Expected value is a dictionary with two keys:
        #    parameters: the parameters to the projection
        #    type:       Typeof projection (value we want)
        tmp = csys.projection()
        if tmp.has_key('type'):
            hd_values['projection'] = tmp['type']
        hd_types['projection'] = 'string'
        hd_units['projection'] = ''
        if tmp.has_key('parameters'):
            hd_comments['projection'] = str(list(tmp['parameters']))
    except:
        no_op = 'noop'
    try:
        hd_values['equinox'] = csys.referencecode(type='direction')[0]
        hd_types['equinox'] = 'string'
        hd_units['equinox'] = ''
        hd_comments['equinox'] = ''
    except:
        no_op = 'noop'
    try:
        hd_values['reffreqtype'] = csys.referencecode(type='spectral')
        hd_types['reffreqtype'] = 'string'
        hd_units['reffreqtype'] = ''
        hd_comments['reffreqtype'] = ''
    except:
        no_op = 'noop'
    try:
        tmp = csys.epoch()
        if tmp.has_key('m0') and tmp['m0'].has_key('value') and tmp['m0'
                ]['value'] > 0:
            hd_values['date-obs'] = qa.time(tmp['m0'], form='ymd')[0]
            hd_units['data-obs'] = 'ymd'
        hd_types['date-obs'] = 'string'
        hd_comments['date-obs'] = ''
    except:
        no_op = 'noop'
    try:
        myia.open(imagename)
        hd_dict = myia.summary(list=False)
        stats = myia.statistics(verbose=False, list=False)
        misc_info = myia.miscinfo()
        data_unit = myia.brightnessunit()
        myia.done()
    except Exception, instance:
        casalog.post(str('*** Error *** Unable to open image file ')
                     + imagename, 'SEVERE')
        casalog.post(str('              Python error: ')
                     + str(instance), 'SEVERE')
        return False
    # Add the Statistical information as follows:
    #    DATAMIN, DATAMAX, MINPIX, MINPIXPOS, MAXPOS, MAXPIXPOS
    # Store the MIN and MAX values.
    if stats.has_key('min'):
        hd_values['datamin'] = stats['min'][0]
        hd_types['datamin'] = 'double'
        hd_units['datamin'] = data_unit
        hd_comments['datamin'] = ''
    if stats.has_key('minposf'):
        hd_values['minpos'] = stats['minposf']
        hd_types['minpos'] = 'list'
        hd_units['minpos'] = ''
        hd_comments['minpos'] = ''
    if stats.has_key('minpos'):
        hd_values['minpixpos'] = stats['minpos']
        hd_types['minpixpos'] = 'list'
        hd_units['minpixpos'] = 'pixels'
        hd_comments['minpixpos'] = ''
    if stats.has_key('max'):
        hd_values['datamax'] = stats['max'][0]
        hd_types['datamax'] = 'double'
        hd_units['datamax'] = data_unit
        hd_comments['datamax'] = ''
    if stats.has_key('maxposf'):
        hd_values['maxpos'] = stats['maxposf']
        hd_types['maxpos'] = 'list'
        hd_units['maxpos'] = ''
        hd_comments['maxpos'] = ''
    if stats.has_key('maxpos'):
        hd_values['maxpixpos'] = stats['maxpos']
        hd_types['maxpixpos'] = 'list'
        hd_units['maxpixpos'] = 'pixels'
        hd_comments['maxpixpos'] = ''

    # Set some more of the standard header keys from the
    # header dictionary abtained from ia.summary().  The
    # fiels that will be set are:
    #     BEAMMAJOR, BEAMMINOR, BEAMPA, BUNIT, MASKS, CRVALx, CRPIXx,
    #     CDETLx,  CTYPEx, CUNITx
    if hd_dict.has_key('unit'):
        hd_values['bunit'] = hd_dict['unit']
        hd_types['bunit'] = 'string'
        hd_units['bunit'] = ''
        hd_comments['bunit'] = ''

    if hd_dict.has_key('restoringbeam'):
        tmp = hd_dict['restoringbeam']
        if tmp.has_key('major'):
            tmp2 = tmp['major']
            if tmp2.has_key('value'):
                hd_values['beammajor'] = tmp2['value']
            if tmp2.has_key('unit'):
                hd_units['beammajor'] = str(tmp2['unit'])
            hd_types['beammajor'] = 'double'
            hd_comments['beammajor'] = ''

        if tmp.has_key('minor'):
            tmp2 = tmp['minor']
            if tmp2.has_key('value'):
                hd_values['beamminor'] = tmp2['value']
            if tmp2.has_key('unit'):
                hd_units['beamminor'] = str(tmp2['unit'])
            hd_types['beamminor'] = 'double'
            hd_comments['beamminor'] = ''

        if tmp.has_key('positionangle'):
            tmp2 = tmp['positionangle']
            if tmp2.has_key('value'):
                hd_values['beampa'] = tmp2['value']
            if tmp2.has_key('unit'):
                hd_units['beampa'] = str(tmp2['unit'])
            hd_types['beampa'] = 'double'
            hd_comments['beampa'] = ''

    if hd_dict.has_key('masks') and len(hd_dict['masks']) > 0:
        hd_values['masks'] = hd_dict['masks'][0]
        hd_types['masks'] = 'string'
        hd_units['masks'] = ''
        hd_comments['masks'] = ''

    if hd_dict.has_key('shape'):
        hd_values['shape'] = hd_dict['shape']
        hd_types['shape'] = 'list'

    # The COORDINATE keywords
    stokes = 'Not Known'
    if isinstance(axes[3][0], int):
        stokes = csys.stokes()
    hd_coordtypes = []
    for i in range(hd_dict['ndim']):
        hd_values['ctype' + str(i + 1)] = hd_dict['axisnames'][i]
        hd_values['crpix' + str(i + 1)] = hd_dict['refpix'][i]
        if hd_dict['axisnames'][i].lower() == 'stokes':
            hd_values['crval' + str(i + 1)] = stokes
            hd_units['crval' + str(i + 1)] = hd_dict['axisunits'][i]
        else:
            hd_values['crval' + str(i + 1)] = hd_dict['refval'][i]
            hd_units['crval' + str(i + 1)] = hd_dict['axisunits'][i]
            hd_types['crval' + str(i + 1)] = 'float'

        hd_values['cdelt' + str(i + 1)] = hd_dict['incr'][i]
        hd_units['cdelt' + str(i + 1)] = hd_dict['axisunits'][i]
        hd_types['cdelt' + str(i + 1)] = 'float'
        hd_values['cunit' + str(i + 1)] = hd_dict['axisunits'][i]

    # Add the miscellaneous info/keywords
    # TODO add some some smarts to figure out the type
    #      of the keyword, and maybe unit.
    for new_key in misc_info.keys():
        hd_values[new_key] = misc_info[new_key]
        hd_types[new_key] = ''
        hd_units[new_key] = ''
        hd_comments[new_key] = ''

    # Find all of the *user defined* keywords, Python sets
    # support differences but lists don't this is why we use
    # sets here.
    user_keys = list(set(hd_values.keys()) - set(hd_keys))
    casalog.post('List of user defined keywords found are: '
                 + str(user_keys), 'DEBUG2')
    casalog.post(str(hd_values), 'DEBUG2')
    casalog.post(str(hd_types), 'DEBUG2')
    casalog.post(str(hd_units), 'DEBUG2')
    casalog.post(str(hd_comments), 'DEBUG2')

    # ############################################################
    #                     List MODE
    #
    # Just #print out all the information we just gathered.
    # ############################################################
    if mode == 'list':
        myimd = imdtool()
        try:
            myimd.open(imagename)
            return myimd.list(True)
        except Exception, instance:
            casalog.post(str('*** Error ***') + str(instance), 'SEVERE')
            casalog.post(str('              Python error: ')
                         + str(instance), 'SEVERE')
            raise
        finally:
            myimd.done()

    # ############################################################
    #                     add MODE
    #
    # Add a new keyword to the image table.
    #
    # ############################################################
    key_list = hd_keys + user_keys
    casalog.post('All of the header keys: ' + str(key_list), 'DEBUG2')

    
    if mode == "add":
        myimd = imdtool()
        try:
            myimd.open(imagename)
            return myimd.add(hdkey, hdvalue)
        except:
            casalog.post(str('*** Error *** ') + str(instance), 'SEVERE')
            raise
        finally:
            myimd.done()

    if mode == 'del':      
        myimd = imdtool()
        try:
            myimd.open(imagename)
            return myimd.remove(hdkey, hdvalue)
        except:
            casalog.post(str('*** Error *** ') + str(instance), 'SEVERE')
            raise
        finally:
            myimd.done()
    
    if mode == "get":
        myimd = imdtool()
        try:
            myimd.open(imagename)
            return myimd.get(hdkey)
        except:
            casalog.post(str('*** Error *** ') + str(instance), 'SEVERE')
            raise
        finally:
            myimd.done()
            
    # ############################################################
    #                     put MODE
    #
    # If we made it here the user is putting something into
    # the header.
    # ############################################################
    if mode != 'put':
        return False
    casalog.post('Putting (changing): ' + hdkey + ' to  '
                 + str(hdvalue), 'DEBUG2')
    if hdkey in tbkeys:
        try:
            tb.open(imagename, nomodify=False)
            if hdkey == 'equinox':
                imagecoords = tb.getkeyword('coords')
                imagecoords['direction0']['system'] = hdvalue
                tb.putkeyword(keyword='coords', value=imagecoords)
                casalog.post('Changing only the value of ' + hdkey
                             + '. It is your responsibility to ensure the coordinate '

                             + 'values are corret; this task does not transform the '

                             + 'coordinates. If you want something that does, see eg imregrid'
                             , 'WARN')
            else:
                if hdkey == 'object':
                    mykeywords['imageinfo']['objectname'] = hdvalue
                elif hdkey == 'imtype':
                    mykeywords['imageinfo']['imagetype'] = hdvalue
                tb.putcolkeywords(columnname="", value=mykeywords)
            tb.flush()
            tb.done()
            casalog.post(hdkey + ' keyword has been UPDATED in '
                         + imagename + "'s header", 'NORMAL')
            return True
        except Exception, instance:
            casalog.post('*** Error *** Unable to update keyword ' + hdkey
                         + ' from image file ' + imagename + "\n" + str(instance), 'SEVERE')
            return False
    elif hdkey in statkeys:
        # This is a statistical value, these are generated by
        # ia.statistics and not actually in the header.
        casalog.post(hdkey
                     + str(' is not part of the header information, but generated by the image\nstatistics function. Therefore, '
                      + hdkey + ' can not be changed.'), 'WARN')
        return hdvalue
    elif hdkey in imkeys:

        # Header values that can be changed through the image analysis tool.
        try:
            # These are field that can be set with the image
            # analysis tool
            myia.open(imagename)
            if hdkey == 'bunit':
                myia.setbrightnessunit(hdvalue)
            elif hdkey == 'masks':

                # We only set the default mask to the first mask in
                # the list.
                # TODO delete any masks that aren't in the list.
                if type(hdvalue, list):
                    myia.maskhanderler(op='set', name=hdvalue[0])
                else:
                    myia.maskhanderler(op='set', name=hdvalue)
            elif hdkey.startswith('beam'):

                # Get orignal values.  Note that since there are
                # expected header fields we assume that they exist
                # in our dictionary of header values.
                #
                # TODO IF NOT A LIST BUT A STRING CHECK FOR
                # UNITS
               

                major = {'unit': 'arcsec', 'value': 1}
                if str(hd_values['beammajor']) != not_known:
                    major = {'value': hd_values['beammajor'],
                             'unit': hd_units['beammajor']}

                minor = {'unit': 'arcsec', 'value': 1}
                if str(hd_values['beamminor']) != not_known:
                    minor = {'value': hd_values['beamminor'],
                             'unit': hd_units['beamminor']}

                pa = {'unit': 'deg', 'value': 0}
                if str(hd_values['beampa']) != not_known:
                    pa = {'value': hd_values['beampa'],
                          'unit': hd_units['beampa']}
                if hdkey == 'beammajor':
                    major = _imhead_strip_units(hdvalue,
                            hd_values['beammajor'], hd_units['beammajor'
                            ])
                    major['value'] = float(major['value'])
                elif hdkey == 'beamminor':
                    minor = _imhead_strip_units(hdvalue,
                            hd_values['beamminor'], hd_units['beamminor'
                            ])
                    minor['value'] = float(minor['value'])
                elif hdkey == 'beampa':
                    pa = _imhead_strip_units(hdvalue, hd_values['beampa'
                            ], hd_units['beampa'])
                    pa['value'] = float(pa['value'])
                else:
                    casalog.post('*** Error *** Unrecognized beam keyword '
                                  + hdkey, 'SEVERE')
                    myia.done()
                    return False
                myia.setrestoringbeam(beam={'major': major, 'minor'
                                    : minor, 'positionangle': pa},
                                    log=True)
            elif hdkey == 'shape':

            # CAS-3301
                casalog.post('*** Error *** imhead does not support changing the shape of the image body. Use the ia tool instead'
                             , 'SEVERE')
                myia.done()
                return False

            myia.done()
            casalog.post(hdkey + ' keyword has been UPDATED to '
                         + imagename + "'s header", 'NORMAL')
            return hdvalue
        except Exception, instance:
            casalog.post('*** Error *** Unable to update keyword '
                         + hdkey + ' from image file ' + imagename
                         + '\n' + str(instance), 'SEVERE')
            return False
    elif hdkey in csyskeys:

        # TODO I'm not sure why those who came before decided to use the coordsys tool for this
        # because its very dangerous as other values can also be changed without the user
        # being aware. For example, I'm moving 'equinox' out of this block because changing
        # the epoch using the coordsys tool also changes the positional (crval) values, which
        # is definitely not desired. - dmehring 2009sep01.
        #
        # Header values that can be changed through the coordsys tool
        try:
            myia.open(imagename)
            csys = myia.coordsys()
            if hdkey == 'date-obs':
                if hdvalue == 'Not Known' or hdvalue == 'UNKNOWN':
                    hdvalue = 0
                tmp = me.epoch(v0=str(hdvalue))
                csys.setepoch(tmp)
            elif hdkey == 'observer':
                csys.setobserver(str(hdvalue))
            elif hdkey == 'projection':
                csys.setprojection(hdvalue)
            elif hdkey == 'telescope':
                csys.settelescope(str(hdvalue))
            elif hdkey == 'reffreqtype':
                csys.setconversiontype(spectral=str(hdvalue))
            elif hdkey == 'restfreq':
                # TODO handle a list of rest frequencies

                if isinstance(hdvalue, list):
                    no_op = 'noop'  # Nothing to change here
                elif not isinstance(hdvalue, str):
                    hdvalue = str(hdvalue)
                else:
                    hdvalue = hdvalue.split(',')
                # Loop through the list of values, adding each
                # one separately.

                if isinstance(hdvalue, str):
                    num_freq = 1
                else:
                    num_freq = len(hdvalue)

                for i in range(num_freq):
                    if not isinstance(hdvalue[i], str):
                        current = str(hdvalue[i])
                    else:
                        current = hdvalue[i]
                    # Remove any units from the string, if
                    # there are any.
                    parsed_input = _imhead_strip_units(current, 0.0,
                            hd_units[hdkey])
                    parsed_input['value'] = float(parsed_input['value'])

                    if i < 1:
                        csys.setrestfrequency(parsed_input)
                    else:
                        csys.setrestfrequency(parsed_input, append=True)
            else:
                casalog.post(str('*** Error *** Unrecognized hdkey ')
                             + str(hdkey), 'SEVERE')
                return

            # Now store the values!
            myia.setcoordsys(csys=csys.torecord())
            myia.done()
            csys.done()
            del csys
            casalog.post(hdkey + ' keyword has been UPDATED to '
                         + imagename + "'s header", 'NORMAL')
            return hdvalue
        except Exception, instance:
            if myia.isopen():
                myia.done()
            if csys != None:
                csys.done()
                del csys
            casalog.post('*** Error *** Unable to UPDATE keyword '
                         + hdkey + ' in image file ' + imagename + "\n" + str(instance),
                         'SEVERE')
            casalog.post(str('              Python error: ')
                         + str(instance), 'SEVERE')
            return False
    elif hdkey[0:5] in crdkeys:

        # Coordinate axes information, changed through the coordsys tool
        try:
            # Open the file and obtain a coordsys tool
            myia.open(imagename)
            csys = myia.coordsys()

            # Find which axis is being modified from the field name
            # (hdkey). Note, that internally we use 0-based
            # indexes but the user input will be 1-based, we
            # convert by subtracting 1. Also note that all of our
            # fields are 5 character long.
            fieldRoot = hdkey[0:5]
            index = -1
            if len(hdkey) > 5:
                index = int(hdkey[5:]) - 1

            # cdelt, crpix, and cval all return a record, the values
            # are store in and *array* in the 'numeric' field.  But
            # we need to input a list, this makes things a bit messy!
            if hdkey.startswith('cdelt'):
                # We need
                if index < 0:
                    csys.setincrement(hdvalue)
                else:
                    # We grab the reference values as quantities,
                    # which allows users to specify a wider variety
                    # of units.  We are going to make an assumption
                    # that if there are no units at the end that the
                    # user gave the value in radians, but checking if
                    # the last character of the string is a number.
                    values = csys.increment(format='q')
                    units = csys.units()

                    if isinstance(hdvalue, str):
                        hdvalue = hdvalue.strip()

                        # Remove any '.' and ':' from the string
                        # if all that remains is a number then the
                        # user didn't give any units.
                        tmpVal = hdvalue.replace('.', '')
                        tmpVal = tmpVal.replace(':', '')
                        if tmpVal.isdigit():
                            if len(units[index]) > 0:
                                hdvalue = hdvalue + units[index]
                    elif isinstance(hdvalue, int) \
                        or isinstance(hdvalue, float):

                        if len(units[index]) > 0:
                            hdvalue = qa.convert(str(hdvalue)
                                    + units[index], units[index])
                        else:
                            hdvalue = str(hdvalue)

                    # reference axies are 1-based, but python
                    # lists are 0-based -- yikes!
                    values['quantity']['*' + str(index + 1)] = \
                        qa.convert(hdvalue, units[index])
                    csys.setincrement(values)
            elif hdkey.startswith('crpix'):

                if index < 0:
                    csys.setreferencepixel(hdvalue)
                else:
                    values = csys.referencepixel()['numeric']
                    values = [values[0], values[1], values[2],
                              values[3]]
                    values[index] = float(hdvalue)
                    csys.setreferencepixel(values)
            elif hdkey.startswith('crval'):
                # Because setreferencevalue has issues
                # with setting the stokes value, if
                # the stokes axis has changed we use the
                # setstokes function instead.
                if isinstance(axes[3][0], int):
                    stokesIndex = axes[3][0]
                    origvalue = csys.referencevalue('s')['string'
                            ][stokesIndex]
                else:
                    origvalue = 'Not Known'

                if index < 0:
                    newvalue = hdvalue[stokesIndex]
                    if origvalue != newvalue:
                        hdvalue[stokesIndex] = origvalue
                        newvalue = hdvalue[stokesIndex]
                        csys.setreferencevalue(hdvalue)
                        csys.setstokes(newvalue)
                else:
                    # We grab the reference values as quantities,
                    # which allows users to specify a wider variety
                    # of units.  We are going to make an assumption
                    # that if there are no units at the end that the
                    # user gave the value in radians, but checking if
                    # the last character of the string is a number.
                    values = csys.referencevalue(format='q')
                    units = csys.units()
                    if isinstance(hdvalue, str):
                        hdvalue = hdvalue.strip()

                        # Remove any '.' and ':' from the string
                        # if all that remains is a number then the
                        # user didn't give any units.
                        tmpVal = hdvalue.replace('.', '')
                        tmpVal = tmpVal.replace(':', '')
                        if tmpVal.isdigit():
                            if index < len(units) and len(units[index]) \
                                > 0:
                                if (hdvalue.count(":") > 1 or hdvalue.count(".") > 2):
                                    # hh:mm:ss or dd.mm.ss input format here
                                    hdvalue = qa.tos(qa.toangle(hdvalue))
                                else:
                                    # input is a floating point number
                                    hdvalue = hdvalue + units[index]
                            else:
                                hdvalue = hdvalue
                    elif isinstance(hdvalue, int) \
                        or isinstance(hdvalue, float):
                        if index < len(units) and len(units[index]) > 0:
                            hdvalue = qa.convert(str(hdvalue)
                                    + units[index], units[index])
                        else:
                            hdvalue = str(hdvalue)

                    # reference axies are 1-based, but python
                    # lists are 0-based -- yikes!
                    #
                    # We also need to deal with stokes changes
                    # very carefully, and use csys.setstokes()
                    #
                    # TODO We need to deal with adding stokes
                    # directional, and spectral values with
                    # the appropriate csys.set?? methods.
                    if index != axes[3][0]:
                        if index < len(units) and len(units[index]) > 0:
                            values['quantity']['*' + str(index + 1)] = \
                                qa.convert(hdvalue, units[index])
                        else:
                            values['quantity']['*' + str(index + 1)] = \
                                hdvalue
                        csys.setreferencevalue(values)
                    else:
                        origStokes = csys.referencevalue('s')
                        origStokes = origStokes['string'][index]
                        if origStokes != hdvalue:
                            csys.setstokes(hdvalue)
            elif hdkey.startswith('ctype'):

                if index < 0:
                    csys.setnames(hdvalue)
                else:
                    values = csys.names()
                    values[index] = str(hdvalue)
                    csys.setnames(values)
            elif hdkey.startswith('cunit'):

                if index < 0:
                    csys.setunits(hdvalue)
                else:
                    values = csys.units()
                    values[index] = str(hdvalue)
                    csys.setunits(values)

            # Store the changed values.
            if csys != None:
                myia.setcoordsys(csys=csys.torecord())
                csys.done()
                del csys

            myia.done()
            casalog.post(hdkey + ' keyword has been UPDATED in '
                         + imagename + "'s header", 'NORMAL')
            return hdvalue
        except Exception, instance:
            casalog.post(str('*** Error ***') + str(instance), 'SEVERE')
            return False
    else:

        # User defined keywords, changed with the image analsys too's
        # "miscinfo" function.
        # TODO Add units and comments
        try:
            # First step is to make sure we have the given
            # value in the expected data type, if there is
            # one.  The default is the string stored in hd_types
            # if there isn't a value then str is used.
            value = hdvalue
            keytype = 'str'
            if type(hdtype) != None and len(hdtype) > 0:
                keytype = hdtype
            elif hd_types.has_key(hdkey) and len(hd_types[hdkey]) > 0:
                keytype = hd_types[hdkey]
            elif isinstance(hdvalue, str):
                keytype = 'str'
            elif isinstance(hdvalue, int):
                keytype = 'int'
            elif isinstance(hdvalue, float):
                keytype = 'float'
            elif isinstance(hdvalue, complex):
                keytype = 'complex'
            elif isinstance(hdvalue, list):
                keytype = 'list'
            elif isinstance(hdvalue, dict):
                keytype = 'dict'

            if keytype == 'double':
                keytype = 'float'
            if keytype == 'string':
                keytype = 'str'

            # TODO -- Add a check to see if we really need
            #        to do the type conversion.
            if isinstance(value, str):
                value = eval(keytype + '("' + value + '")')
            else:
                value = eval(keytype + '("' + str(value) + '")')

            misc_info[hdkey] = value
            myia.open(imagename)
            myia.setmiscinfo(misc_info)
            myia.done()
            casalog.post(hdkey + ' keyword has been ADDED to '
                         + imagename + "'s header with value "
                         + str(value), 'NORMAL')
            return True
        except Exception, instance:
            casalog.post('*** Error *** Unable to change keyword '
                         + hdkey + ' in ' + imagename + ' to '
                         + str(hdvalue), 'SEVERE')
            casalog.post(str('              Python error: ')
                         + str(instance), 'SEVERE')
            return False


#
# NAME:        _imhead_strip_units
#
# AUTHOR:      S. Jaeger
#
# PURPOSE:     To take as input a string which contains a numeric value
#              followed by a unit and separate them.
#
# DESCRIPTION: Take the input string, find the the index of the
#              last numerical character in the string.  We assume
#              this is the point where the number ends and the unit
#              begins and split the input string at this place.
#
#              If there is no value found, then the default value
#              is used.  Similarly for the units.
#
# RETURN:      dictionary of the form:
#                  { 'value': numberFound, 'unit', 'unitfound' }


def _imhead_strip_units(input_number, default_value=0.0, default_unit=''
                        ):
    # Find the place where the units start and the number
    # ends.

    if isinstance(input_number, dict):
        if input_number.has_key('value'):
            value = input_number['value']
        else:
            value = default_value

        if input_number.has_key('unit'):
            unit = input_number['unit']
        else:
            unit = default_unit
    elif isinstance(input_number, str):
        lastNumber = -1
        for j in range(len(input_number)):
            if input_number[j].isdigit():
                lastNumber = j

        if lastNumber >= len(input_number) - 1:
            unit = default_unit
        else:
            unit = input_number[lastNumber + 1:]

        if lastNumber < 0:
            value = default_value
        else:
            value = input_number[0:lastNumber + 1]
    else:
        raise Exception, \
            'Unable to parse units from numerical value in input ' \
            + str(input_number)

    return {'value': value, 'unit': unit}



