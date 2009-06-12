#!/usr/bin/env python
# -*- coding: utf-8 -*-

## @package Singleton
#
# Implementation of singleton pattern base classes.
#
# This is a try for singleton pattern base classes to be inherited by other
# classes, which programmer want to make a Sigletons.
# @note This is not ideal, be careful with constructors of inheriting classes.
#       

import thread
import gtk

## @deprecated Old class no longer in use.
#
class HmmSingleton(type):
    '''Implement Pattern: SINGLETON'''
    def __init__(cls, name, bases, dic):
        super(Singleton,cls).__init__(name, bases, dic)
        cls.__instance = None
        
    def __call__(cls,*args,**kw):
        if cls.__instance is None:
            cls.__instance = super(Singleton, cls).__call__(*args ,**kw)
        return cls.__instance

class Singleton(object):
    '''
    Implements singleton pattern
    
    This class implements base class for sigleton design patter. Any other class
    which has to be a singleton shall inherit from this class.
    @warning Be careful with constructors in inherited classes.
    @bug Problem with GTK+ object still not resolved.
    
    '''
    __lockObj = thread.allocate_lock()  # lock object
    __instance = None  # the unique instance
    __initialized = False

    def __new__(cls, *args, **kargs):
        return cls.getInstance(cls, *args, **kargs)

    def __empty_init(self, *args, **kwargs):
        pass

    def __init__(self, *args, **kargs):
        """
        The constructor of singleton instance.
        
        This method is a inheritable constructor for singleton object. It
        removes itself from the class object by substitution of the empty method
        what protects further constructor calls on object creation requests (
        which means for example \code instance = ClassName() \endcode call).
        """
        self.__class__.__init__ = self.__class__.__empty_init
        #if type(self).__initialized == False:
        #    type(self).__initialized = True
        #    return False
        #return True

    @staticmethod
    def getInstance(cls, *args, **kargs):
        '''Static method to have a reference to \b **THE \b UNIQUE** instance'''
        # Critical section start
        cls.__lockObj.acquire()
        try:
            if cls.__instance is None:
                # (Some exception may be thrown...)
                # Initialize **the unique** instance
                cls.__instance = super(Singleton, cls).__new__(cls, *args, **kargs)

                '''Probably no longer true: Initialize object **here**, as you would do in __init__()...'''

        finally:
            #  Exit from critical section whatever happens
            cls.__lockObj.release()
        # Critical section end

        return cls.__instance
