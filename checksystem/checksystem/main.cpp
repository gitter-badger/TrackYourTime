/*
 * TrackYourTime - cross-platform time tracker
 * Copyright (C) 2015-2016  Alexander Basov <basovav@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QCoreApplication>
#include <QDir>
#include <QTextStream>
#include <fcntl.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>


enum eActiveWindowState{
    ACTIVE_WINDOW_OK,
    CANT_OPEN_X_DYSPLAY,
    CANT_GET_NET_ACTIVE_WINDOW,
    CANT_GET_WINDOW_CLASS
};

eActiveWindowState checkActiveWindow(){
    Display *display	= XOpenDisplay(NULL);
    if (!display)
        return CANT_OPEN_X_DYSPLAY;

    Window root 	= DefaultRootWindow(display);
    Atom active 	= XInternAtom(display, "_NET_ACTIVE_WINDOW", False);

    XTextProperty text;
    char **name = NULL;
    int param;

    Atom type_ret;
    int format_ret;
    unsigned long items_ret;
    unsigned long after_ret;
    unsigned char *prop_data = 0;

    if(XGetWindowProperty(display, root, active, 0, 0x7fffffff, False, XA_WINDOW,
    &type_ret, &format_ret, &items_ret, &after_ret, &prop_data) == Success)
    {
        Window *active_window = (Window *)prop_data;
        Atom wm_state = XInternAtom(display, "WM_CLASS", False);
        XGetTextProperty(display, *active_window, &text, wm_state);
	XCloseDisplay(display);
        if (text.value==NULL)
            return CANT_GET_WINDOW_CLASS;
        return ACTIVE_WINDOW_OK;
    }
    else{
	XCloseDisplay(display);
        return CANT_GET_NET_ACTIVE_WINDOW;
	}
}

enum eInputState{
    UDEV_OK,
    UDEV_NOTFOUND,
    UDEV_KEYBOARD_NOT_FOUND,
    UDEV_NOACCESS
};

eInputState checkUdev(){
    QDir dir("/dev/input/by-id");
    if (!dir.exists())
        return UDEV_NOTFOUND;

    QStringList keyboards = dir.entryList(QStringList() << "*keyboard*");
    if (keyboards.size()==0)
        return UDEV_KEYBOARD_NOT_FOUND;

    int fd = open(("/dev/input/by-id/"+keyboards[0]).toUtf8().constData(), 0);
    if (fd==-1)
        return UDEV_NOACCESS;
    close(fd);

    return UDEV_OK;
}

QTextStream& qStdOut()
{
    static QTextStream ts( stdout );
    return ts;
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    eActiveWindowState av_state = checkActiveWindow();
    eInputState udev_state = checkUdev();

    switch(av_state){
    case ACTIVE_WINDOW_OK:{
        qStdOut() << "X Window System - OK" << '\n';
    }
        break;
    case CANT_OPEN_X_DYSPLAY:{
        qStdOut() << "X Window System - FAILED" << '\n';
        qStdOut() << "XOpenDisplay - FAILED" << '\n';
        qStdOut() << "How to fix: run X Window System(e.g startx)" << '\n';
    }
        break;
    case CANT_GET_NET_ACTIVE_WINDOW:{
        qStdOut() << "X Window System - FAILED" << '\n';
        qStdOut() << "_NET_ACTIVE_WINDOW property not available" << '\n';
        qStdOut() << "How to fix: no way to fix it" << '\n';
    }
        break;
    case CANT_GET_WINDOW_CLASS:{
        qStdOut() << "X Window System - FAILED" << '\n';
        qStdOut() << "WM_CLASS property not available" << '\n';
        qStdOut() << "How to fix: no way to fix it" << '\n';
    }
        break;
    }
    if (av_state!=ACTIVE_WINDOW_OK)
        qStdOut() << "X Window System failure is critical. Before you fix it - time tracker can't work" << '\n';

    switch(udev_state){
    case UDEV_OK:{
        qStdOut() << "udev - OK" << '\n';
    }
        break;
    case UDEV_NOTFOUND:{
        qStdOut() << "udev - FAILED" << '\n';
        qStdOut() << "/dev/input/by-id not found" << '\n';
        qStdOut() << "How to fix: install udev" << '\n';
    }
        break;
    case UDEV_KEYBOARD_NOT_FOUND:{
        qStdOut() << "udev - FAILED" << '\n';
        qStdOut() << "/dev/input/by-id/*keyboard* not found" << '\n';
        qStdOut() << "keyboard not found" << '\n';
        qStdOut() << "How to fix: plug keyboard" << '\n';
    }
        break;
    case UDEV_NOACCESS:{
        qStdOut() << "udev - FAILED" << '\n';
        qStdOut() << "/dev/input/by-id/*keyboard* no access" << '\n';
        qStdOut() << "How to fix: under root open udev rules file(create if not exists) '85-pure-data.rules' typical location is /etc/udev/rules.d/ and add string 'SUBSYSTEM==\"input\", MODE=\"666\", GROUP=\"input\"'" << '\n';
    }
        break;
    }
    if (udev_state!=UDEV_OK)
        qStdOut() << "udev failure is non critical. Time tracker can work without fix this failure, but sleep mode will not work." << '\n';

    return 0;
}
