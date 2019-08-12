#!/usr/bin/env python
# -*- coding: utf-8 -*-

import pygtk
pygtk.require('2.0')
import gtk
import gobject
import cairo
import thread
import traceback
import sys
import os.path
import gettext
from math import pi
from ActionDispatcher import ActionDispatcher

class ControlButtonBase(object):
    def __init__(self, container):
        if self.__class__ is ControlButtonBase:
            raise TypeError('You cannot use base type directly. Inheritance must take place.')
        self._container = container
        self._drawing_table = []
        self._boundaries_table = []
        #self._colors = {
        #  #'relief' : (0.965, 0.953, 0.897, 1.0),
        #  'relief' : self._container._color_table[gtk.STATE_PRELIGHT]['relief'],
        #  #'etch' : (0.35, 0.35, 0.35, 1.0)
        #  'etch' : self._container._color_table[gtk.STATE_PRELIGHT]['etch']
        #}
        self._process_color(gtk.STATE_INSENSITIVE)
        self._state = 'Released'
    
    def _test(self, context, mx,my):
        """This is planned to test the cursor click place."""
        
        self._draw(context, self._boundaries_table) # Fake draw for testing
        
        (mx, my) = context.device_to_user(mx, my)
        
        return context.in_fill(mx, my), context.in_stroke(mx, my)
    
    def _process_color(self, state):
        style = self._container.get_style()
        #for state in [gtk.STATE_NORMAL]: #, gtk.STATE_ACTIVE, gtk.STATE_PRELIGHT, gtk.STATE_SELECTED, gtk.STATE_INSENSITIVE]:
        light_color = style.light[state]
        face_color = style.bg[state]
        dark_color = style.dark[state]
        #print state, light_color.red, light_color.green, light_color.blue
        self._colors = {
            'relief' : (light_color.red/65535.0, light_color.green/65535.0, light_color.blue/65535.0, 1.0),
            'face' : (face_color.red/65535.0, face_color.green/65535.0, face_color.blue/65535.0, 1.0),
            'etch' : (dark_color.red/65535.0, dark_color.green/65535.0, dark_color.blue/65535.0, 1.0)    
        }
    
    def _relief_color(self):
        if self._state == 'Released':
            return self._colors['relief']
        elif self._state == 'Pressed':
            return self._colors['etch']
    
    def _etch_color(self):
        if self._state == 'Released':
            return self._colors['etch']
        elif self._state == 'Pressed':
            return self._colors['relief']
    
    def _face_color(self):
        return self._colors['face']
    
    def _on_press(self, *args, **kwargs):
        print "Pressed", self.__class__
        self._state = 'Pressed'
    
    def _on_release(self, *args, **kwargs):
        print "Released", self.__class__
        self._state = 'Released'
        
    def _draw(self, context, drawing_table = None):
        self._process_color({'Pressed':gtk.STATE_ACTIVE, 'Released':gtk.STATE_NORMAL}[self._state])
        if drawing_table is None:
            drawing_table = self._drawing_table
        for element in drawing_table:
            x_list = []
            y_list = []
            context.new_path()
            for command in element:
                if command[0] == 'm':
                    context.move_to(command[1], command[2])
                    x_list.append(command[1])
                    y_list.append(command[2])
                elif command[0] == 'l':
                    context.line_to(command[1], command[2])
                    x_list.append(command[1])
                    y_list.append(command[2])
                elif command[0] == 'b':
                    context.curve_to(command[1], command[2], command[3], command[4], command[5], command[6])
                    x_list.append(command[5])
                    y_list.append(command[6])
                elif command[0] == 'a':
                    context.arc(command[1], command[2], command[3], command[4], command[5])
                    x_list.append(command[1] - command[3])
                    x_list.append(command[1] + command[3])
                    y_list.append(command[2] - command[3])
                    y_list.append(command[2] + command[3])
                elif command[0] == 'n':
                    context.arc_negative(command[1], command[2], command[3], command[4], command[5])
                    x_list.append(command[1] - command[3])
                    x_list.append(command[1] + command[3])
                    y_list.append(command[2] - command[3])
                    y_list.append(command[2] + command[3])
                elif command[0] == 's':
                    if isinstance(command[1], tuple):
                        pattern = cairo.SolidPattern(command[1][0], command[1][1], command[1][2], command[1][3])
                    elif callable(command[1]):
                        data = command[1]()
                        pattern = cairo.SolidPattern(data[0], data[1], data[2], data[3])
                    else:    
                        pattern = cairo.SolidPattern(command[1], command[2], command[3], command[4])
                    context.set_source(pattern)
                elif command[0] == 'g':
                    pattern = cairo.LinearGradient(min(x_list), min(y_list), max(x_list), max(y_list))
                    if callable(command[2]) and callable(command[4]):
                        data = command[2]()
                        pattern.add_color_stop_rgba(command[1], data[0], data[1], data[2], data[3])
                        data = command[4]()
                        pattern.add_color_stop_rgba(command[3], data[0], data[1], data[2], data[3])
                    else:
                        pattern.add_color_stop_rgb(command[1], command[2], command[3], command[4])
                        pattern.add_color_stop_rgb(command[5], command[6], command[7], command[8])
                    context.set_source(pattern)
                elif command[0] == 'p':
                    context.set_source_rgba(command[1], command[2], command[3], command[4])
                    context.set_line_width(command[5])
                    context.stroke_preserve()
                    context.set_source_rgba(command[1], command[2], command[3], 0.0)
                else:
                    raise NotImplementedError(_('Command "%s" not implemented in drawing.') % command[0])
            context.close_path()
            #pattern = cairo.LinearGradient(min(x_list), min(y_list), max(x_list), max(y_list))
            #pattern.add_color_stop_rgb(0.25, 100, 100, 100)
            #pattern.add_color_stop_rgb(1, 0, 0, 0)
            context.fill_preserve()
        return

