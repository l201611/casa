import os
import matplotlib

######################################
##    Add CASA custom toolbar       ##
######################################
class CustomToolbarCommon:
    def __init__(self,parent):
        self.plotter=parent
        #self.figmgr=self.plotter._plotter.figmgr

    ### select the nearest spectrum in pick radius
    ###    and display spectral value on the toolbar.
    def _select_spectrum(self,event):
        # Do not fire event when in zooming/panning mode
        mode = self.figmgr.toolbar.mode
        if not mode == '':
            return
            # When selected point is out of panels
        if event.inaxes == None:
            return
        # If not left button
        if event.button != 1:
            return

        xclick=event.xdata
        yclick=event.ydata
        dist2=1000.
        pickline=None
        # If the pannel has picable objects
        pflag=False
        for lin in event.inaxes.lines:
            if not lin.pickable():
                continue
            pflag=True
            flag,pind = lin.contains(event)
            if not flag:
                continue
            # Get nearest point
            inds = pind['ind']
            xlin = lin.get_xdata()
            ylin = lin.get_ydata()
            for i in inds:
                d2=(xlin[i]-xclick)**2+(ylin[i]-yclick)**2
                if dist2 >= d2:
                    dist2 = d2
                    pickline = lin
        # No pickcable line in the pannel
        if not pflag:
            return
        # Pickable but too far from mouse position
        elif pickline is None:
            picked='No line selected.'
            self.figmgr.toolbar.set_message(picked)
            return
        del pind, inds, xlin, ylin
        # Spectra are Picked
        theplot = self.plotter._plotter
        thetoolbar = self.figmgr.toolbar
        thecanvas = self.figmgr.canvas
        # Disconnect the default motion notify event
        # Notice! the other buttons are also diabled!!!
        thecanvas.mpl_disconnect(thetoolbar._idDrag)
        # Get picked spectrum
        xdata = pickline.get_xdata()
        ydata = pickline.get_ydata()
        titl=pickline.get_label()
        titp=event.inaxes.title.get_text()
        panel0=event.inaxes
        picked="Selected: '"+titl+"' in panel '"+titp+"'."
        thetoolbar.set_message(picked)
        # Generate a navigation window
        #naviwin=Navigationwindow(titp,titl)
        #------------------------------------------------------#
        # Show spectrum data at mouse position
        def spec_data(event):
            # Getting spectrum data of neiboring point
            xclick=event.xdata
            if event.inaxes != panel0:
                return
            ipoint=len(xdata)-1
            for i in range(len(xdata)-1):
                xl=xclick-xdata[i]
                xr=xclick-xdata[i+1]
                if xl*xr <= 0.:
                    ipoint = i
                    break
            # Output spectral value on the navigation window
            posi='[ %s, %s ]:  x = %.2f   value = %.2f'\
                  %(titl,titp,xdata[ipoint],ydata[ipoint])
            #naviwin.posi.set(posi)
            thetoolbar.set_message(posi)
        #------------------------------------------------------#
        # Disconnect from mouse events
        def discon(event):
            #naviwin.window.destroy()
            theplot.register('motion_notify',None)
            # Re-activate the default motion_notify_event
            thetoolbar._idDrag=thecanvas.mpl_connect('motion_notify_event',
                                                     thetoolbar.mouse_move)
            theplot.register('button_release',None)
            return
        #------------------------------------------------------#
        # Show data value along with mouse movement
        theplot.register('motion_notify',spec_data)
        # Finish events when mouse button is released
        theplot.register('button_release',discon)


    ### Calculate statistics of the selected area.
    def _single_mask(self,event):
        # Do not fire event when in zooming/panning mode
        if not self.figmgr.toolbar.mode == '':
            return
        # When selected point is out of panels
        if event.inaxes == None:
            return
        if event.button ==1:
            baseinv=True
        elif event.button == 3:
            baseinv=False
        else:
            return

        def _calc_stats():
            msk=mymask.get_mask()
            mymask.scan.stats(stat='max',mask=msk)
            mymask.scan.stats(stat='min',mask=msk)
            mymask.scan.stats(stat='sum',mask=msk)
            mymask.scan.stats(stat='mean',mask=msk)
            mymask.scan.stats(stat='median',mask=msk)
            mymask.scan.stats(stat='rms',mask=msk)
            mymask.scan.stats(stat='stddev',mask=msk)

        # Interactive mask definition
        from asap.interactivemask import interactivemask
        mymask=interactivemask(plotter=self.plotter,scan=self.plotter._data)
        # Create initial mask
        mymask.set_basemask(invert=baseinv)
        # Inherit event
        mymask.set_startevent(event)
        # Set callback func
        mymask.set_callback(_calc_stats)
        # Selected mask
        mymask.select_mask(once=True,showmask=False)

    def _mod_note(self,event):
        # Do not fire event when in zooming/panning mode
        if not self.figmgr.toolbar.mode == '':
            return
        if event.button ==1:
            self.notewin.load_textwindow(event)
        elif event.button == 3 and self._note_picked(event):
            self.notewin.load_modmenu(event)
        return

    def _note_picked(self,event):
        # just briefly check if any texts are picked
        for textobj in self.canvas.figure.texts:
            if textobj.contains(event)[0]:
                return True
        for ax in self.canvas.figure.axes:
            for textobj in ax.texts:
                if textobj.contains(event)[0]:
                    return True
        #print "No text picked"
        return False

    def _new_page(self,next=True):
        self.plotter._plotter.hold()
        #self.plotter._plotter.clear()
        self.plotter._plot(self.plotter._data)
        self.plotter._plotter.release()
        self.plotter._plotter.tidy()
        self.plotter._plotter.show(hardrefresh=False)
        pass

