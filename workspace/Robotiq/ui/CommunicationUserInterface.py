#!/usr/bin/env python
# -*- coding: utf-8 -*-

import gtk
import gobject
import gettext
from ui.ActionDispatcher import ActionDispatcher
from misc.Singleton import Singleton
from comm.CommunicationManager import CommunicationManager

"""@package ui.CommunicationUserInterface

This module contains the class responsible for common GUI elements releated
to connection in the application.
"""

class CommunicationUserInterface(Singleton):
    """
    The CommunicationInterface class for UI manipulation.
    
    This class connects the GUI with the Communication Manager responsible for
    different communication channels.
    """
    def __init__(self):
        super(self.__class__, self).__init__()
        self.__comm_mgr = CommunicationManager()
        self.__feed_uimanager()
        self.__add_channels()
        action_dispatcher = ActionDispatcher()
        action_dispatcher['connect'] += self.connect
        action_dispatcher['disconnect'] += self.disconnect
        action_dispatcher['connection-setup'] += self.setup_dialog
        
    def __feed_uimanager(self):
        print "$Debug feed..."
        from ui.MainWindow import MainWindow
        main_window = MainWindow()
        self.__uimanager = main_window.uimanager
        self.__main_actiongroup = gtk.ActionGroup('CommUserIfaceActionGroup')
        self.__chan_actiongroup = gtk.ActionGroup('CommChannelsActionGroup')
        action_dispatcher = ActionDispatcher()
        self.__main_actiongroup.add_actions([
                ('connect', gtk.STOCK_CONNECT, _('_Connect'), 'F5', _('Connect to the robot.'), action_dispatcher['connect']),
                ('disconnect', gtk.STOCK_DISCONNECT, _('_Disconnect'), None, _('Disconnect the connection.'), action_dispatcher['disconnect']),
                ('connection-setup', gtk.STOCK_PROPERTIES, _('Confi_guration'), None, _('Configure all communication channels.'), action_dispatcher['connection-setup'])
            ])
        
        self.__uimanager.insert_action_group(self.__main_actiongroup, 1)
        self.__uimanager.insert_action_group(self.__chan_actiongroup, 2)
        self.__uimanager.ensure_update()
        self.__main_actiongroup.get_action('connect').set_sensitive(True)
        self.__main_actiongroup.get_action('disconnect').set_sensitive(False)
        #main_window.update_bars()
    
    def __add_channels(self):
        # Roll through all channels...
        actions_list = []
        items_string = ''
        number = 0
        channels_data =  self.__comm_mgr.get_channels_data()
        for chan_data in channels_data:
            actions_list.append(
                (chan_data['action'], None, chan_data['menu'], None, chan_data['tooltip'], number)
            )
            items_string += '<menuitem name="%s" action="%s" />\n' % (chan_data['name'], chan_data['action'])
            if number == 0:
                self.select(chan_data['name'])
            number += 1
        #self.__chan_actiongroup.add_toggle_actions(actions_list)
        self.__chan_actiongroup.add_radio_actions(actions_list, on_change = self.__on_change)
        # Feed uimanager with additional menu items.
        id = self.__uimanager.add_ui_from_string('''
        <ui>
          <menubar name="MenuBar">
            <menu name="ConnectionMenu" action="connection-menu">
              <placeholder name="ConnectionPlaceholder">
                %s
              </placeholder>
            </menu>
          </menubar>
        </ui>
        ''' % items_string)
        return
    
    def __update_uimanager(self):
        """
        This private method updates the uimanager and controls.
        @todo GUI has to be handled in higher level class with commonized
              interface for adding protocols similarly as plugins.
        """
        from ui.MainWindow import MainWindow
        main_window = MainWindow()
        action_dispatcher = ActionDispatcher()
        if self.__comm_mgr is not None:
          if self.__comm_mgr.state == 'connected':
              self.__main_actiongroup.get_action('connect').set_sensitive(False)
              self.__main_actiongroup.get_action('disconnect').set_sensitive(True)
              self.__main_actiongroup.get_action('connection-setup').set_sensitive(False)
              self.__chan_actiongroup.set_sensitive(False)
          else:
              self.__main_actiongroup.get_action('connect').set_sensitive(True)
              self.__main_actiongroup.get_action('disconnect').set_sensitive(False)
              self.__main_actiongroup.get_action('connection-setup').set_sensitive(True)
              self.__chan_actiongroup.set_sensitive(True)
          self.__uimanager.ensure_update()
        return 
    
    def __on_change(self, current, user_param = None):
        print "Selected:", current
        #TODO: Need finish, without additional dummy communicator this is impossible
    
    def get_tabs(self):
        tabs = []
        for chan_data in self.__comm_mgr.get_channels_data():
            tab = {
              'name' : chan_data['name'],
              'parameters' : chan_data['parameters'],
              'setup_method' : chan_data['setup_method'],
            }
            tabs.append(tab)
        return tabs
    
    def setup(self, tabs):
        channels_data = self.__comm_mgr.get_channels_data()
        for num, tab in enumerate(tabs):
            if channels_data[num]['name'] == tab['name']:
                for param in channels_data[num]['parameters']:
                    value = tab['parameters'][param]['value']
                    channels_data[num]['parameters'][param]['value'] = value
        self.__comm_mgr.set_channels_data(channels_data)
    
    def select(self, name):
        if self.__comm_mgr is not None:
            self.__comm_mgr.select(name)
    
    def connect(self, p = None):
        if self.__comm_mgr is not None:
            self.__comm_mgr.connect()
        self.__update_uimanager()
        
    def disconnect(self, p = None):
        if self.__comm_mgr is not None:
            self.__comm_mgr.disconnect()
        self.__update_uimanager()
        
    def setup_dialog(self, p = None):
        dialog = SetupDialog()
        response = dialog.run()
        
        