class ControlButtonUp(ControlButtonBase):
    def __init__(self, container):
        super(ControlButtonUp, self).__init__(container)
        self._press_action_name = 'control-up-press'
        self._release_action_name = 'control-up-release'
        self._drawing_table = [
          (('m', 50, 110),
           ('l', 30, 130),
           ('l', 30+4.366, 130-2.435),
           ('l', 50, 112.3),
           #('b', 40, 25, 60, 25, 70, 30),
           ('l', 50, 110),
           #('s', 0.965, 0.953, 0.897, 1.0)),
           ('s', self._relief_color)),
          (('m', 50, 110),
           ('l', 70, 130),
           ('b', 60, 125, 40, 125, 30, 130),
           ('l', 30+4.366, 130-2.435),
           ('b', 36+4.366, 126-2.435, 64-4.366, 126-2.435, 70-4.366, 130-2.435),
           #('l', 70-4.366, 30-2.435),
           ('l', 50, 112.3),
           ('l', 50, 110),
           #('s', 0.35, 0.35, 0.35, 1.0)),
           ('s', self._etch_color)),
          (('m', 50, 112.3),
           ('l', 30+4.366, 130-2.435),
           ('b', 36+4.366, 126-2.435, 64-4.366, 126-2.435, 70-4.366, 130-2.435),
           ('l', 50, 112.3),
           ('s', self._face_color)),
          (('m', 50, 116),
           ('l', 50-4.064, 116+5.699),
           ('l', 50+4.064, 116+5.699),
           ('s', 0.95, 0, 0, 1.0))
        ]
        self._boundaries_table = [
          (('m', 50, 110),
           ('l', 70, 130),
           ('b', 64, 125, 36, 125, 30, 130),
           ('l', 50, 110),
           ('s', 1, 0, 0, 1))
        ]

class ControlButtonDown(ControlButtonBase):
    def __init__(self, container):
        super(ControlButtonDown, self).__init__(container)
        self._press_action_name = 'control-down-press'
        self._release_action_name = 'control-down-release'
        self._drawing_table = [
          (('m', 30, 170),
           ('l', 50, 190),
           ('l', 70, 170),
           ('l', 70-4.366, 170+2.435),
           ('l', 50, 187.7),
           ('l', 30+4.366, 170+2.435),
           ('l', 30, 170),
           ('s', self._etch_color)),
          (('m', 70, 170),
           ('b', 60, 175, 40, 175, 30, 170),
           ('l', 30+4.366, 170+2.435),
           ('b', 36+4.366, 174+2.435, 64-4.366, 174+2.435, 70-4.366, 170+2.435),
           ('l', 70, 170),
           ('s', self._relief_color)),
          (('m', 50, 187.7),
           ('l', 30+4.366, 170+2.435),
           ('b', 36+4.366, 174+2.435, 64-4.366, 174+2.435, 70-4.366, 170+2.435),
           ('l', 50, 187.7),
           ('s', self._face_color)),
          (('m', 50, 184),
           ('l', 50-4.064, 184-5.699),
           ('l', 50+4.064, 184-5.699),
           ('s', 0.95, 0, 0, 1.0))        
        ]
        self._boundaries_table = [
          (('m', 50, 190),
           ('l', 70, 170),
           ('b', 64, 175, 36, 175, 30, 170),
           ('l', 50, 190),
           ('s', 0, 1, 0, 1))
        ]

class ControlButtonLeft(ControlButtonBase):
    def __init__(self, container):
        super(ControlButtonLeft, self).__init__(container)
        self._press_action_name = 'control-left-press'
        self._release_action_name = 'control-left-release'
        self._drawing_table = [
          (('m', 10, 150),
           ('l', 30, 130),
           ('l', 30-2.435, 130+4.366,),
           ('l', 12.3, 150),
           ('l', 10, 150),
           ('s', self._relief_color)),
          (('m', 10, 150),
           ('l', 30, 170), 
           ('b', 25, 160, 25, 140, 30, 130),
           ('l', 30-2.435, 130+4.366),
           ('b', 26-2.435, 136+4.366, 26-2.435, 164-4.366, 30-2.435, 170-4.366),
           ('l', 12.3, 150),
           ('l', 10, 150),
           ('s', self._etch_color)),
          (('m', 12.3, 150),
           ('l', 30-2.435, 130+4.366),
           ('b', 26-2.435, 136+4.366, 26-2.435, 164-4.366, 30-2.435, 170-4.366),
           ('l', 12.3, 150),
           ('s', self._face_color)),
          (('m', 16, 150),
           ('l', 16+5.699, 150-4.064),
           ('l', 16+5.699, 150+4.064),
           ('s', 0.95, 0, 0, 1.0))
        ]
        self._boundaries_table = [
          (('m', 10, 150),
           ('l', 30, 130),
           ('b', 25, 136, 25, 164, 30, 170),
           ('l', 10, 150),
           ('s', 0, 0, 1, 1))
        ]

class ControlButtonRight(ControlButtonBase):
    def __init__(self, container):
        super(ControlButtonRight, self).__init__(container)
        self._press_action_name = 'control-right-press'
        self._release_action_name = 'control-right-release'
        self._drawing_table = [
          (('m', 70, 130),
           ('l', 90, 150),
           ('l', 70, 170),
           ('l', 70+2.435, 170-4.366),
           ('l', 87.7, 150),
           ('l', 70+2.435, 130+4.366),
           ('l', 70, 130),
           ('s', self._etch_color)),
          (('m', 70, 170),
           ('b', 75, 160, 75, 140, 70, 130),
           ('l', 70+2.435, 130+4.366),
           ('b', 74+2.435, 136+4.366, 74+2.435, 164-4.366, 70+2.435, 170-4.366),
           ('l', 70, 170),
           ('s', self._relief_color)),
          (('m', 87.7, 150),
           ('l', 70+2.435, 130+4.366),
           ('b', 74+2.435, 136+4.366, 74+2.435, 164-4.366, 70+2.435, 170-4.366),
           ('l', 87.7, 150),
           ('s', self._face_color)),
          (('m', 84, 150),
           ('l', 84-5.699, 150-4.064),
           ('l', 84-5.699, 150+4.064),
           ('s', 0.95, 0, 0, 1.0))
        ]
        self._boundaries_table = [
          (('m', 90, 150),
           ('l', 70, 170),
           ('b', 75, 164, 75, 136, 70, 130),
           ('l', 90, 150),
           ('s', 1, 0, 1, 1))
        ]