#####################################
##    Backend dependent Classes    ##
#####################################
### TkAgg
if matplotlib.get_backend() == 'TkAgg':
    import Tkinter as Tk
    from notationwindow import NotationWindowTkAgg

class CustomToolbarTkAgg(CustomToolbarCommon, Tk.Frame):
    def __init__(self,parent):
        from asap.asapplotter import asapplotter
        if not isinstance(parent,asapplotter):
            return False
        if not parent._plotter:
            return False
        self._p=parent._plotter
        self.figmgr=self._p.figmgr
        self.canvas=self.figmgr.canvas
        self.mode=''
        self.button=True
        self._add_custom_toolbar()
        self.notewin=NotationWindowTkAgg(master=self.canvas)
        CustomToolbarCommon.__init__(self,parent)

    def _add_custom_toolbar(self):
        Tk.Frame.__init__(self,master=self.figmgr.window)
        self.bSpec=self._NewButton(master=self,
                                   text='spec value',
                                   command=self.spec_show)
        self.bStat=self._NewButton(master=self,
                                   text='statistics',
                                   command=self.stat_cal)
        self.bNote=self._NewButton(master=self,
                                   text='notation',
                                   command=self.modify_note)
        #self.bPrev=self._NewButton(master=self,
        #                           text='- page',
        #                           command=self.prev_page)
        self.bNext=self._NewButton(master=self,
                                   text='+ page',
                                   command=self.next_page)
        if os.uname()[0] != 'Darwin':
            #self.bPrev.config(padx=5)
            self.bNext.config(padx=5)
        self.bQuit=self._NewButton(master=self,
                                   text='Quit',
                                   command=self.quit,
                                   side=Tk.RIGHT)
        self.pack(side=Tk.BOTTOM,fill=Tk.BOTH)

        self.disable_button()
        return #self

    def _NewButton(self, master, text, command, side=Tk.LEFT):
        if os.uname()[0] == 'Darwin':
            b = Tk.Button(master=master, text=text, command=command)
        else:
            b = Tk.Button(master=master, text=text, padx=2, pady=2,
                          command=command)
        b.pack(side=side)
        return b

    def spec_show(self):
        if not self.figmgr.toolbar.mode == '' or not self.button: return
        self.figmgr.toolbar.set_message('spec value: drag on a spec')
        if self.mode == 'spec': return
        self.bStat.config(relief='raised')
        self.bSpec.config(relief='sunken')
        self.bNote.config(relief='raised')
        self.mode='spec'
        self.notewin.close_widgets()
        self.__disconnect_event()
        #self.canvas.mpl_connect('button_press_event',self._select_spectrum)
        self._p.register('button_press',self._select_spectrum)

    def stat_cal(self):
        if not self.figmgr.toolbar.mode == '' or not self.button: return
        self.figmgr.toolbar.set_message('statistics: select a region')
        if self.mode == 'stat': return
        self.bSpec.config(relief='raised')
        self.bStat.config(relief='sunken')
        self.bNote.config(relief='raised')
        self.mode='stat'
        self.notewin.close_widgets()
        self.__disconnect_event()
        self._p.register('button_press',self._single_mask)

    def modify_note(self):
        if not self.figmgr.toolbar.mode == '': return
        self.figmgr.toolbar.set_message('text: select a position/text')
        if self.mode == 'note': return
        self.bSpec.config(relief='raised')
        self.bStat.config(relief='raised')
        self.bNote.config(relief='sunken')
        self.mode='note'
        self.__disconnect_event()
        self._p.register('button_press',self._mod_note)

    def prev_page(self):
        self.figmgr.toolbar.set_message('plotting the previous page')
        self._new_page(next=False)

    def next_page(self):
        self.figmgr.toolbar.set_message('plotting the next page')
        self._new_page(next=True)

    def quit(self):
        self.__disconnect_event()
        #self.delete_bar()
        self.disable_button()
        self.figmgr.window.wm_withdraw()

    def enable_button(self):
        if self.button: return
        self.bSpec.config(state=Tk.NORMAL)
        self.bStat.config(state=Tk.NORMAL)
        self.button=True
        self.spec_show()

    def disable_button(self):
        if not self.button: return
        self.bStat.config(relief='raised', state=Tk.DISABLED)
        self.bSpec.config(relief='raised', state=Tk.DISABLED)
        #self.bPrev.config(state=Tk.DISABLED)
        #self.bNext.config(state=Tk.DISABLED)
        self.button=False
        self.mode=''
        self.__disconnect_event()

    def enable_next(self):
        self.bNext.config(state=Tk.NORMAL)

    def disable_next(self):
        self.bNext.config(state=Tk.DISABLED)

    def enable_prev(self):
        #self.bPrev.config(state=Tk.NORMAL)
        pass

    def disable_prev(self):
        #self.bPrev.config(state=Tk.DISABLED)
        pass

    def delete_bar(self):
        self.__disconnect_event()
        self.destroy()

    def __disconnect_event(self):
        #idP=self.figmgr.toolbar._idPress
        #idR=self.figmgr.toolbar._idRelease
        #if idP is not None:
        #    self.canvas.mpl_disconnect(idP)
        #    self.figmgr.toolbar._idPress=None
        #if idR is not None:
        #    self.canvas.mpl_disconnect(idR)
        #    self.figmgr.toolbar._idRelease=None
        self._p.register('button_press',None)
        self._p.register('button_release',None)
