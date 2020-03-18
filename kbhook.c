/*
 * Copyright © 2009 Red Hat, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#include <X11/Xlib.h>
#include <X11/extensions/XInput.h>
#include <X11/extensions/XInput2.h>
#include <X11/Xutil.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

int xi_opcode;

Bool is_pointer(int use)
{
    return use == XIMasterPointer || use == XISlavePointer;
}

Bool is_keyboard(int use)
{
    return use == XIMasterKeyboard || use == XISlaveKeyboard;
}

Bool device_matches(XIDeviceInfo *info, char *name)
{
    if (strcmp(info->name, name) == 0)
    {
        return True;
    }

    if (strncmp(name, "pointer:", strlen("pointer:")) == 0 &&
        strcmp(info->name, name + strlen("pointer:")) == 0 &&
        is_pointer(info->use))
    {
        return True;
    }

    if (strncmp(name, "keyboard:", strlen("keyboard:")) == 0 &&
        strcmp(info->name, name + strlen("keyboard:")) == 0 &&
        is_keyboard(info->use))
    {
        return True;
    }

    return False;
}

XIDeviceInfo* xi2_find_device_info(Display *display, char *name)
{
    XIDeviceInfo *info;
    XIDeviceInfo *found = NULL;
    int ndevices;
    Bool is_id = True;
    int i, id = -1;

    for (i = 0; i < strlen(name); i++)
    {
        if (!isdigit(name[i]))
        {
            is_id = False;
            break;
        }
    }

    if (is_id)
    {
        id = atoi(name);
    }

    info = XIQueryDevice(display, XIAllDevices, &ndevices);
    for (i = 0; i < ndevices; i++)
    {
        if (is_id ? info[i].deviceid == id : device_matches(&info[i], name))
        {
            if (found)
            {
                fprintf(stderr,
                        "Warning: There are multiple devices matching '%s'.\n"
                        "To ensure the correct one is selected, please use "
                        "the device ID, or prefix the\ndevice name with "
                        "'pointer:' or 'keyboard:' as appropriate.\n\n",
                        name);
                XIFreeDeviceInfo(info);
                return NULL;
            }
            else
            {
                found = &info[i];
            }
        }
    }

    return found;
}

int test_xi2(Display *display, int argc, char *argv[])
{
    XIEventMask mask[2];
    XIEventMask *m;
    Window win;
    int deviceid = -1;
    int rc;

    setvbuf(stdout, NULL, _IOLBF, 0);

    win = DefaultRootWindow(display);

    if (argc >= 1)
    {
        XIDeviceInfo *info;
        info = xi2_find_device_info(display, argv[0]);
        deviceid = info->deviceid;
    }

    /* Select for motion events */
    m = &mask[0];
    m->deviceid = (deviceid == -1) ? XIAllDevices : deviceid;
    m->mask_len = XIMaskLen(XI_LASTEVENT);
    m->mask = calloc(m->mask_len, sizeof(char));

    m = &mask[1];
    m->deviceid = (deviceid == -1) ? XIAllMasterDevices : deviceid;
    m->mask_len = XIMaskLen(XI_LASTEVENT);
    m->mask = calloc(m->mask_len, sizeof(char));
    XISetMask(m->mask, XI_RawKeyPress);
    XISetMask(m->mask, XI_RawKeyRelease);
    // XISetMask(m->mask, XI_RawButtonPress);
    // XISetMask(m->mask, XI_RawButtonRelease);

    XISelectEvents(display, win, &mask[0], 2);
    XSync(display, False);

    free(mask[0].mask);
    free(mask[1].mask);

    int ctr_alt = 0;
    int ctr_ctrl = 0;
    int ctr_shift = 0;
    int ctr_super = 0;
    int ctr_other = 0;
    int can_alt_shift = 0;
    int can_ctrl_shift = 0;
    int can_super_shift = 0;

    while(1)
    {
        XEvent ev;
        XGenericEventCookie *cookie = (XGenericEventCookie*)&ev.xcookie;
        XNextEvent(display, (XEvent*)&ev);

        if (XGetEventData(display, cookie) &&
            cookie->type == GenericEvent &&
            cookie->extension == xi_opcode)
        {
            XIRawEvent *event = cookie->data;
            switch (cookie->evtype)
            {
                case XI_RawKeyPress:
                {
                    if (event->detail == 64)
                    {
                        ctr_alt++;
                        can_alt_shift |= ctr_shift && !ctr_ctrl && !ctr_super && !ctr_other;
                        can_ctrl_shift = 0;
                        can_super_shift = 0;
                        break;
                    }

                    if (event->detail == 37 || event->detail == 105)
                    {
                        ctr_ctrl++;
                        can_alt_shift = 0;
                        can_ctrl_shift |= ctr_shift && !ctr_alt && !ctr_super && !ctr_other;
                        can_super_shift = 0;
                        break;
                    }

                    if (event->detail == 50 || event->detail == 62)
                    {
                        ctr_shift++;
                        can_alt_shift |= ctr_alt && !ctr_ctrl && !ctr_super && !ctr_other;
                        can_ctrl_shift |= ctr_ctrl && !ctr_alt && !ctr_super && !ctr_other;
                        can_super_shift |= ctr_super && !ctr_ctrl && !ctr_alt && !ctr_other;
                        break;
                    }

                    if (event->detail == 133 || event->detail == 134)
                    {
                        ctr_super++;
                        can_alt_shift = 0;
                        can_ctrl_shift = 0;
                        can_super_shift |= ctr_shift && !ctr_ctrl && !ctr_alt && !ctr_other;
                        break;
                    }

                    // Implement Ctrl+Alt+T
                    if (event->detail == 28 && ctr_ctrl && ctr_alt && !ctr_shift && !ctr_super && !ctr_other)
                    {
                        system("./terminal.sh");
                    }

                    ctr_other++;
                    can_alt_shift = 0;
                    can_ctrl_shift = 0;
                    can_super_shift = 0;
                    break;
                }
                case XI_RawKeyRelease:
                {
                    if (event->detail == 64)
                    {
                        ctr_alt -= ctr_alt > 0;

                        // Implement Alt+Shift (on Alt release)
                        if (ctr_shift && !ctr_alt && can_alt_shift && !ctr_ctrl && !ctr_super && !ctr_other)
                        {
                            system("./layout_rotate_cjk.sh");
                        }

                        break;
                    }

                    if (event->detail == 37 || event->detail == 105)
                    {
                        ctr_ctrl -= ctr_ctrl > 0;

                        // Implement Ctrl+Shift (on Ctrl release)
                        if (ctr_shift && !ctr_ctrl && can_ctrl_shift && !ctr_alt && !ctr_super && !ctr_other)
                        {
                            system("./layout_rotate.sh");
                        }

                        break;
                    }

                    if (event->detail == 50 || event->detail == 62)
                    {
                        ctr_shift -= ctr_shift > 0;

                        // Implement Alt+Shift (on Shift release)
                        if (ctr_alt && !ctr_shift && can_alt_shift && !ctr_ctrl && !ctr_super && !ctr_other)
                        {
                            system("./layout_rotate_cjk.sh");
                        }

                        // Implement Ctrl+Shift (on Shift release)
                        if (ctr_ctrl && !ctr_shift && can_ctrl_shift && !ctr_alt && !ctr_super && !ctr_other)
                        {
                            system("./layout_rotate.sh");
                        }

                        break;
                    }

                    if (event->detail == 133 || event->detail == 134)
                    {
                        ctr_super -= ctr_super > 0;
                        break;
                    }

                    ctr_other -= ctr_other > 0;
                    break;
                }
                case XI_RawButtonPress:
                {
                    break;
                }
                case XI_RawButtonRelease:
                {
                    break;
                }
            }
        }

        XFreeEventData(display, cookie);
    }

    XDestroyWindow(display, win);

    return EXIT_SUCCESS;
}

int main(int argc, char* argv[])
{
    Display *display = XOpenDisplay(NULL);
    int event, error;

    if (display == NULL)
    {
        fprintf(stderr, "Unable to connect to X server.\n");
        XCloseDisplay(display);
        return EXIT_FAILURE;
    }

    if (!XQueryExtension(display, "XInputExtension", &xi_opcode, &event, &error))
    {
        fprintf(stderr, "X Input extension not available.\n");
        XCloseDisplay(display);
        return EXIT_FAILURE;
    }

    int r = test_xi2(display, argc - 1, argv + 1);
    XSync(display, False);
    XCloseDisplay(display);
    return r;
}