class ControlButtonStop(ControlButtonBase):
    def __init__(self, container):
        super(ControlButtonStop, self).__init__(container)
        self._press_action_name = 'control-stop-press'
        self._release_action_name = 'control-stop-release'
        self._drawing_table = [
          (('a', 50, 150, 15, 3*pi/4.0, 7*pi/4.0),
           ('n', 50, 150, 17, 13*pi/8.0, 23*pi/8.0),
           ('s', self._relief_color)),
          (('a', 50, 150, 15, 7*pi/4.0, 3*pi/4.0),
           ('n', 50, 150, 17, 23*pi/8.0, 13*pi/8.0),
           #('g', 0.4, 0.965, 0.953, 0.897, 0.6, 0.35, 0.35, 0.35)),
           ('g', 0.4, self._relief_color, 0.7, self._etch_color)),
          (('a', 50, 150, 15, 0, 6.28),
           ('s', self._face_color)),
          (('m', 45, 145),
           ('l', 55, 145),
           ('l', 55, 155),
           ('l', 45, 155),
           ('l', 45, 145), 
           ('s', 0.95, 0, 0, 1.0)),
          (('m', 43, 143),
           ('l', 57, 143),
           ('l', 57, 157),
           ('l', 43, 157),
           ('l', 43, 143),
           ('p', 0.95, 0, 0, 1.0, 0.2))
        ]
        self._boundaries_table = [
          (('a', 50, 150, 17, 0, 4*pi),
           ('s', 1, 1, 0, 1))
        ]

class ControlButtonRightArmUp(ControlButtonBase):
    def __init__(self, container):
        super(ControlButtonRightArmUp, self).__init__(container)
        self._press_action_name = 'control-rightarmup-press'
        self._release_action_name = 'control-rightarmup-release'
        self._drawing_table = [
          (('m', 77.2, 10.0),
           ('l', 76.3, 12.5),
           ('l', 81.3, 19.7),
           ('l', 84.0, 20.2),
           ('l', 77.2, 10.0),
           ('s', self._etch_color)),
          (('m', 84.0, 20.2),
           ('l', 81.3, 19.7),
           ('l', 70.5, 25.6),
           ('l', 70.4, 27.8),
           ('l', 84.0, 20.2),
           ('s', self._etch_color)),
          (('m', 70.4, 27.8),
           ('l', 70.5, 25.6),
           ('l', 63.5, 19.7),
           ('l', 60.2, 19.4),
           ('l', 70.4, 27.8),
           ('s', self._etch_color)),
          (('m', 60.2, 19.4),
           ('l', 63.5, 19.7),
           ('l', 76.3, 12.5),
           ('l', 77.2, 10.0),
           ('l', 60.2, 19.4),
           ('s', self._relief_color)),
          (('m', 76.3, 12.5),
           ('l', 81.3, 19.7),
           ('l', 70.5, 25.6),
           ('l', 63.5, 19.7),
           ('l', 76.3, 12.5),
           ('s', self._face_color)) 
        ]
        self._boundaries_table = [
          (('m', 77.2, 10.0),
           ('l', 84.0, 20.2),
           ('l', 70.4, 27.8),
           ('l', 60.2, 19.4),
           ('l', 77.2, 10.0),
           ('p', 1.0, 0.0, 0.0, 1.0, 0.2))
        ]

class ControlButtonRightArmDown(ControlButtonBase):
    def __init__(self, container):
        super(ControlButtonRightArmDown, self).__init__(container)
        self._press_action_name = 'control-rightarmdown-press'
        self._release_action_name = 'control-rightarmdown-release'
        self._drawing_table = [
          (('m', 88.2, 26.5),
           ('l', 87.7, 28.9),
           ('l', 94.8, 39.5),
           ('l', 97.3, 40.3),
           ('l', 88.2, 26.5),
           ('s', self._etch_color)),
          (('m', 97.3, 40.3),
           ('l', 94.8, 39.5),
           ('l', 90.3, 42.0),
           ('l', 90.1, 44.0),
           ('l', 97.3, 40.3),
           ('s', self._etch_color)),
          (('m', 90.1, 44.0),
           ('l', 90.3, 42.0),
           ('l', 79.8, 33.0),
           ('l', 76.2, 32.6),
           ('l', 90.1, 44.0),
           ('s', self._etch_color)),
          (('m', 76.2, 32.6),
           ('l', 79.8, 33.0),
           ('l', 87.7, 28.9),
           ('l', 88.2, 26.5),
           ('l', 76.2, 32.6),
           ('s', self._relief_color)),
          (('m', 87.7, 28.9),
           ('l', 94.8, 39.5),
           ('l', 90.3, 42.0),
           ('l', 78.8, 33.0),
           ('l', 87.7, 28.9),
           ('s', self._face_color))
        ]
        self._boundaries_table = [
          (('m', 88.2, 26.5),
           ('l', 97.3, 40.3),
           ('l', 90.1, 44.0),
           ('l', 76.2, 32.6),
           ('l', 88.2, 26.5),
           ('p', 0.0, 1.0, 0.0, 1.0, 0.2))
        ]
        
class ControlButtonRightArmIn(ControlButtonBase):
    def __init__(self, container):
        super(ControlButtonRightArmIn, self).__init__(container)
        self._press_action_name = 'control-rightarmin-press'
        self._release_action_name = 'control-rightarmin-release'
        self._drawing_table = [
          (('m', 60.2, 30.4),
           ('l', 60.9, 33.3),
           ('l', 78.7, 47.0),
           ('l', 82.3, 47.4),
           ('l', 60.2, 30.4),
           ('s', self._etch_color)),
          (('m', 82.3, 47.4),
           ('l', 78.7, 47.0),
           ('l', 66.9, 52.6),
           ('l', 66.2, 55.0),
           ('l', 82.3, 47.4),
           ('s', self._etch_color)),
          (('m', 66.2, 55.0),
           ('l', 66.9, 52.6),
           ('l', 58.9, 37.8),
           ('l', 56.8, 38.0),
           ('l', 66.2, 55.0),
           ('s', self._etch_color)),
          (('m', 56.8, 38.0),
           ('l', 58.9, 37.8),
           ('l', 60.9, 33.3),
           ('l', 60.2, 30.4),
           ('l', 56.8, 38.0),
           ('s', self._relief_color)),
          (('m', 60.9, 33.3),
           ('l', 78.7, 47.0),
           ('l', 66.9, 52.6),
           ('l', 58.9, 37.8),
           ('l', 60.9, 33.3),
           ('s', self._face_color))
        ]
        self._boundaries_table = [
          (('m', 60.2, 30.4),
           ('l', 82.3, 47.4),
           ('l', 66.2, 55.0),
           ('l', 56.8, 38.0),
           ('l', 60.2, 30.4),
           ('p', 1.0, 0.0, 0.0, 0.0, 1.0, 0.2))
        ]