class SetupDialog(gtk.Dialog):
    def __init__(self):
        from ui.MainWindow import MainWindow
        main_window = MainWindow()
        super(self.__class__, self).__init__(_('Connection configuration'), main_window, gtk.DIALOG_MODAL | gtk.DIALOG_DESTROY_WITH_PARENT, None)
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
        self.__tabs = CommunicationUserInterface().get_tabs()
        self.__current_page = 0
        for tab in self.__tabs:
            rows = 0
            table = gtk.Table(1, 2, True) 
            table.set_row_spacings(4)
            table.set_col_spacings(2)
            table.set_border_width(6)
            for name, details in tab['parameters'].items():
                rows += 1
                table.resize(rows, 2)
                label = gtk.Label(details['name']+':')
                label.set_justify(gtk.JUSTIFY_RIGHT)
                label.set_alignment(1.0, 0.5)
                label.show()
                model = gtk.ListStore(gobject.TYPE_STRING, gobject.TYPE_PYOBJECT)
                combo = gtk.ComboBox(model)
                combo.set_wrap_width(1)
                cell = gtk.CellRendererText()
                combo.pack_start(cell, True)
                combo.add_attribute(cell, 'text', 0)

                if 'options' in details and details['options'] is not None:
                    for option in details['options']:
                        #combo.append_text(str(option))
                        iter = model.append((str(option), option))
                        if details['value'] == option:
                            combo.set_active_iter(iter)
                elif 'logic' in details:
                    #combo.append_text(_('Disable'))
                    iter_true = model.append((_('Disable'), 0))
                    #combo.append_text(_('Enable'))
                    iter_false = model.append((_('Enable'), 1))
                    if details['value'] == True or details['value'] == 1:
                        combo.set_active_iter(iter_true)
                    else:
                        combo.set_active_iter(iter_false)
                combo.connect('changed', self.__on_combo_change, name)
                combo.show()
                table.attach(label, 0, 1, details['position'] - 1, details['position'])
                table.attach(combo, 1, 2, details['position'] - 1, details['position'])
            self.__notebook.append_page(table, gtk.Label(tab['name']))
            table.show()
        
        self.vbox.pack_start(self.__notebook)
        self.__notebook.show()
        return
    
    def __on_combo_change(self, combo, param):
        self.__save_button.set_sensitive(True)
        iter = combo.get_active_iter()
        model = combo.get_model()
        if iter is not None and model is not None:
            value = model.get_value(iter, 1)
            self.__tabs[self.__current_page]['parameters'][param]['value'] = value
            print 'New value set to', value
        
    def __on_cancel_clicked(self, button, param = None):
        self.response(gtk.RESPONSE_CANCEL)
        self.destroy()
        
    def __on_save_clicked(self, button, param = None):
        self.response(gtk.RESPONSE_ACCEPT)
        comm_ui_if = CommunicationUserInterface()
        comm_ui_if.setup(self.__tabs)    
        self.destroy()
    
    def __on_switch_page(self, notebook, page, page_num, user_param = None):
        print "Page:",page
        print "Page num:", page_num
        self.__current_page = page_num
        
    def garbage(self):
        for tab in CommunicationUserInterface().get_tabs():
            liststore = gtk.ListStore(gobject.TYPE_STRING, gobject.TYPE_STRING, gobject.TYPE_STRING)
            for name, details in tab['parameters'].items():
                if 'options' in details:
                    options = details['options']
                elif 'logic' in details:
                    options = ['Disable', 'Enable']
                liststore.insert(
                      details['position'],
                      [name, 'A', options]
                    )
                print "@Debug loop", name, details['position'] 
            treeview = gtk.TreeView(liststore)
            treeview.append_column(gtk.TreeViewColumn(_('Parameter'), gtk.CellRendererText(), text = 0))
            value_renderer = gtk.CellRendererCombo()
            value_renderer.text_column = 2
            value_renderer.mode = gtk.CELL_RENDERER_MODE_EDITABLE
            value_renderer.editable = True
            value_renderer.editable_set = True
            treeview.append_column(gtk.TreeViewColumn(_('Value'), value_renderer, text = 1))
            self.__notebook.append_page(treeview, gtk.Label(tab['name']))
            treeview.show()      