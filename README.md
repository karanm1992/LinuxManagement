# LinuxManagement
Python based Linux process and network management and control system

Pre requisites - 
Python 3 with PyGTK library installed.

4 files required to run the program - 

1. screen1.py - 
     This defines the UI in python and the functionalities of traversing the file system.
     The screens can be used to view the currently running processes and network connections.
     Add processes and network connections to block or unblock.

2. syscall.c - 
    This defines the loadable kernel module that will take the list of blocked processes selected from the UI.
     It will block those connections or file accesses by intercepting the system calls and checking the access.
     
3. script.sh / unblock.sh - 
    Shell scripts that are internally called by the python program to load and unload the kernel module from the kernel.
