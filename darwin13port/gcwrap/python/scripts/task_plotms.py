import os
import time
from taskinit import *

def plotms(vis=None, plotindex=None,
           gridrows=None, gridcols=None,
           xaxis=None, xdatacolumn=None, 
           yaxis=None, ydatacolumn=None,
           selectdata=None, field=None, spw=None,
           timerange=None, uvrange=None, antenna=None, scan=None,
           correlation=None, array=None, observation=None, msselect=None,
           averagedata=None,
           avgchannel=None, avgtime=None, avgscan=None, avgfield=None,
           avgbaseline=None, avgantenna=None, avgspw=None, scalar=None,
           transform=None,
           freqframe=None,restfreq=None,veldef=None,shift=None,
           extendflag=None,
           extcorr=None, extchannel=None,
           iteraxis=None,rowindex=None,colindex=None,xselfscale=None,yselfscale=None,
           xsharedaxis=None, ysharedaxis=None,
           customsymbol=None, symbolshape=None, symbolsize=None,
           symbolcolor=None, symbolfill=None, symboloutline=None,
           coloraxis=None,
           customflaggedsymbol=None, flaggedsymbolshape=None,
           flaggedsymbolsize=None, flaggedsymbolcolor=None,
           flaggedsymbolfill=None, flaggedsymboloutline=None,
           plotrange=None,
           title=None, xlabel=None, ylabel=None,
           showmajorgrid=None, majorwidth=None, majorstyle=None,  majorcolor=None,    
           showminorgrid=None, minorwidth=None, minorstyle=None,  minorcolor=None,    
           plotfile=None, expformat=None, exprange=None,
           highres=None, overwrite=None, 
           showgui=None
):

