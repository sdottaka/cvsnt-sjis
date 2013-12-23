/*
** The cvsgui protocol used by WinCvs
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
** License as published by the Free Software Foundation; either
** version 2.1 of the License, or (at your option) any later version.
** 
** This library is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Lesser General Public License for more details.
** 
** You should have received a copy of the GNU Lesser General Public
** License along with this library; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*!
	\mainpage
 
	\section intro Introduction
 
	<i>cvsgui protocol</i> is designed to aid in the launching and control 
	over a child <a href="http://www.cvshome.org">CVS</a> client process. 
	It provides smooth interception of output and arrangement of the 
	environment as well as detection of it's termination. The standard 
	output (<b>stdout</b>) and standard error (<b>stderr</b>) are 
	intercepted independently which gives more flexibility in presenting 
	the results allowing to create graphical user interface applications 
	for CVS easily.

	<i>cvsgui protocol</i> is easy to integrate with existing applications 
	and is already build into <a href="http://www.cvsnt.org">CVSNT</a> 
	client binary as well as provided by the 
	<a href="http://cvsgui.sourceforge.net/">CvsGui</a> patched version of 
	classic CVS client.

	The source code is distributed under the terms of 
	<a href="http://www.opensource.org/licenses/lgpl-license.php">GNU Lesser GPL</a> 
	license which allows both 
	<a href="http://www.opensource.org/docs/definition.php">Open Source</a> 
	and commercial software to use it freely.

	Large portions of <i>cvsgui protocol</i> code originate from 
	<a href="http://www.gimp.org/">GNU Image Manipulation Program (GIMP)</a> 
	project.

	\section design How it works

	<i>cvsgui protocol</i> (protocol) arranges communication between two 
	applications: graphical user interface (GUI) and CVS client binary (CVS). 
	
	On the GUI application side two pipes for reading and writing are opened. 
	Their file descriptors are converted to strings and added to the command 
	line for the CVS application to use. The command line formatted by the 
	GUI part has an indicator of protocol being used in the form of 
	<i>-cvsgui</i> switch preceding the actual pipes file descriptors so CVS 
	is able to detect the protocol command line and parse it accordingly.
	
	CVS utilizes the pipes to send all it's output and errors. CVS can use 
	protocol to query the environment variables from the GUI during it's 
	execution and it can communicate the exit code after it completes its 
	operation,

	GUI application is notified about output and errors sent by CVS using 
	simple callback mechanism for standard output and standard error streams 
	as well as environment variables query and exit code notification.

	\section implementation How to use it

	To use the protocol in your GUI application you need a CVS client that 
	supports it. <a href="http://www.cvsnt.org">CVSNT</a> supports protocol 
	in full and <a href="http://cvsgui.sourceforge.net/">CvsGui</a> project 
	can be the source of patched classic CVS client that does so. These 
	projects can be used as a reference point to implement both GUI and CVS 
	side of the protocol.

	GUI application should use the following files:
		- cvsgui_process.h
		- cvsgui_process.cpp
		- cvsgui_protocol.h
		- cvsgui_wire.h
		- cvsgui_wire.cpp

	The cvsgui_process.h file contains all definitions that your GUI 
	application needs to access. To start the CVS process you need to use
	cvs_process_run(). It will open the communication pipes and format the 
	command line properly for the protocol to work.

	The cvs_process_run() takes CvsProcessCallbacks structure addresss as one 
	of it's parameters. That structure contains the callbacks for stdout, 
	stderr, environment and the exit code report. The callbacks will be 
	invoked by the protocol whenever the data is available or the value of the 
	environment variable is needed. Each callback additionally receives the 
	CVS process information as a \link _CvsProcess CvsProcess \endlink 
	structure. That structure also allows to access the application data if 
	there was one specified at cvs_process_run() call. 

	The <b>text data</b> notified to standard output and standard error 
	callbacks comes as a NULL-terminated string with lines of text separated 
	by a newline character. It is the GUI application responsibility to set 
	the appropriate newline translation mode in case the output is written to 
	the file on disk. 

	The <b>binary data</b> notified to standard output callback is 
	communicated by sending an empty string and mark the zero length of data 
	to allow GUI application change the translation mode before the actual 
	data is send.

	The return value of cvs_process_run() can be used to further control the 
	execution of launched CVS process (e.g. you can stop it by calling 
	cvs_process_stop()) or to detect whether CVS is still active. The
	cvs_process_give_time() and cvs_process_is_active() functions can be used 
	called to answer calls from the CVS process when synchronous execution is 
	needed.

	\section license License

	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 2.1 of the License, or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

	See the \ref licensepage for more details.
	
*/

/*!
	\page licensepage Full LGPL Text
	\verbinclude COPYING
*/
