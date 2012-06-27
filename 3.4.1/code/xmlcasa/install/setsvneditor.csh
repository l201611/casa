#!/bin/echo Usage: source
#
# This used to be a little script to set the SVN editor and load in some
# required infomation for committing changes, with a vi bias.
#
# It now just sets SVN_EDITOR to be a script itself, which is more convenient
# and supports most editors.

#Routine for setting the SVN_EDITOR to get commit template
set AIPSROOT=`/bin/echo $CASAPATH | awk '{print $1}'`

setenv SVN_EDITOR $AIPSROOT/code/xmlcasa/install/svn_editor
echo "SVN_EDITOR set to $SVN_EDITOR"

# A warning for anyone relying on the old setsvneditor.csh to bizarrely set
# SVN_EDITOR to the archenemy of their EDITOR.  Most people should see no
# difference.
if ("$1" != "") then
   echo 'It loads the template in $VISUAL, $EDITOR, or vi, respectively,'
   echo "ignoring your request here for $1."
endif

