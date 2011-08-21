// Copyright (c) 2011, Gediminas Liktaras
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer. 
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.


#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include <cstdlib>
#include <ctime>


//--- CONSTANTS ----------------------------------------------------------------

/** Positive infinity. */
const int INFINITY = 0x7fffffff;

/** Sleep time of the program's event loop in nanoseconds (10^-9). */
const int SLEEP_TIME_NS = 1000000;


/** Class of windows, belonging to Skype. */
const std::string SKYPE_WINDOW_CLASS = "Skype";

/** Number of window identification records. */
const int SKYPE_WINDOW_TYPE_COUNT = 8;

/** Skype window identification information (name substring, window role). */
const std::string SKYPE_WINDOW_TYPES[SKYPE_WINDOW_TYPE_COUNT][2] = {
    { "Add a Skype Contact",         "skype-add-contact" },
    { "Add to Chat",                 "skype-add-to-chat" },
    { "Skype™ Chat",                 "skype-chat"        },
    { "Start conference call",       "skype-conf-call"   },
    { "Skype™ (Beta)",               "skype-main"        },
    { "Options",                     "skype-options"     },
    { "Profile for",                 "skype-profile"     },
    { "Skype™ 2.2 (Beta) for Linux", "skype-sign-in"     }
};


//--- MAIN VARIABLES -----------------------------------------------------------

/** Connection to the X server. */
Display *display;


/** WM_CLASS atom. */
Atom WM_CLASS_ATOM;

/** _NET_WM_NAME atom. */
Atom WM_NAME_ATOM;

/** WM_WINDOW_ROLE atom. */
Atom WM_WINDOW_ROLE_ATOM;


/** Whether to print verbose messages. */
bool verbose = false;


//--- UTILITY FUNCTIONS --------------------------------------------------------

/** Checks whether string str1 is contained within str2. */
bool isSubstring(const std::string &str1, const std::string &str2) {
    return (std::search(str2.begin(), str2.end(),
                        str1.begin(), str1.end()) != str2.end());
}


/** Returns the value of the given property of the given window as strings. */
std::vector<std::string> getStringProperty(Window window, Atom property) {
    Atom actualType;
    int actualFormat;
    unsigned long itemCount;
    unsigned long bytesLeft;
    char *data;

    if (XGetWindowProperty(display, window, property, 0, INFINITY, False,
            AnyPropertyType, &actualType, &actualFormat, &itemCount, &bytesLeft,
            reinterpret_cast<unsigned char**>(&data)) == Success) {
        
        if (itemCount > 0) {
            std::vector<std::string> propertyStrings;
            propertyStrings.push_back(data);

            for (size_t i = 0; i < itemCount - 1; ++i) {
                if (data[i] == 0) {
                    propertyStrings.push_back(data + i + 1);
                }
            }    
            return propertyStrings;
        }
    }

    return std::vector<std::string>();
}

/** Sets the given property of the given window to the specified string value. */
void setStringProperty(Window window, Atom property, std::string value) {
    const char *rawValue = value.c_str();
    XChangeProperty(display, window, property, XA_STRING, 8, PropModeReplace,
            reinterpret_cast<const unsigned char*>(rawValue), value.size());
}


//--- WINDOW ROLE SETTER -------------------------------------------------------

/** Check if the given window is a Skype window. */
bool isSkypeWindow(Window window) {
    std::vector<std::string> classNames = getStringProperty(window, WM_CLASS_ATOM);
    for (size_t i = 0; i < classNames.size(); ++i) {
        if (classNames[i] == SKYPE_WINDOW_CLASS) {
            return true;
        }
    }
    return false;
}

/** Set the window role of the given window, if it is recognized. */
void setWindowRole(Window window) {
    if (!isSkypeWindow(window)) {
        return;
    }

    std::vector<std::string> windowNames = getStringProperty(window, WM_NAME_ATOM);
    for (size_t i = 0; i < windowNames.size(); ++i) {
        for (int j = 0; j < SKYPE_WINDOW_TYPE_COUNT; ++j) {
            if (isSubstring(SKYPE_WINDOW_TYPES[j][0], windowNames[i])) {
                setStringProperty(window, WM_WINDOW_ROLE_ATOM, SKYPE_WINDOW_TYPES[j][1]);
                return;
            }
        }
    }

    if (verbose) {
        std::cerr << "Recognized Skype window " << std::hex << window << ", "
                  << "could not identify it further." << std::endl;
    }
}


//--- INITIALIZATION -----------------------------------------------------------

/** Parse command line arguments. */
void parseArgs(int /*argc*/, char ** /*argv*/) {
    // TODO.
}

/** Initialize the program. */
void initialize(int argc, char **argv) {
    parseArgs(argc, argv);

    display = XOpenDisplay(NULL);
    if (!display) {
        std::cerr << "Could not open a connection to the X server." << std::endl;
        exit(EXIT_FAILURE);
    }

    WM_CLASS_ATOM = XInternAtom(display, "WM_CLASS", false);
    WM_NAME_ATOM = XInternAtom(display, "_NET_WM_NAME", false);
    WM_WINDOW_ROLE_ATOM = XInternAtom(display, "WM_WINDOW_ROLE", false);

    int screenCount = XScreenCount(display);
    for (int i = 0; i < screenCount; ++i) {
        Window rootWindow = XRootWindow(display, i);
        XSelectInput(display, rootWindow, SubstructureNotifyMask);
    }
}


//--- WINDOW MONITORING --------------------------------------------------------

/** Check the currently open windows for Skype windows, set their window roles as needed. */
void scanWindowTree() {
    Window rootReturn;
    Window parentReturn;
    Window *children;
    unsigned int childCount;

    int screenCount = XScreenCount(display);
    for (int i = 0; i < screenCount; ++i) {
        Window rootWindow = XRootWindow(display, i);
        XQueryTree(display, rootWindow, &rootReturn, &parentReturn, &children, &childCount);

        for (unsigned int j = 0; j < childCount; ++j) {
            setWindowRole(children[j]);
        }
        XFree(children);
    }
}

/** Monitor the newly created windows, set their window roles as needed. */
void eventLoop() {
    timespec sleep_timespec = { 0, SLEEP_TIME_NS };
    XEvent event;

    while (true) {
        while (XPending(display)) {
            XNextEvent(display, &event);

            switch (event.type) {
            case CreateNotify :
                setWindowRole(event.xcreatewindow.window);
                break;

            default :
                break;
            }
        }

        nanosleep(&sleep_timespec, NULL);
    }
}


//--- ENTRY POINT --------------------------------------------------------------

/** The entry point. */
int main(int argc, char **argv) {
    initialize(argc, argv);
    scanWindowTree();
    eventLoop();

    return EXIT_SUCCESS;
}
