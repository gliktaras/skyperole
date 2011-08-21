skyperole
=========
This small application monitors the opened windows and sets window roles of Skype windows. I wrote it primarily to get xmonad recognize Skype in its layout hook with XMonad.Layout.IM module.


Building and Installing
-----------------------
Build requirements: X11 libraries (i.e. xorg), GNU make, gcc.

Build procedure:
  
    git clone git@github.com:gliktaras/skyperole.git skyperole
    cd skyperole
    make

No additional installation is necessary.


Usage
-----
To use skyperole, simply have it running in the background.


Set Window Rules
----------------
skyperole makes the following window role assignments:

Skype window                 | Window role
---------------------------- | -----------------
Add contact dialog           | skype-add-contact
Add to chat dialog           | skype-add-to-chat
Chat window                  | skype-chat       
Options                      | skype-options    
Sign in window               | skype-sign-in    
Start conference call dialog | skype-conf-call  
The contact list             | skype-main       
User profile                 | skype-profile    


Skype Version Compatibility
---------------------------
Since it uses substring matching as a method of recognizing windows, it will not work with non-english versions of Skype and it is not guaranteed to work with any versions other than Linux Beta 2.2.0.35.


Licensing
---------
BSD 2-clause License (aka Simplified BSD).