class ControlButtonRightArmOut(ControlButtonBase):
    def __init__(self, container):
        super(ControlButtonRightArmOut, self).__init__(container)
        self._press_action_name = 'control-rightarmout-press'
        self._release_action_name = 'control-rightarmout-release'
        self._drawing_table = [
          (('m', 88.6, 55.3),
           ('l', 88.5, 54.6),
           ('l', 95.7, 60.0),
           ('l', 99.4, 60.5),
           ('l', 88.6, 52.3),
           ('s', self._etch_color)),
          (('m', 99.4, 60.5),
           ('l', 95.7, 60.0),
           ('l', 76.1, 69.3),
           ('l', 75.5, 71.8),
           ('l', 99.4, 60.5),
           ('s', self._etch_color)),
          (('m', 75.5, 71.8),
           ('l', 76.1, 69.3),
           ('l', 72.4, 62.4),
           ('l', 69.7, 61.4),
           ('l', 75.5, 71.8),
           ('s', self._etch_color)),
          (('m', 69.7, 61.4),
           ('l', 72.4, 62.4),
           ('l', 88.5, 54.6),
           ('l', 88.6, 52.3),
           ('l', 69.7, 61.4),
           ('s', self._relief_color)),
          (('m', 88.5, 54.6),
           ('l', 95.7, 60.0),
           ('l', 76.1, 69.3),
           ('l', 72.4, 62.4),
           ('l', 88.5, 54.6),
           ('s', self._face_color))
        ]
        self._boundaries_table = [
          (('m', 88.5, 52.6),
           ('l', 99.4, 60.5),
           ('l', 75.5, 71.8),
           ('l', 69.7, 61.4),
           ('l', 88.5, 52.6),
           ('p', 0.0, 0.0, 1.0, 1.0, 0.2))
        ]

class ControlButtonTiltRight(ControlButtonBase):
    def __init__(self, container):
        super(ControlButtonTiltRight, self).__init__(container)
        self._press_action_name = 'control-tiltright-press'
        self._release_action_name = 'control-tiltright-release'
        self._drawing_table = [
          (('m', 53.4, 43.1),
           ('l', 55.2, 50.8),
           ('l', 60.9, 62.6),
           ('l', 63.6, 64.4),
           ('l', 53.4, 43.1),
           ('s', self._etch_color)),
          (('m', 63.6, 64.4),
           ('l', 60.9, 62.6),
           ('l', 55.2, 62.6),
           ('l', 53.4, 64.4),
           ('l', 63.6, 64.4),
           ('s', self._etch_color)),
          (('m', 53.4, 64.4),
           ('l', 55.2, 62.6),
           ('l', 55.2, 50.8),
           ('l', 53.4, 43.1),
           ('l', 53.4, 64.4),
           ('s', self._relief_color)),
          (('m', 55.2, 50.8),
           ('l', 60.9, 62.6),
           ('l', 55.2, 62.6),
           ('l', 55.2, 50.8),
           ('s', self._face_color))
        ]
        self._boundaries_table = [
          (('m', 53.4, 43.1),
           ('l', 63.6, 64.4),
           ('l', 53.4, 64.4),
           ('l', 53.4, 43.1),
           ('p', 0.0, 1.0, 1.0, 1.0, 0.2))
        ]

# --- Left work

class ControlButtonLeftArmUp(ControlButtonBase):
    def __init__(self, container):
        super(ControlButtonLeftArmUp, self).__init__(container)
        self._press_action_name = 'control-leftarmup-press'
        self._release_action_name = 'control-leftarmup-release'
        self._drawing_table = [
          (('m',  22.80, 10.00),
           ('l',  23.70, 12.50),
           ('l',  18.70, 19.70),
           ('l',  16.00, 20.20),
           ('l',  22.80, 10.00),
           ('s',  self._etch_color)),
          (('m',  16.00, 20.20),
           ('l',  18.70, 19.70),
           ('l',  29.50, 25.60),
           ('l',  29.60, 27.80),
           ('l',  16.00, 20.20),
           ('s',  self._etch_color)),
          (('m',  29.60, 27.80),
           ('l',  29.50, 25.60),
           ('l',  36.50, 19.70),
           ('l',  39.80, 19.40),
           ('l',  29.60, 27.80),
           ('s',  self._etch_color)),
          (('m',  39.80, 19.40),
           ('l',  36.50, 19.70),
           ('l',  23.70, 12.50),
           ('l',  22.80, 10.00),
           ('l',  39.80, 19.40),
           ('s',  self._relief_color)),
          (('m',  23.70, 12.50),
           ('l',  18.70, 19.70),
           ('l',  29.50, 25.60),
           ('l',  36.50, 19.70),
           ('l',  23.70, 12.50),
           ('s',  self._face_color))
        ]
        self._boundaries_table = [
          (('m',  22.80, 10.00),
           ('l',  16.00, 20.20),
           ('l',  29.60, 27.80),
           ('l',  39.80, 19.40),
           ('l',  22.80, 10.00),
           ('p',  1.00, 0.00, 0.00, 1.00, 0.20)),
        ]

