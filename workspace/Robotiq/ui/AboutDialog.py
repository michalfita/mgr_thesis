#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""@package ui.AboutDialog
Implementation of the about dialog.

Please refer to \b GTK+ or \b pygtk documentation for more details, how this
mechanism behaves.
"""

import pygtk
pygtk.require('2.0')
import gtk
import gobject
import thread
import traceback
import sys
import os.path
import gettext

LICENSE = """Copyright 2009 Michał Fita

Licensed under the Apache License, Version 2.0 (the "License"); you may not use \
this file except in compliance with the License. You may obtain a copy of the \
License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed \
under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR \
CONDITIONS OF ANY KIND, either express or implied. See the License for the \
specific language governing permissions and limitations under the License."""

class AboutDialog(gtk.AboutDialog):
    """The about dialog class.
    """
    def __init__(self):
        """Object constructor.
        
        Calls base constructor and sets hooks for e-mail and url.
        """
        super(AboutDialog, self).__init__()
        gtk.about_dialog_set_email_hook(self.email_hook, None)
        gtk.about_dialog_set_url_hook(self.url_hook, None)
        
    def run(self):
        """Executes the dialog.
        
        This method prepares all fields available in GTK+ about dialog and then
        calls the method \p run() from the GTK+ base class gtk.AboutDialog.
        """
        self.set_name('Robotiq')
        self.set_version(_('Release %d') % 1)
        self.set_copyright(_('Copyright by Michał Fita © 2008-2009. Some rights reserved according to the license.'))
        self.set_comments(_('This is the application for control the robot mechanics by processor board processing the commands.'))
        self.set_license(LICENSE)
        self.set_wrap_license(80)
        self.set_website('http://michal.fita.me/')
        self.set_website_label('http://michal.fita.me/')
        self.set_authors(u'Michał Fita <michal.fita@gmail.com>')
        self.set_documenters(u'None yet')
        self.set_artists(_('Author of the icon: %s') % u'Cian Walsh <cian@afterglow.ie>')
        self.set_translator_credits(_('translator-credits'))
        self.set_program_name('Robotiq')
        pixbuf = gtk.gdk.pixbuf_new_from_file('icons/robot2-128x128.png')
        self.set_logo(pixbuf)
        self.connect('response', self.on_response)
        super(AboutDialog, self).run()
    
    def email_hook(self, dialog, link, data):
        """E-mail hook method.
        """
        dialog.response(gtk.RESPONSE_CLOSE)
        
    def url_hook(self, dialog, link, data):
        """URL hook method.
        """
        dialog.response(gtk.RESPONSE_CLOSE)

    def on_response(self, windows, response_code):
        print response_code
        print "gtk.RESPONSE_CLOSE = ",gtk.RESPONSE_CLOSE
	if response_code == gtk.RESPONSE_CLOSE:
            self.hide()
        if response_code == gtk.RESPONSE_CANCEL:
	    self.hide()
