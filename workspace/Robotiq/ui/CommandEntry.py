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
import logging
from ui.ActionDispatcher import ActionDispatcher

class CommandEntry(gtk.ComboBoxEntry):
    """Class for command entry combo box holding history of commands."""
    def __init__(self, provider, output, history_length = 10):
        super(CommandEntry, self).__init__(model = None, column = -1)
        self.__log = logging.getLogger('robotiq.command_entry')
        
        self.history_length = history_length
        self.provider = provider
        self.output = output
        self.action_dispatcher = ActionDispatcher()
        
        model = gtk.ListStore(str)
        #model.connect("row-inserted", self.h_row_inserted)
        self.set_model(model)
        self.set_text_column(0)
        self.entry = self.get_child()
        
        self.entry.connect("changed", self.h_changed)
        self.entry.connect("activate", self.h_activate)
    
    def set_history_length(self, history_length):
        self.history_length = history_length
        #gobject.idle_add(self.model_process) # Defer model processing to idle
        return
    
    def h_changed(self, entry):
        if self.get_active() != -1:
            self.entry.grab_focus()
            (sel_start, sel_end) = self.entry.get_selection_bounds()
            self.entry.select_region(sel_end, sel_end)
        return True
    
    def h_activate(self, widget, ):
        entry_text = self.entry.get_text()
        
        if 'local-output-data' in self.action_dispatcher:
            self.action_dispatcher['local-output-data'](entry_text + "\r\n")
        if self.provider is not None:
            self.provider.write(entry_text + "\n")
        if self.output is not None:
            self.output.owrite(entry_text + "\n")
        gobject.idle_add(self.model_process, entry_text) # Defer model processing to idle
        self.entry.set_text('')
        return True
    
    def model_process(self, entry_text = None):
        """Adds history entry (if not none or empty) to the list and limits
           number of elements."""
        model = self.get_model()
        if entry_text is not None and len(entry_text) > 0:
            for row in model:
                if row[0] == entry_text:
                    self.__log.debug('Debug %s' % str(row.iter))
                    model.move_before(row.iter, model[0].iter)
                    break
            else:
                model.prepend([entry_text])                    
        while len(model) > self.history_length:
            path = len(model) - 1
            it = model.get_iter(path)
            model.unref_node(it)
            model.remove(it)
        return False # stops processing in loop
    
    def h_row_inserted(self, model, path, iter):
        return True
