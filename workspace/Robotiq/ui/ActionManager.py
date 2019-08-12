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
from Singleton import Singleton

class ActionManager(Singleton):
    def __init__(self):
        if not super(ActionManager, self).__init__():
            self.__action_method_map = {}
        
    def add_action(self, name, method):
        if not isinstance(name, str):
            raise TypeError(_('New action has to have name string, other types are not allowed.'))
        if not callable(method):
            raise TypeError(_('New action has to have callable object as second argument.'))
        self.__action_method_map[name] = method    
        return
    
    def get_action(self, name):
        if not isinstance(name, str):
            raise TypeError(_('New action has to have name string, other types are not allowed.'))
        if name not in self.__action_method_map[name]:
            raise AtributeError(_('No such action as "%s" found.') % name)
        return self.__action_method_map[name]
        
    def remove_action(self, name):
        if not isinstance(name, str):
            raise TypeError(_('New action has to have name string, other types are not allowed.'))
        if name not in self.__action_method_map[name]:
            raise AtributeError(_('No such action as "%s" found.') % name)
        del self.__action_method_map[name]
        
    def __setitem__(self, name, method):
        self.add_action(self, name, method)
        
    def __getitem__(self, name):
        self.get_action(self, name):
        
    def __delitem__(self, name):
        self.remove_action(self, name)
    
    def __contains__(self, name):
        if name in self.__action_method_map:
            return True
        return False
        
    def __iter__(self):
        return self.__action_method_map.__iter__() # FIXME: Shouldn't we protect it better?
        