# we'll add these later
#           extspw=None, extantenna=None,
#           exttime=None, extscans=None, extfield=None,

    """
    
            Task for plotting and interacting with visibility data.  A variety
        of axes choices (including data column) along with MS selection and
        averaging options are provided for data selection.  Flag extension
        parameters are also available for flagging operations in the plotter.
        
            All of the provided parameters can also be set using the GUI once
        the application has been launched.  Additional and more specific
        operations are available through the GUI and/or through the plotms
        tool (pm).
        

    Keyword arguments:
    vis -- input visibility dataset
           default: ''
    plotindex -- specific plot identifier in cases of multiple plots (zero-based).
            default: 0
    xaxis, yaxis -- what to plot on the two axes
                    default: '' (uses PlotMS defaults/current set).
        &gt;&gt;&gt; xaxis, yaxis expandable parameters
        xdatacolumn, 
        ydatacolumn -- which data column to use for data axes
                       default: '' (uses PlotMS default/current set).
    iteraxis -- what axis to iterate on when doing iteration plots
                default: ''
              &gt;&gt;&gt; rowindex, colindex, xsharedaxis, ysharedaxis, xselfscale, yselfscale expandable parameters
        rowindex -- the row location for this plot in the case of multiple plots per page.
                    default: 1.
        colindex -- the column location for this ploat in the case of multiple plots per page.
                    default: 1.
        xselfscale -- use a common scale for the x-axis.
                       default: False.
        yselfscale -- use a common scale for the y-axis.
                       default: False.
        xsharedaxis -- use a common x-axis for vertically aligned plots.
                        default: False.
        ysharedaxis -- use a common y-axis for horizontally aligned plots.
                        default: False.
    selectdata -- data selection parameters flag
                  (see help par.selectdata for more detailed information)
                  default: False
      &gt;&gt;&gt; selectdata expandable parameters
        field -- select using field ID(s) or field name(s)
                 default: '' (all).
        spw -- select using spectral window/channels
               default: '' (all)
        timerange -- select using time range
                     default: '' (all).
        uvrange -- select using uvrange
                   default: '' (all).
        antenna -- select using antenna/baseline
                   default: '' (all).
        scan -- select using scan number
                default: '' (all).
        correlation -- select using correlations
                       default: '' (all).
        array -- select using (sub)-array range
                 default: '' (all).
        observation -- select by observation ID(s).
                 default: '' (all).
        msselect -- TaQL selection expression
                    default: '' (all).
    
    averagedata -- data averaing parameters flag
                   default: False.
      &gt;&gt;&gt; averagedata expandable parameters
        avgchannel -- average over channel?  either blank for none, or a value
                      in channels.
                      default: '' (none).
        avgtime -- average over time?  either blank for none, or a value in
                   seconds.
                   default: '' (none).
        avgscan -- average over scans?  only valid if time averaging is turned
                   on.
                   default: False.
        avgfield -- average over fields?  only valid if time averaging is
                    turned on.
                    default: False.
        avgbaseline -- average over all baselines?  mutually exclusive with
                       avgantenna.
                       default: False.
        avgantenna -- average by per-antenna?  mutually exclusive with
                      avgbaseline.
                      default: False.
        avgspw -- average over all spectral windows?
                  default: False.
    
    extendflag -- have flagging extend to other data points?
                  default: False.
      &gt;&gt;&gt; extendflag expandable parameters
        extcorr -- extend flags based on correlation?  blank = none.
                          default: ''.
        extchannel -- extend flags based on channel?
                      default: False.
        extspw -- extend flags based on spw?
                  default: False.
        extantenna -- extend flags based on antenna?  should be either blank,
                      'all' for all baselines, or an antenna-based value.
                      default: ''.
        exttime -- extend flags based on time (within scans)?
                   default: False.
        extscans -- extend flags based on scans?  only valid if time extension
                    is turned on.
                    default: False.
        extfield -- extend flags based on field?  only valid if time extension
                    is turned on.
                    default: False.
        showgui -- Whether or not to display the plotting GUI
                  default: True; example showgui=False

    coloraxis -- which axis to use for colorizing
                     default: ''  (ignored - same as colorizing off)              
    
    title  -- title along top of plot (called "canvas" in some places)
    exprange -- whether to export all iteration plots or only the current one
    xlabel, ylabel -- text to label horiz. and vert. axes, with formatting (%% and so on)
    
    """
    # Check if DISPLAY environment variable is set.
    if os.getenv('DISPLAY') == None:
        casalog.post('ERROR: DISPLAY environment variable is not set!  Cannot open plotms.', 'SEVERE')
        return False
    
    if (plotfile and os.path.exists(plotfile) and not overwrite):
        casalog.post("Plot file " + plotfile + " exists and overwrite is false, cannot write the file", "SEVERE")
        return False

    try:            
        # Check synonyms
        # format is:  synonym['new_term'] = 'existing_term', with 
        # the existing term being what's coded in PlotMSConstants.h  (case insensitive)
        synonyms = {}
        synonyms['timeinterval'] = synonyms['timeint'] = synonyms['time_interval']='interval'
        synonyms['chan'] = 'channel'
        synonyms['freq'] = 'frequency'
        synonyms['vel'] = 'velocity'
        synonyms['correlation'] = 'corr'
        synonyms['ant1'] = 'antenna1'
        synonyms['radialvelocity']='Radial Velocity [km/s]'
        synonyms['rho']='Distance (rho) [km]'
        synonyms['weight'] = 'wt'
        synonyms['ant2'] = 'antenna2'
        synonyms['uvdistl'] = synonyms['uvdist_l']='uvwave'
        synonyms['amplitude'] = 'amp'
        synonyms['imaginary'] = 'imag'
        synonyms['ant'] = 'antenna'
        synonyms['parang'] = synonyms['parallacticangle'] = 'parangle'
        synonyms['hourang'] = 'hourangle'
        synonyms['ant-parallacticangle']=synonyms['ant-parang'] = 'ant-parangle'
        synonyms['gamp']=synonyms['gainamp']='gainamp'
        synonyms['gphase']=synonyms['gainphase']='gainphase'
        synonyms['greal']=synonyms['gainreal']='gainreal'
        synonyms['gimag']=synonyms['gainimag']='gainimag'
        synonyms['swp']=synonyms['swpower']=synonyms['switchedpower']='swpower'
        synonyms['opac']=synonyms['opacity']='opac'
        synonyms['del']=synonyms['delay']='delay'
        
        if(synonyms.has_key(xaxis)): xaxis = synonyms[xaxis]
        if(synonyms.has_key(yaxis)): yaxis = synonyms[yaxis]
        
        # synonyms for data columns (only one, so just hardcode it)
        if (xdatacolumn=='cor' or xdatacolumn=='corr'):  xdatacolumn='corrected'
        if (ydatacolumn=='cor' or ydatacolumn=='corr'):  ydatacolumn='corrected'

        #if showgui:
         #   tp.setgui( True )     ####  showgui );
        #else:
         #   tp.setgui( False )    
        vis = os.path.abspath(vis.strip())
        if not plotindex:
            plotindex = 0
    
        #Determine whether this is going to be a scripting client or a full GUI supporting
        #user interaction.  This must be done before other properties are set because it affects
        #the constructor of plotms.
        pm.setShowGui( showgui )
        if gridrows or gridcols:
            if not gridrows:
                gridrows = 1
            if not gridcols:
                gridcols = 1
            pm.setGridSize( gridrows, gridcols )
        pm.setPlotMSFilename(vis, False, plotindex )
        pm.setPlotAxes(xaxis, yaxis, xdatacolumn, ydatacolumn, False, plotindex)
        
        # Set selection
        if (selectdata and os.path.exists(vis)):
            pm.setPlotMSSelection(field, spw, timerange, uvrange, antenna, scan,
                                  correlation, array, str(observation), msselect, False, plotindex)
        else:
            pm.setPlotMSSelection('', '', '', '', '', '', '', '', '', '', False, plotindex)
       
        # Set averaging
        if not averagedata:
            avgchannel = avgtime = ''
            avgscan = avgfield = avgbaseline = avgantenna = avgspw = False
           
            scalar = False
            
        pm.setPlotMSAveraging(avgchannel, avgtime, avgscan, avgfield, avgbaseline, 
                              avgantenna, avgspw, scalar, False, plotindex)
        # Set transformations
        if not transform:
            freqframe=''
            restfreq=''
            veldef='RADIO'
            shift=[0.0,0.0]
        
        pm.setPlotMSTransformations(freqframe,veldef,restfreq,shift[0],shift[1],
                                    False, plotindex)
        
        # Set flag extension
        # for now, some options here are not available:
        # pm.setFlagExtension(extendflag, extcorrelation, extchannel, extspw, extantenna, exttime, extscans, extfield)
        extcorrstr=''
        if extcorr:
            extcorrstr='all'
        pm.setFlagExtension(extendflag, extcorrstr, extchannel)
        
        # Export range
        if (exprange == ""):
            exprange='current'
        pm.setExportRange(exprange)

        # Set stuff that informs the plot on additional axes
        #  (iteration, colorization, etc.)
        # (Iteration)
        if (iteraxis==""):
            iternx = iterny=1
            xselfscale=yselfscale=False
            xsharedaxis = ysharedaxis = False
        pm.setPlotMSIterate(iteraxis,rowindex,colindex,
                            xselfscale,yselfscale,
                            xsharedaxis,ysharedaxis,False,plotindex);
        
        # (Colorization)
        pm.setColorAxis(coloraxis,False,plotindex)

        # Set custom symbol
        symbolshape = symbolshape or 'autoscaling'
        symbolsize = symbolsize or 2
        symbolcolor = symbolcolor or '0000ff'
        symbolfill = symbolfill or 'fill'
        symboloutline = symboloutline or False
        pm.setSymbol(symbolshape, symbolsize, symbolcolor,
                     symbolfill, symboloutline, False,plotindex)
        
        # Set custom flagged symbol
        flaggedsymbolshape = flaggedsymbolshape or 'nosymbol'
        flaggedsymbolsize = flaggedsymbolsize or 2
        flaggedsymbolcolor = flaggedsymbolcolor or 'ff0000'
        flaggedsymbolfill = flaggedsymbolfill or 'fill'
        flaggedsymboloutline = flaggedsymboloutline or False
        pm.setFlaggedSymbol(flaggedsymbolshape, flaggedsymbolsize,
                            flaggedsymbolcolor, flaggedsymbolfill,
                            flaggedsymboloutline, False, plotindex)
        
        # Set various user-directed appearance parameters
        pm.setTitle(title,False,plotindex)
        pm.setXAxisLabel(xlabel,False,plotindex)
        pm.setYAxisLabel(ylabel,False,plotindex)
        pm.setGridParams(showmajorgrid, majorwidth, majorstyle, majorcolor,
                         showminorgrid, minorwidth, minorstyle, minorcolor, False, plotindex)

        if (len(plotrange)!=4):
            if (len(plotrange)==0):
                plotrange=[0.0,0.0,0.0,0.0]
            else:
                raise Exception, 'plotrange parameter has incorrect number of elements.'

        xrange=plotrange[1]-plotrange[0]
        yrange=plotrange[3]-plotrange[2]

        pm.setXRange((xrange<=0.), plotrange[0],plotrange[1], False, plotindex)
        pm.setYRange((yrange<=0.), plotrange[2],plotrange[3], False, plotindex)
        
        # Update
        plotUpdated = pm.update()
        if not plotUpdated:
            casalog.post( "There was a problem updating the plot.")
        else:
            # write file if requested
            casalog.post("Plot file " + plotfile, 'NORMAL')
            if(plotfile != ""):
                time.sleep(0.5)
                if (pm.isDrawing()):
                    casalog.post("Will wait until drawing of the plot has completed before exporting it",'NORMAL')
                    while (pm.isDrawing()):
                        time.sleep(0.5)
                casalog.post("Exporting the plot.",'NORMAL')
                casalog.post("Calling pm.save,", 'NORMAL')
                plotUpdated = pm.save( plotfile, expformat, highres)
    
    except Exception, instance:
        plotUpdated = False
        print "Exception during plotms task: ", instance
        
    return plotUpdated