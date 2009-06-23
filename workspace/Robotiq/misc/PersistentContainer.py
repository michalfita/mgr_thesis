#!/usr/bin/env python
# -*- coding: utf-8 -*-

import gettext
import os
import os.path
import atexit
import inspect
from ConfigParser import SafeConfigParser
from ConfigParser import NoOptionError
from misc.Singleton import Singleton

class PersistentContainer(Singleton):
    class __SectionContainer(object):
        def __init__(self, section, parser):
            self.__section = section
            self.__parser = parser
            
        def __getattr__(self, name):
            try:
                value = self.__parser.get(self.__section, name)
            except NoOptionError:
                value = None
            return value
            
        def __setattr__(self, name, value):
            if name[:1] == '_':
                self.__dict__[name] = value
            else:
                self.__parser.set(self.__section, name, str(value))
            
            
    def __init__(self):
        super(self.__class__, self).__init__()
        self.__cfgpr = SafeConfigParser()
        self.__cfgpath = os.path.expanduser('~/.pyconsole.cfg')
        print 'Debug path:', self.__cfgpath
        self.__cfgpr.read(self.__cfgpath)
        atexit.register(self.__atexit)
    
    def __del__(self):
        print '__del__()'
        fp = open(self.__cfgpath, "w")
        print 'Debug file:', fp
        self.__cfgpr.write(fp)
        fp.close()   
        
    def __atexit(self):
        print '__atexit()'
        fp = open(self.__cfgpath, "w")
        print 'Debug file:', fp
        self.__cfgpr.write(fp)
        fp.close()
        
    def put(self, section, option, value):
        if not self.__cfgpr.has_section(section):
            self.__cfgpr.add_section(section)
        self.__cfgpr.set(section, option, str(value))
    
    def get(self, section, option, type = None, default = None):
        if not self.__cfgpr.has_section(section):
            return default
        try:
            data = self.__cfgpr.get(section, option)
        except NoOptionError:
            data = None
        else:
            if type is not None and inspect.isclass(type):
                return type(data)
        return data
    
    def __getitem__(self, key):
        s_key = key.split('.')
        if len(s_key) < 2:
            raise KeyError("Key has to be in form 'section.option'")
        section, option = s_key
        return self.get(section, option)    
    
    def __setitem__(self, key, value):
        s_key = key.split('.')
        if len(s_key) < 2:
            raise KeyError("Key has to be in form 'section.option'")
        section, option = s_key
        self.put(section, option, value)
        
    def __getattr__(self, name):
        if not self.__cfgpr.has_section(name):
            self.__cfgpr.add_section(name)
        sec_cntr = PersistentContainer.__SectionContainer(name, self.__cfgpr)
        return sec_cntr
        
    def deprecated__setattr__(self, name, value):
        if name[:1] == '_':
            self.__dict__[name] = value
        else:
            pass