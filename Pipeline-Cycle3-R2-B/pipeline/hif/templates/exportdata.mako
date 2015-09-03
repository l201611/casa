<%!
import os
%>

<%inherit file="t2-4m_details-base.html"/>


<%block name="title">Data Products for Export</%block>

<h2>Results</h2>

<h4>Summary Documents, Scripts, and Logs</h4>

<p>Pipeline processing request, results, script, and logs</p>

<table class="table table-bordered table-striped" summary="Processing documents, scripts, and logs">
    <caption>Pipeline processing documents, scripts, and logs</caption>
    <thead>
	<tr>
	    <th scope="col">Description</th>
	    <th scope="col">File name</th>
	</tr>
    </thead>
    <tbody>
%for r in result:
      <tr>
          <td>Pipeline processing request</td>
          <td>${r.pprequest}</td>
      </tr>
      <tr>
          <td>Pipeline web log</td>
          <td>${r.weblog}</td>
      </tr>
      <tr>
          <td>Pipeline processing script</td>
          <td>${r.pipescript}</td>
      </tr>
      <tr>
          <td>Pipeline restore script</td>
          <td>${r.restorescript}</td>
      </tr>
      <tr>
          <td>CASA commands log</td>
          <td>${r.commandslog}</td>
      </tr>
%endfor
    </tbody>
</table>

<h4>Calibration Instructions and Final Flags</h4>

<p>Per ASDM, text file of applycal instructions and compressed tar file of final flags </p>

<table class="table table-bordered table-striped" summary="Per ASDM calibration and flagging">
    <caption>Applycal instructions and final flags</caption>
    <thead>
	<tr>
	    <th scope="col">Measurement Set</th>
	    <th scope="col">Session</th>
	    <th scope="col">Applycal Instructions</th>
	    <th scope="col">Final Flags</th>
	</tr>
    </thead>
    <tbody>
%for r in result:
    %for session in r.sessiondict:
        %for vis in r.sessiondict[session][0]:
      <tr>
          <td>${vis}</td>
          <td>${session}</td>
          <td>${r.visdict[vis][1]}</td>
          <td>${r.visdict[vis][0]}</td>
      </tr>
        %endfor
    %endfor
%endfor
    </tbody>
</table>

<h4>Calibration Tables</h4>

<p>Per observing session, compressed tar file of the final calibration tables</p>

<table class="table table-bordered table-striped" summary="Per session calibration tables">
    <caption>Final calibration tables</caption>
    <thead>
	<tr>
	    <th scope="col">Session</th>
	    <th scope="col">Calibration Tables</th>
	</tr>
   </thead>
   <tbody>
%for r in result:
    %for session in r.sessiondict:
      <tr>
          <td>${session}</td>
          <td>${r.sessiondict[session][1]}</td>
      </tr>
    %endfor
%endfor
   </tbody>
</table>

<h4>Calibrator Source Images</h4>

<p>FITS files of all the calibrator source images</p>

<table class="table table-bordered table-striped" summary="Calibrator source images">
    <caption>Final calibrator images</caption>
    <thead>
	<tr>
	    <th scope="col">Source name</th>
	    <th scope="col">Source type</th>
	    <th scope="col">Spw</th>
	    <th scope="col">FITS file</th>
	</tr>
   </thead>
   <tbody>
%for r in result:
    %for calimage, fitsfile in zip(r.calimages[0], r.calimages[1]):
      <tr>
          <td>${calimage['sourcename']}</td>
          <td>${calimage['sourcetype']}</td>
          <td>${calimage['spwlist']}</td>
          <td>${os.path.basename(fitsfile)}</td>
      </tr>
    %endfor
%endfor
   </tbody>
</table>

<h4>Target Source Images</h4>

<p>FITS files of all the target source images</p>

<table class="table table-bordered table-striped" summary="Target source images">
    <thead>
	<tr>
	    <th scope="col">Source name</th>
	    <th scope="col">Source type</th>
	    <th scope="col">Spw</th>
	    <th scope="col">FITS file</th>
	</tr>
   </thead>
   <tbody>
%for r in result:
    %for targetimage, fitsfile in zip(r.targetimages[0], r.targetimages[1]):
      <tr>
          <td>${targetimage['sourcename']}</td>
          <td>${targetimage['sourcetype']}</td>
          <td>${targetimage['spwlist']}</td>
          <td>${os.path.basename(fitsfile)}</td>
      </tr>
    %endfor
%endfor
   </tbody>
</table>