class ControlButtonLeftArmDown(ControlButtonBase):
    def __init__(self, container):
        super(ControlButtonLeftArmDown, self).__init__(container)
        self._press_action_name = 'control-leftarmdown-press'
        self._release_action_name = 'control-leftarmdown-release'
        self._drawing_table = [
          ( ('m',  11.80, 26.50) ,
            ('l',  12.30, 28.90) ,
            ('l',  5.20, 39.50) ,
            ('l',  2.70, 40.30) ,
            ('l',  11.80, 26.50) ,
            ('s',  self._etch_color) ),
          ( ('m',  2.70, 40.30) ,
            ('l',  5.20, 39.50) ,
            ('l',  9.70, 42.00) ,
            ('l',  9.90, 44.00) ,
            ('l',  2.70, 40.30) ,
            ('s',  self._etch_color) ),
          ( ('m',  9.90, 44.00) ,
            ('l',  9.70, 42.00) ,
            ('l',  20.20, 33.00) ,
            ('l',  23.80, 32.60) ,
            ('l',  9.90, 44.00) ,
            ('s',  self._etch_color) ),
          ( ('m',  23.80, 32.60) ,
            ('l',  20.20, 33.00) ,
            ('l',  12.30, 28.90) ,
            ('l',  11.80, 26.50) ,
            ('l',  23.80, 32.60) ,
            ('s',  self._relief_color) ),
          ( ('m',  12.30, 28.90) ,
            ('l',  5.20, 39.50) ,
            ('l',  9.70, 42.00) ,
            ('l',  21.20, 33.00) ,
            ('l',  12.30, 28.90) ,
            ('s',  self._face_color) ),
        ]
        self._boundaries_table = [
          ( ('m',  11.80, 26.50) ,
            ('l',  2.70, 40.30) ,
            ('l',  9.90, 44.00) ,
            ('l',  23.80, 32.60) ,
            ('l',  11.80, 26.50) ,
            ('p',  0.00, 1.00, 0.00, 1.00, 0.20) ),
        ]

class ControlButtonLeftArmIn(ControlButtonBase):
    def __init__(self, container):
        super(ControlButtonLeftArmIn, self).__init__(container)
        self._press_action_name = 'control-leftarmin-press'
        self._release_action_name = 'control-leftarmin-release'
        self._drawing_table = [
          ( ('m',  39.80, 30.40) ,
            ('l',  39.10, 33.30) ,
            ('l',  21.30, 47.00) ,
            ('l',  17.70, 47.40) ,
            ('l',  39.80, 30.40) ,
            ('s',  self._etch_color) ),
          ( ('m',  17.70, 47.40) ,
            ('l',  21.30, 47.00) ,
            ('l',  33.10, 52.60) ,
            ('l',  33.80, 55.00) ,
            ('l',  17.70, 47.40) ,
            ('s',  self._etch_color) ),
          ( ('m',  33.80, 55.00) ,
            ('l',  33.10, 52.60) ,
            ('l',  41.10, 37.80) ,
            ('l',  43.20, 38.00) ,
            ('l',  33.80, 55.00) ,
            ('s',  self._etch_color) ),
          ( ('m',  43.20, 38.00) ,
            ('l',  41.10, 37.80) ,
            ('l',  39.10, 33.30) ,
            ('l',  39.80, 30.40) ,
            ('l',  43.20, 38.00) ,
            ('s',  self._relief_color) ),
          ( ('m',  39.10, 33.30) ,
            ('l',  21.30, 47.00) ,
            ('l',  33.10, 52.60) ,
            ('l',  41.10, 37.80) ,
            ('l',  39.10, 33.30) ,
            ('s',  self._face_color) ),
        ]
        self._boundaries_table = [
          ( ('m',  39.80, 30.40) ,
            ('l',  17.70, 47.40) ,
            ('l',  33.80, 55.00) ,
            ('l',  43.20, 38.00) ,
            ('l',  39.80, 30.40) ,
            ('p',  1.00, 0.00, 0.00, 0.00, 1.00) ),
        ]

class ControlButtonLeftArmOut(ControlButtonBase):
    def __init__(self, container):
        super(ControlButtonLeftArmOut, self).__init__(container)
        self._press_action_name = 'control-leftarmout-press'
        self._release_action_name = 'control-leftarmout-release'
        self._drawing_table = [
          ( ('m',  11.40, 55.30) ,
            ('l',  11.50, 54.60) ,
            ('l',  4.30, 60.00) ,
            ('l',  0.60, 60.50) ,
            ('l',  11.40, 52.30) ,
            ('s',  self._etch_color) ),
          ( ('m',  0.60, 60.50) ,
            ('l',  4.30, 60.00) ,
            ('l',  23.90, 69.30) ,
            ('l',  24.50, 71.80) ,
            ('l',  0.60, 60.50) ,
            ('s',  self._etch_color) ),
          ( ('m',  24.50, 71.80) ,
            ('l',  23.90, 69.30) ,
            ('l',  27.60, 62.40) ,
            ('l',  30.30, 61.40) ,
            ('l',  24.50, 71.80) ,
            ('s',  self._etch_color) ),
          ( ('m',  30.30, 61.40) ,
            ('l',  27.60, 62.40) ,
            ('l',  11.50, 54.60) ,
            ('l',  11.40, 52.30) ,
            ('l',  30.30, 61.40) ,
            ('s',  self._relief_color) ),
          ( ('m',  11.50, 54.60) ,
            ('l',  4.30, 60.00) ,
            ('l',  23.90, 69.30) ,
            ('l',  27.60, 62.40) ,
            ('l',  11.50, 54.60) ,
            ('s',  self._face_color) ),
        ]
        self._boundaries_table = [
          ( ('m',  11.50, 52.60) ,
            ('l',  0.60, 60.50) ,
            ('l',  24.50, 71.80) ,
            ('l',  30.30, 61.40) ,
            ('l',  11.50, 52.60) ,
            ('p',  0.00, 0.00, 1.00, 1.00, 0.20) ),
        ]

class ControlButtonTiltLeft(ControlButtonBase):
    def __init__(self, container):
        super(ControlButtonTiltLeft, self).__init__(container)
        self._press_action_name = 'control-tiltleft-press'
        self._release_action_name = 'control-tiltleft-release'
        self._drawing_table = [
          ( ('m',  46.60, 43.10) ,
            ('l',  44.80, 50.80) ,
            ('l',  39.10, 62.60) ,
            ('l',  36.40, 64.40) ,
            ('l',  46.60, 43.10) ,
            ('s',  self._etch_color) ),
          ( ('m',  36.40, 64.40) ,
            ('l',  39.10, 62.60) ,
            ('l',  44.80, 62.60) ,
            ('l',  46.60, 64.40) ,
            ('l',  36.40, 64.40) ,
            ('s',  self._etch_color) ),
          ( ('m',  46.60, 64.40) ,
            ('l',  44.80, 62.60) ,
            ('l',  44.80, 50.80) ,
            ('l',  46.60, 43.10) ,
            ('l',  46.60, 64.40) ,
            ('s',  self._relief_color) ),
          ( ('m',  44.80, 50.80) ,
            ('l',  39.10, 62.60) ,
            ('l',  44.80, 62.60) ,
            ('l',  44.80, 50.80) ,
            ('s',  self._face_color) ),
        ]
        self._boundaries_table = [
          ( ('m',  46.60, 43.10) ,
            ('l',  36.40, 64.40) ,
            ('l',  46.60, 64.40) ,
            ('l',  46.60, 43.10) ,
            ('p',  0.00, 1.00, 1.00, 1.00, 0.20) ),
        ]

