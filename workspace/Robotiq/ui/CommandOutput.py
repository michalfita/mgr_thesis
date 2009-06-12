#!/usr/bin/env python
# -*- coding: utf-8 -*-

import pygtk
pygtk.require('2.0')
import gtk
import gobject
import thread
import traceback
import sys
import os.path
import gettext
from ActionDispatcher import ActionDispatcher

def temp_print(x, n):
    print 'Tag: %s' % x.get_property('name')

class CommandOutput(gtk.TextView):
    def __init__(self, provider):
        super(CommandOutput, self).__init__()
        self.set_editable(False)
        self.provider = provider
        
        self.prepare_tags()
        
        action_dispatcher = ActionDispatcher()
        action_dispatcher['local-output-data'] += self.owrite
        action_dispatcher['remote-input-data'] += self.iwrite
        action_dispatcher['configuration-update'] += self.prepare_tags
        
        self.set_size_request(150,200)
         
        #gobject.idle_add(self.h_idle)
    
    def prepare_tags(self):
        # TODO: load styling from configuration
        tag = gtk.TextTag(name='RemoteInput')
        tag.set_property('foreground', 'Black')
        self.get_buffer().get_tag_table().add(tag)
        tag = gtk.TextTag(name='LocalOutput')
        tag.set_property('foreground' ,'Blue')
        self.get_buffer().get_tag_table().add(tag)
        self.get_buffer().get_tag_table().foreach(temp_print)
    
    def write(self, data, style):
        buffer = self.get_buffer()
        buffer.insert_at_cursor(data)
        end_iter_before = buffer.get_end_iter()
        end_iter_before.backward_chars(len(data)) # TODO: find design pattern
        end_iter_after = buffer.get_end_iter()
        buffer.apply_tag_by_name(style, end_iter_before, end_iter_after)
        
    def owrite(self, data):
        """Method displays local output of the application."""
        self.write(data, 'LocalOutput')
    
    def iwrite(self, data):
        """Method displays remote input of the application."""
        self.write(data, 'RemoteInput')
        
    def h_idle(self):
        gtk.gdk.threads_enter()
        if self.provider is not None:
          data = self.provider.read()
          if data is not None:
              self.get_buffer().insert_at_cursor(data)
        gtk.gdk.threads_leave()
        return True