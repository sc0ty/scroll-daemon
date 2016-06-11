/*
Copyright (c) 2016, Mike Szymaniak
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <linux/input.h>
#include <linux/uinput.h>


#define MOUSE_BTN_LEFT		0x01
#define MOUSE_BTN_RIGHT		0x02
#define MOUSE_BTN_MIDDLE	0x04


struct mouse_event
{
	unsigned char btn;
	char x, y;
};


static void die(const char *msg);
static void usage(const char *name);

static void output_init(int fd);
static void output_scroll(int fd, __u16 axis, int val);
static void output_key(int fd, __u16 keycode, int pressed);
static void output_syn(int fd);


int main(int argc, const char* argv[])
{
	struct input_event ev;
	int dx = 0;
	int dy = 0;
	char last_btn = 0;
	char btn_map = 0;

	const char *outfile = "/dev/uinput";

	if (argc < 4)
		usage(argv[0]);

	int i;
	for (i = 4; i < argc; i++)
	{
		if (argv[i][0] == '-')
		{
			const char *p;
			for (p = argv[i] + 1; *p; p++)
			{
				switch (*p)
				{
					case 'l': btn_map |= MOUSE_BTN_LEFT; break;
					case 'r': btn_map |= MOUSE_BTN_RIGHT; break;
					case 'm': btn_map |= MOUSE_BTN_MIDDLE; break;
					default: usage(argv[0]);
				}
			}
		}
		else
			outfile = argv[4];
	}

	int vspeed = atoi(argv[2]);
	int hspeed = atoi(argv[3]);

	int ifd = open(argv[1], O_RDONLY);
	if (ifd  == -1)
		die("input device open error");

	int ofd = open(outfile, O_WRONLY | O_NONBLOCK);
	if (ofd  == -1)
		die("output device open error");

	output_init(ofd);

	while (read(ifd, &ev, sizeof(struct input_event)))
	{
		struct mouse_event *mouse = (struct mouse_event*)&ev;
		dx += mouse->x;
		dy += mouse->y;
		int syn = 0;

		//printf("btn: %x, pos: %d, %d\n", mouse->btn, mouse->x, mouse->y);

#define HANDLE_BTN(b, ob) \
		if ((btn_map & b) && (!(mouse->btn & b) ^ !(last_btn & b))) \
		{ \
			output_key(ofd, ob, mouse->btn & b); \
			syn = 1; \
		}

		HANDLE_BTN(MOUSE_BTN_LEFT, BTN_LEFT)
		HANDLE_BTN(MOUSE_BTN_RIGHT, BTN_RIGHT)
		HANDLE_BTN(MOUSE_BTN_MIDDLE, BTN_MIDDLE)

#undef HANDLE_BTN

		if (vspeed && (dy / vspeed))
		{
			output_scroll(ofd, REL_WHEEL, dy/vspeed);
			dy %= vspeed;
			syn = 1;
		}
		if (hspeed && (dx / hspeed))
		{
			output_scroll(ofd, REL_HWHEEL, dx/hspeed);
			dx %= hspeed;
			syn = 1;
		}

		if (syn)
		{
			output_syn(ofd);
			last_btn = mouse->btn;
			dx = dy = 0;
		}
	}

	return 0;
}

void output_init(int fd)
{
	ioctl(fd, UI_SET_EVBIT, EV_KEY);
	ioctl(fd, UI_SET_EVBIT, EV_REL);
	ioctl(fd, UI_SET_EVBIT, EV_SYN);

	ioctl(fd, UI_SET_RELBIT, REL_X);
	ioctl(fd, UI_SET_RELBIT, REL_Y);
	ioctl(fd, UI_SET_RELBIT, REL_WHEEL);
	ioctl(fd, UI_SET_RELBIT, REL_HWHEEL);
	
	ioctl(fd, UI_SET_KEYBIT, BTN_LEFT);
	ioctl(fd, UI_SET_KEYBIT, BTN_RIGHT);
	ioctl(fd, UI_SET_KEYBIT, BTN_MIDDLE);
	

	struct uinput_user_dev uidev;
	memset(&uidev, 0, sizeof(uidev));
	snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "psmouse-scroll");
	uidev.id.bustype = BUS_USB;
	uidev.id.vendor  = 0x01;	// Generic Desktop Controls
	uidev.id.product = 0x01;	// Pointer
	uidev.id.version = 1;

	if(write(fd, &uidev, sizeof(uidev)) < 0)
		die("output device configuration error");

	if(ioctl(fd, UI_DEV_CREATE) < 0)
		die("output device creation error");
}

void output_scroll(int fd, __u16 axis, int val)
{
	struct input_event ev;
	memset(&ev, 0, sizeof(struct input_event));
	ev.type = EV_REL;
	ev.code = axis;
	ev.value = val;

	if (write(fd, &ev, sizeof(struct input_event)) < 0)
		die("output device write error");
}

void output_key(int fd, __u16 keycode, int pressed)
{
	struct input_event ev;
	memset(&ev, 0, sizeof(struct input_event));
	ev.type = EV_KEY;
	ev.code = keycode;
	ev.value = pressed ? 1 : 0;

	if (write(fd, &ev, sizeof(struct input_event)) < 0)
		die("output device write error");
}

void output_syn(int fd)
{
	struct input_event ev;
	memset(&ev, 0, sizeof(struct input_event));
	ev.type = EV_SYN;
	ev.code = SYN_REPORT;
	ev.value = 0;

	if(write(fd, &ev, sizeof(struct input_event)) < 0)
		die("output device write error");
}

void die(const char *msg)
{
	perror(msg);
	exit(EXIT_FAILURE);
}

void usage(const char *name)
{
	printf("Usage:  %s INPUT VSPEED HSPEED -lrm [OUTPUT]\n"
			"    INPUT  - input file, eg. \"/dev/input/mouse0\"\n"
			"    VSPEED - vertical scrolling speed\n"
			"    HSPEED - horizontal scrolling speed\n"
			"    OUTPUT - output file, if not provided \"/dev/uinput\" is used\n"
			"    -l     - pass left mouse button\n"
			"    -r     - pass right mouse button\n"
			"    -m     - pass middle mouse button\n"
			"Speeds are integer values, use zero to disable or negative value "
			"to reverse scroll direction. Smaller value means faster scroll.\n"
			"Parameters must be passed in order defined above.\n",
			name);
	exit(EXIT_SUCCESS);
}