# --- Worker
class ControlButtonsNew(gtk.DrawingArea):
    """Class draws all control buttons to steer the robot."""
    def __init__(self):
        super(ControlButtonsNew, self).__init__()
        
        self.set_size_request(100, 200)
        self.scale = 1.0
        
        #self._process_default_colors()
        
        self._buttons = [
            ControlButtonUp(self),
            ControlButtonDown(self),
            ControlButtonLeft(self),
            ControlButtonRight(self),
            ControlButtonStop(self),
            ControlButtonRightArmUp(self),
            ControlButtonRightArmDown(self),
            ControlButtonRightArmIn(self),
            ControlButtonRightArmOut(self),
            ControlButtonTiltRight(self),
            ControlButtonLeftArmUp(self),
            ControlButtonLeftArmDown(self),
            ControlButtonLeftArmIn(self),
            ControlButtonLeftArmOut(self),
            ControlButtonTiltLeft(self),
        ]
        self._last_button = None
        
        self.add_events(gtk.gdk.BUTTON_PRESS_MASK | gtk.gdk.BUTTON_RELEASE_MASK)
        
        self._process_dispatcher_additions()
        
        self.connect("expose-event", self.h_area_expose)
        self.connect("button-press-event", self.h_press_event)
        self.connect("button-release-event", self.h_release_event)
    
    def _process_dispatcher_additions(self):
        dispatcher = ActionDispatcher()
        for button in self._buttons:
          dispatcher[button._press_action_name] += button._on_press
          dispatcher[button._press_action_name] += self._dumb_queue_draw
          dispatcher[button._release_action_name] += button._on_release
          dispatcher[button._release_action_name] += self._dumb_queue_draw
    
    def _dumb_queue_draw(self, *args, **kwargs):
        """This allows to avoid problem with dispatching for different number
        of arguments.
        """
        self.queue_draw()       
        
    #def _process_default_colors(self):
    #    self._color_table = {}
    #    #style = self.get_style()
    #    style = gtk.widget_get_default_style()
    #    for state in [gtk.STATE_NORMAL, gtk.STATE_ACTIVE, gtk.STATE_PRELIGHT, gtk.STATE_SELECTED, gtk.STATE_INSENSITIVE]:
    #        
    #        light_color = style.light[state]
    #        dark_color = style.dark[state]
    #        print state, light_color.red, light_color.green, light_color.blue
    #        self._color_table[state] = {
    #            'relief' : (light_color.red/65535.0, light_color.green/65535.0, light_color.blue/65535.0, 1.0),
    #            'etch' : (dark_color.red/65535.0, dark_color.green/65535.0, dark_color.blue/65535.0, 1.0)    
    #        }
        
    def _get_context(self, x, y, w, h):
        context = self.window.cairo_create()

        # set a clip region for the expose event
        context.rectangle(x, y, w, h)

        (x, y, w, h, d) = self.window.get_geometry()
        self.scale = float(min((w, h-100))) / 100.0
        
        if float(w) / float(h) > 0.5:
          self.scale = float(h) / 200.0
          tx = (w / 2.0) - (100.0 * self.scale) / 2.0
          ty = 0.0
        else:
          self.scale = float(w) / 100.0
          tx = 0.0
          ty = (h / 2.0) - (200.0 * self.scale) / 2.0
          
        #print "Debug:", w,h,tx,ty, float(w) / float(h), self.scale
        
        context.translate(tx, ty)
        context.scale(self.scale, self.scale)                           
        context.clip()
        
        return context
    
    def h_press_event(self, widget, event):
        print "Press",event.x, event.y
        (x, y, w, h, d) = self.window.get_geometry()
        #self.context = self._get_context(0, 0, w, h)
        self.context = self._get_context(0, 0, 0, 0)
        for button in self._buttons:
            (inside, edge) = button._test(self.context, event.x, event.y)
            if inside or edge:
                dispatcher = ActionDispatcher()
                self._last_button = button
                dispatcher[button._press_action_name]()
                #button._on_press() # this may need to be called by action manager
                #self.queue_draw()   
        return True
        
    def h_release_event(self, widget, event):
        if self._last_button is not None:
            dispatcher = ActionDispatcher()
            dispatcher[self._last_button._release_action_name]() 
            #self._last_button._on_release()
            #self.queue_draw()
            self._last_button = None
        return True
        
    def h_area_expose(self, area, event):
        self.context = self._get_context(event.area.x, event.area.y,
                           event.area.width, event.area.height)
        self._draw(self.context)

        return False
    
    def _draw(self, context):
        for button in self._buttons:
            button._draw(context)
            #button._draw(context, button._boundaries_table)

