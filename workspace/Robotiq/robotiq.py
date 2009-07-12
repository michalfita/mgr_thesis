#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
@mainpage The Robotiq Project Documentation

@date 2008-2009
@author Michał Fita <michal@fite.me>
@section intro The Introduction
This project named \b Robotiq is a realization of the front end application for
the robot build as project described in my masters thesis. It is being written
on AGH University of Science and Technology in Kraków, Poland. This application
is part of larger project of the robot steering system.

Please refer to original thesis document for more details about the whole
project and all its elements.

@section toc Table of contents
\li <A HREF="namespaces.html">All available modules.</a>
\li <A HREF="classes.html">All available classes.</a>
"""
"""
@package robotiq
Documentation for main runnable module.

Here the main Application class is defined responsible for running the GUI
and handling unexpected exceptions.
"""

import pygtk
pygtk.require('2.0')
import gtk
import gobject
import glib
import thread
import traceback
import sys
import os.path
import gettext
from misc.Singleton import Singleton
from ui.MainWindow import MainWindow
from ui.ActionDispatcher import ActionDispatcher
from comm.SerialCommunicator import SerialCommunicator

## Main class for this application.
#
# This singleton class starts the GUI and tries to handle all unexpected
# exceptions and display them in the dialog window before application is closed.
class Application(Singleton):
    ## Produces exception information into GUI dialog window.
    #
    # This method formats the output from the exception traceback and puts
    # information about processed exception into the dialog window.
    # @todo Allow storage of traceback to file.
    def exception_handler(self):
        (extype, value, trace) = sys.exc_info()
        trb_list = traceback.extract_tb(trace,  10)
        del trace
        trb_string = '<span size="small">'
        for entry in trb_list:
            trb_string += _("File: %s, Line: %d,\nIn function: <b>%s</b>\nCause: %s\n") % entry
        trb_string += '</span>'
        dialog = gtk.MessageDialog(parent = None,
                                   type=gtk.MESSAGE_ERROR, 
                                   flags = gtk.DIALOG_MODAL, 
                                   buttons = gtk.BUTTONS_CLOSE)
        dialog.set_title(_('Unhandled exception'))
        dialog.set_markup(_('The <b>error</b> \"%s: %s\" that has just occured during program operation left following <b>traceback</b>:') % (extype.__name__, value))
        dialog.format_secondary_markup(trb_string)
        dialog.run()
    
    ## Application class constructor.
    #
    # Installs translation engine and handler for exit request event.
    # @warning Due to not yet solved dependencies this construction is called
    #          every time application object creation is requested, while in
    #          Singleton pattern such call should return the existing instance.
    def __init__(self):
        super(Application, self).__init__()
        translation = gettext.translation('robotiq', os.path.join('.','locale'), ['pl', 'en'], fallback = False, codeset = 'utf-8')
        translation.install(True)
        action_dispatcher = ActionDispatcher()
        action_dispatcher['application-quit'] += self.on_req_exit
    
    ## Callable method.
    #
    # This method makes instance of this class a callable object, so it may
    # behave like function.
    def __call__(self):
        try:
            #glib.threads_init()
            gtk.gdk.threads_init()
            self.main_window = MainWindow()
            #self.communicator = SerialCommunicator()
            self.main_window.show()
            #gobject.idle_add()
            #gtk.gdk.threads_enter()
            gtk.main()
        except:
            self.exception_handler()
        finally:
            gtk.main_iteration(False)
        return
    
    ## Exit request event handler method.
    #
    # Quits the application on request.
    # @param param Anything passed to signal.
    def on_req_exit(self, param):
        gtk.main_quit()
        
if __name__ == '__main__':
    application = Application()
    application()

## \page Requirements
# This application has following dependencies:
# \li PyGTK - Python bindings to GTK+
# \li PySerial - Python Serial Port Extension
# 
# In case of running under Windows operatin system, additionally:
# \li PyWin32 - Python extension for Microsoft Windows
#