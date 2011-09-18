#!/usr/bin/env python
# -*- coding: utf-8 -*-

import gtk
import gobject
import gettext
import logging
import itertools
from ui.ActionDispatcher import ActionDispatcher
from misc.Singleton import Singleton
from comm.CommunicationManager import CommunicationManager
from misc.Commands import Commands

"""@package ui.CommandsUserInterface

This module contains the class responsible for common GUI elements related
to configurable commands bound in the application.
"""

class CommandsUserInterface(Singleton):
    """
    The CommandsUserInterface class for UI manipulation.
    
    This class connects the GUI with the ... responsible for
    sending configured commands invoked by control buttons.
    """
    def __init__(self):
        super(self.__class__, self).__init__()
        self.__log = logging.getLogger('robotiq.cmds_ui')
        self.__commands = Commands()
        self.__feed_uimanager()
        action_dispatcher = ActionDispatcher()
        action_dispatcher['commands-settings'] += self.settings_dialog
        
    def __feed_uimanager(self):
        self.__log.debug("Called __feed_uimanager()")
        from ui.MainWindow import MainWindow
        main_window = MainWindow()
        self.__uimanager = main_window.uimanager
        self.__main_actiongroup = gtk.ActionGroup('CmdsUserIfaceActionGroup')
        action_dispatcher = ActionDispatcher()
        self.__main_actiongroup.add_actions([
                ('commands-settings', gtk.STOCK_PROPERTIES, _('_Settings'), None, _('Configure commands bound to buttons.'), action_dispatcher['commands-settings'])
            ])
        
        self.__uimanager.insert_action_group(self.__main_actiongroup, 1)
        self.__uimanager.ensure_update()
    
    def get_tabs(self):
        tabs = []
        commands_dict = self.__commands.get_commands_dict()
        self.__log.debug('commands_dict = %s', repr(commands_dict))
        for t in self.__commands.get_types():
            position = itertools.count(1)
            parameters = dict([(k,{'name':x.name, 'command': x.command, 'position':position.next() }) for k,x in commands_dict.items() if x.type == t])
            tabs.append({
                         'name' : {'hip':_('Hips'), 'arm':_('Arms'),'leg':_('Legs')}[t],
                         'parameters' : parameters,
                        })
        return tabs
    
    def setup(self, tabs):
#        channels_data = self.__comm_mgr.get_channels_data()
        commands_dict = self.__commands.get_commands_dict()
        for num, tab in enumerate(tabs):
#            if channels_data[num]['name'] == tab['name']:
            for action, d in tab['parameters'].items():
                commands_dict[action].command = d['command']
#                for param in channels_data[num]['parameters']:
#                    value = tab['parameters'][param]['value']
#                    channels_data[num]['parameters'][param]['value'] = value
#        self.__comm_mgr.set_channels_data(channels_data)
        pass
            
    def settings_dialog(self, p = None):
        dialog = SettingsDialog()
        response = dialog.run()
        
class SettingsDialog(gtk.Dialog):
    def __init__(self):
        self.__log = logging.getLogger('robotiq.cmds_ui.settings_dlg')
        from ui.MainWindow import MainWindow
        main_window = MainWindow()
        super(self.__class__, self).__init__(_('Commands configuration'), main_window, gtk.DIALOG_MODAL | gtk.DIALOG_DESTROY_WITH_PARENT, None)
        #Buttons (FIXME: order could be subject to change)
        self.__save_button = gtk.Button(None, gtk.STOCK_SAVE)
        self.__save_button.use_stock = True
        self.__save_button.set_sensitive(False)
        self.__save_button.connect('clicked', self.__on_save_clicked)
        self.add_action_widget(self.__save_button, gtk.RESPONSE_ACCEPT)
        self.__save_button.show()
        self.__cancel_button = gtk.Button(None, gtk.STOCK_CANCEL)
        self.__cancel_button.connect('clicked', self.__on_cancel_clicked)
        self.__cancel_button.show()
        self.add_action_widget(self.__cancel_button, gtk.RESPONSE_CANCEL)
        #Notebook
        self.__notebook = gtk.Notebook()
        self.__notebook.connect('switch-page', self.__on_switch_page)
        self.__tables = []
        self.__tabs = CommandsUserInterface().get_tabs()
        self.__current_page = 0
        for tab in self.__tabs:
            rows = 0
            table = gtk.Table(1, 2, True) 
            table.set_row_spacings(4)
            table.set_col_spacings(2)
            table.set_border_width(6)
            for action, details in tab['parameters'].items():
                rows += 1
                table.resize(rows, 2)
                label = gtk.Label(_(details['name']) + ':')
                label.set_justify(gtk.JUSTIFY_RIGHT)
                label.set_alignment(1.0, 0.5)
                label.show()
                entry = gtk.Entry()
                entry.set_width_chars(30)
                entry.set_text(details['command'])
                entry.connect('changed', self.__on_entry_change, action)
                entry.show()
                table.attach(label, 0, 1, details['position'] - 1, details['position'])
                table.attach(entry, 1, 2, details['position'] - 1, details['position'])
            self.__notebook.append_page(table, gtk.Label(tab['name']))
            table.show()
        
        self.vbox.pack_start(self.__notebook)
        self.__notebook.show()
        return
    
    def __on_entry_change(self, entry, param):
        self.__save_button.set_sensitive(True)
        command = entry.get_text()
        self.__tabs[self.__current_page]['parameters'][param]['command'] = command
        self.__log.debug('New command set to %s.' % str(command))
            
    def __on_cancel_clicked(self, button, param = None):
        self.response(gtk.RESPONSE_CANCEL)
        self.destroy()
        
    def __on_save_clicked(self, button, param = None):
        self.response(gtk.RESPONSE_ACCEPT)
        comm_ui_if = CommandsUserInterface()
        comm_ui_if.setup(self.__tabs)    
        self.destroy() 
     
    def __on_switch_page(self, notebook, page, page_num, user_param = None):
        self.__log.debug('__on_switch_page(): Page: %s; Page number: %d' % (str(page), page_num))
        self.__current_page = page_num                