# *** Deprecated code ***
##\cond GARBAGE
class ControlButtonsOld(gtk.DrawingArea):
    __drawing_table = [ # m move, l line, b bezier, a arc, n negative arc, s solid pat, g linear gradient, p pen/stroke
      (('m', 77.2, 10.0),
       ('l', 84.0, 20.2),
       ('l', 70.4, 27.8),
       ('l', 60.2, 19.4),
       ('l', 77.2, 10.0),
       ('p', 1.0, 0.0, 0.0, 1.0, 0.2),
       ),
      (('m', 87.4, 27.0),
       ('l', 96.8, 40.6),
       ('l', 91.7, 43.1),
       ('l', 77.2, 32.1),
       ('l', 87.4, 27.0),
       ('p', 1.0, 0.0, 0.0, 1.0, 0.2),
       ),
      (('m', 60.2, 30.4),
       ('l', 82.3, 47.4),
       ('l', 66.2, 55.0),
       ('l', 56.8, 38.0),
       ('l', 60.2, 30.4),
       ('p', 1.0, 0.0, 0.0, 1.0, 0.2),
       ),
      (('m', 53.4, 43.1),
       ('l', 63.6, 64.4),
       ('l', 53.4, 64.4),
       ('l', 53.4, 43.1),
       ('p', 1.0, 0.0, 0.0, 1.0, 0.2),
       ),
      (('m', 88.3, 52.5),
       ('l', 97.6, 60.1),
       ('l', 73.8, 71.2),
       ('l', 69.5, 61.9),
       ('l', 88.3, 52.5),
       ('p', 1.0, 0.0, 0.0, 1.0, 0.2),
       ),
      # mirror
      (('m', 22.8, 10.0),
       ('l', 16.0, 20.2),
       ('l', 29.6, 27.8),
       ('l', 39.8, 19.4),
       ('l', 22.8, 10.0),
       ('p', 1.0, 0.0, 0.0, 1.0, 0.2),
       ),
      (('m', 12.6, 27.0),
       ('l', 3.3, 40.6),
       ('l', 8.4, 43.1),
       ('l', 22.8, 32.1),
       ('l', 12.6, 27.0),
       ('p', 1.0, 0.0, 0.0, 1.0, 0.2),
       ),
      (('m', 39.8, 30.4),
       ('l', 17.7, 47.4),
       ('l', 33.9, 55.0),
       ('l', 43.2, 38.0),
       ('l', 39.8, 30.4),
       ('p', 1.0, 0.0, 0.0, 1.0, 0.2),
       ),
      (('m', 46.6, 43.1),
       ('l', 36.4, 64.4),
       ('l', 46.6, 64.4),
       ('l', 46.6, 43.1),
       ('p', 1.0, 0.0, 0.0, 1.0, 0.2),
       ),
      (('m', 11.8, 52.5),
       ('l', 2.4, 60.1),
       ('l', 26.2, 71.2),
       ('l', 30.4, 61.9),
       ('l', 11.8, 52.5),
       ('p', 1.0, 0.0, 0.0, 1.0, 0.2),
       ),
      # -----------------------------------------------------------------------
      # up button
      (('m', 50, 110),
       ('l', 30, 130),
       ('l', 30+4.366, 130-2.435),
       ('l', 50, 112.3),
       #('b', 40, 25, 60, 25, 70, 30),
       ('l', 50, 110),
       ('s', 0.965, 0.953, 0.897, 1.0)),
      (('m', 50, 110),
       ('l', 70, 130),
       ('b', 60, 125, 40, 125, 30, 130),
       ('l', 30+4.366, 130-2.435),
       ('b', 36+4.366, 126-2.435, 64-4.366, 126-2.435, 70-4.366, 130-2.435),
       #('l', 70-4.366, 30-2.435),
       ('l', 50, 112.3),
       ('l', 50, 110),
       ('s', 0.35, 0.35, 0.35, 1.0)),
      (('m', 50, 112.3),
       ('l', 30+4.366, 130-2.435),
       ('b', 36+4.366, 126-2.435, 64-4.366, 126-2.435, 70-4.366, 130-2.435),
       ('l', 50, 112.3),
       ('s', 1.0, 1.0, 1.0, 1.0)),
      (('m', 50, 116),
       ('l', 50-4.064, 116+5.699),
       ('l', 50+4.064, 116+5.699),
       ('s', 0.95, 0, 0, 1.0)),
      # down button
      (('m', 30, 170),
       ('l', 50, 190),
       ('l', 70, 170),
       ('l', 70-4.366, 170+2.435),
       ('l', 50, 187.7),
       ('l', 30+4.366, 170+2.435),
       ('l', 30, 170),
       ('s', 0.35, 0.35, 0.35, 1.0)),
      (('m', 70, 170),
       ('b', 60, 175, 40, 175, 30, 170),
       ('l', 30+4.366, 170+2.435),
       ('b', 36+4.366, 174+2.435, 64-4.366, 174+2.435, 70-4.366, 170+2.435),
       ('l', 70, 170),
       ('s', 0.965, 0.953, 0.897, 1.0)),
      (('m', 50, 187.7),
       ('l', 30+4.366, 170+2.435),
       ('b', 36+4.366, 174+2.435, 64-4.366, 174+2.435, 70-4.366, 170+2.435),
       ('l', 50, 187.7),
       ('s', 1.0, 1.0, 1.0, 1.0)),
      (('m', 50, 184),
       ('l', 50-4.064, 184-5.699),
       ('l', 50+4.064, 184-5.699),
       ('s', 0.95, 0, 0, 1.0)),
      # left button
      (('m', 10, 150),
       ('l', 30, 130),
       ('l', 30-2.435, 130+4.366,),
       ('l', 12.3, 150),
       ('l', 10, 150),
       ('s', 0.965, 0.953, 0.897, 1.0)),
      (('m', 10, 150),
       ('l', 30, 170), 
       ('b', 25, 160, 25, 140, 30, 130),
       ('l', 30-2.435, 130+4.366),
       ('b', 26-2.435, 136+4.366, 26-2.435, 164-4.366, 30-2.435, 170-4.366),
       ('l', 12.3, 150),
       ('l', 10, 150),
       ('s', 0.35, 0.35, 0.35, 1.0)),
      (('m', 12.3, 150),
       ('l', 30-2.435, 130+4.366),
       ('b', 26-2.435, 136+4.366, 26-2.435, 164-4.366, 30-2.435, 170-4.366),
       ('l', 12.3, 150),
       ('s', 1.0, 1.0, 1.0, 1.0)),
      (('m', 16, 150),
       ('l', 16+5.699, 150-4.064),
       ('l', 16+5.699, 150+4.064),
       ('s', 0.95, 0, 0, 1.0)),
      # right button
      (('m', 70, 130),
       ('l', 90, 150),
       ('l', 70, 170),
       ('l', 70+2.435, 170-4.366),
       ('l', 87.7, 150),
       ('l', 70+2.435, 130+4.366),
       ('l', 70, 130),
       ('s', 0.35, 0.35, 0.35, 1.0)),
      (('m', 70, 170),
       ('b', 75, 160, 75, 140, 70, 130),
       ('l', 70+2.435, 130+4.366),
       ('b', 74+2.435, 136+4.366, 74+2.435, 164-4.366, 70+2.435, 170-4.366),
       ('l', 70, 170),
       ('s', 0.965, 0.953, 0.897, 1.0)),
      (('m', 87.7, 150),
       ('l', 70+2.435, 130+4.366),
       ('b', 74+2.435, 136+4.366, 74+2.435, 164-4.366, 70+2.435, 170-4.366),
       ('l', 87.7, 150),
       ('s', 1.0, 1.0, 1.0, 1.0)),
      (('m', 84, 150),
       ('l', 84-5.699, 150-4.064),
       ('l', 84-5.699, 150+4.064),
       ('s', 0.95, 0, 0, 1.0)),
      # middle button
      (('a', 50, 150, 15, 3*pi/4.0, 7*pi/4.0),
       ('n', 50, 150, 17, 13*pi/8.0, 23*pi/8.0),
       ('s', 0.965, 0.953, 0.897, 1.0)),
      (('a', 50, 150, 15, 7*pi/4.0, 3*pi/4.0),
       ('n', 50, 150, 17, 23*pi/8.0, 13*pi/8.0),
       ('g', 0.4, 0.965, 0.953, 0.897, 0.6, 0.35, 0.35, 0.35)),
      (('a', 50, 150, 15, 0, 6.28),
       ('s', 1.0, 1.0, 1.0, 1.0)),
      (('m', 45, 145),
       ('l', 55, 145),
       ('l', 55, 155),
       ('l', 45, 155),
       ('l', 45, 145), 
       ('s', 0.95, 0, 0, 1.0)),
      (('m', 43, 143),
       ('l', 57, 143),
       ('l', 57, 157),
       ('l', 43, 157),
       ('l', 43, 143),
       ('p', 0.95, 0, 0, 1.0, 0.2)) 
    ]
    def __init__(self):
        super(ControlButtons, self).__init__()
        
        self.set_size_request(100, 200)
        self.scale = 1.0
        self.connect("expose-event", self.h_area_expose)
    
    def _get_context(self):
        context = self.window.cairo_create()

        # set a clip region for the expose event
        context.rectangle(event.area.x, event.area.y,
                           event.area.width, event.area.height)

        (x, y, w, h, d) = self.window.get_geometry()
        self.scale = float(min((w, h-100))) / 100.0
        
        if float(w) / float(h) > 0.5:
          self.scale = float(h) / 200.0
          tx = (w / 2.0) - (100.0 * self.scale) / 2.0
          ty = 0.0
        else:
          self.scale = float(w) / 100.0
          tx = 0.0
          ty = (h / 2.0) - (200.0 * self.scale) / 2.0
          
        #print "Debug:", w,h,tx,ty, float(w) / float(h), self.scale
        
        context.translate(tx, ty)
        context.scale(self.scale, self.scale)                           
        context.clip()
        
        return context
        
    def h_area_expose(self, area, event):
        self.context = self._get_context()
        self._draw(self.context)
        return False
      
    def _draw(self, context):
        context.set_antialias(cairo.ANTIALIAS_DEFAULT)
        
        #pattern = cairo.LinearGradient(10, 10, 90, 90)
        #pattern.add_color_stop_rgb(0.25, 200, 200, 200)
        #pattern.add_color_stop_rgb(0.75, 100, 100, 100)
        
        for element in ControlButtons.__drawing_table:
            x_list = []
            y_list = []
            for command in element:
                if command[0] == 'm':
                    context.move_to(command[1], command[2])
                    x_list.append(command[1])
                    y_list.append(command[2])
                elif command[0] == 'l':
                    context.line_to(command[1], command[2])
                    x_list.append(command[1])
                    y_list.append(command[2])
                elif command[0] == 'b':
                    context.curve_to(command[1], command[2], command[3], command[4], command[5], command[6])
                    x_list.append(command[5])
                    y_list.append(command[6])
                elif command[0] == 'a':
                    context.arc(command[1], command[2], command[3], command[4], command[5])
                    x_list.append(command[1] - command[3])
                    x_list.append(command[1] + command[3])
                    y_list.append(command[2] - command[3])
                    y_list.append(command[2] + command[3])
                elif command[0] == 'n':
                    context.arc_negative(command[1], command[2], command[3], command[4], command[5])
                    x_list.append(command[1] - command[3])
                    x_list.append(command[1] + command[3])
                    y_list.append(command[2] - command[3])
                    y_list.append(command[2] + command[3])
                elif command[0] == 's':
                    pattern = cairo.SolidPattern(command[1], command[2], command[3], command[4])
                    context.set_source(pattern)
                elif command[0] == 'g':
                    pattern = cairo.LinearGradient(min(x_list), min(y_list), max(x_list), max(y_list))
                    pattern.add_color_stop_rgb(command[1], command[2], command[3], command[4])
                    pattern.add_color_stop_rgb(command[5], command[6], command[7], command[8])
                    context.set_source(pattern)
                elif command[0] == 'p':
                    context.set_source_rgba(command[1], command[2], command[3], command[4])
                    context.set_line_width(command[5])
                    context.stroke_preserve()
                    context.set_source_rgba(command[1], command[2], command[3], 0.0)
                else:
                    raise NotImplementedError(_('Command "%s" not implemented in drawing.') % command[0])
            context.close_path()
            #pattern = cairo.LinearGradient(min(x_list), min(y_list), max(x_list), max(y_list))
            #pattern.add_color_stop_rgb(0.25, 100, 100, 100)
            #pattern.add_color_stop_rgb(1, 0, 0, 0)
            context.fill_preserve()
            context.new_path()
        
        #context.move_to(50, 10)
        #context.line_to(30, 30)
        #context.curve_to(40, 25, 60, 25, 70, 30)
        #context.line_to(50, 10)
        #context.close_path()
        
        #context.new_path()
        
        #context.move_to(50, 90)
        #context.line_to(30, 70)
        #context.curve_to(40, 75, 60, 75, 70, 70)
        #context.line_to(50, 90)
        #context.close_path()
        
        #context.set_source(pattern)
        #context.fill_preserve()
##\endcond

# Final binding       
#ControlButtons = ControlButtonsOld
ControlButtons = ControlButtonsNew