// doomgeneric for Allegro library

#include "doomkeys.h"
#include "i_system.h"
#include "i_video.h"
#include "m_argv.h"
#include "doomgeneric.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <keyboard.h>
#include <string.h>
#include <framebuffer.h>
#include <vga.h>

static volatile unsigned int s_ticks = 0;

// Framebuffer
static uint8_t *framebuffer;
static int palette_fd = -1;
static vga_color_t palette[VGA_PALETTE_SIZE];

static int framebufferFD = -1;
static unsigned int fbWidth, fbHeight, fbStride, fbBytesPerPixel, fbOffsetX, fbOffsetY;

// Keyboard
int keyboard_fd = -1;

// Systick
int systick_fd = -1;


framebuffer_information_t fb_info;

void DG_Init() {

	printf("Initializing AdOS\n");

	framebufferFD = open_framebuffer();
	get_framebuffer_info(&fb_info);
	if (framebufferFD < 0)
		I_Error("Failed to open framebuffer");

	fbWidth = fb_info.width;
	fbHeight = fb_info.height;
	fbStride = fb_info.pitch;
	fbBytesPerPixel = fb_info.bpp / 8;
	fbOffsetX = 0;
	fbOffsetY = 0;

	framebuffer = mmap(NULL, fb_info.width * fb_info.height * (fb_info.bpp / 8), 0, 0, framebufferFD, 0);
	memset(framebuffer, 10, fbStride * fbHeight);
	
	systick_fd = open("systick", 0);
	if (systick_fd < 0)
	 	I_Error("Failed to open systick");
	
	keyboard_fd = open("keyboard", 0);
	if (keyboard_fd < 0)
	 	I_Error("Failed to open keyboard");
	
	palette_fd = open("palette", 0);
	if (palette_fd < 0)
	 	I_Error("Failed to open palette");
}


void DG_DrawFrame()
{

	for (int line = 0; line < DOOMGENERIC_RESY; line++) {
		memcpy(
			(void *)((uintptr_t)(framebuffer) + (fbStride * line) + fbOffsetY + fbOffsetX),
			(void *)(((uintptr_t)DG_ScreenBuffer) + (DOOMGENERIC_RESX * line * fbBytesPerPixel)), DOOMGENERIC_RESX * fbBytesPerPixel);
	}

#if defined(__i386__)
	if (palette_changed) {
		int i;

		for (i = 0; i < 256; i++) {
			palette[i].red = colors[i].r >> 2;
			palette[i].green = colors[i].g >> 2;
			palette[i].blue = colors[i].b >> 2;
		}
		set_palette(palette, palette_fd);

		palette_changed = false;
	}
#endif
}


void DG_SleepMs(uint32_t ms)
{
	int base_systick;
    if (read(systick_fd, &base_systick, sizeof (base_systick)) == -1)
    {
		I_Error ("Read error on systick_fd");
        return;
    }

    int systick = base_systick;
    while (systick < base_systick + ms)
    {
        if (read (systick_fd, &systick, sizeof (systick)) == -1)
        {
			I_Error ("Read error on systick_fd");
            return;
        }
    }
}


uint32_t DG_GetTicksMs()
{
	uint32_t tick;
	if (read(systick_fd, &tick, sizeof (tick)) == -1)
    {
		I_Error ("Read error on systick_fd");
	}

	return tick;
}

unsigned char kbdus[128] =
{
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8',	/* 9 */
  '9', '0', '-', '=', '\b',	/* Backspace */
  '\t',			/* Tab */
  'q', 'w', 'e', 'r',	/* 19 */
  't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',	/* Enter key */
    0,			/* 29   - Control */
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',	/* 39 */
 '\'', '`',   0,		/* Left shift */
 '\\', 'z', 'x', 'c', 'v', 'b', 'n',			/* 49 */
  'm', ',', '.', '/',   0,				/* Right shift */
  '*',
    0,	/* Alt */
  ' ',	/* Space bar */
    0,	/* Caps lock */
    0,	/* 59 - F1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,	/* < ... F10 */
    0,	/* 69 - Num lock*/
    0,	/* Scroll Lock */
    0,	/* Home key */
    0,	/* Up Arrow */
    0,	/* Page Up */
  '-',
    0,	/* Left Arrow */
    0,
    0,	/* Right Arrow */
  '+',
    0,	/* 79 - End key*/
    0,	/* Down Arrow */
    0,	/* Page Down */
    0,	/* Insert Key */
    0,	/* Delete Key */
    0,   0,   0,
    0,	/* F11 Key */
    0,	/* F12 Key */
    0,	/* All other keys are undefined */
};

int DG_GetKey(int *pressed, unsigned char *doomKey)
{
	int scancode = -1;
	if (read (keyboard_fd, &scancode, sizeof (int)) == -1 || scancode == -1){
		return 0;
	} else {
		switch (scancode & 0x7F)
		{
			case ADOS_KEY_RIGHT:			
				*doomKey = KEY_RIGHTARROW;
				break;
			case ADOS_KEY_LEFT:				
				*doomKey = KEY_LEFTARROW;
				break;
			case ADOS_KEY_UP:				
				*doomKey = KEY_UPARROW;
				break;
			case ADOS_KEY_DOWN:				
				*doomKey = KEY_DOWNARROW;
				break;
			case ADOS_KEY_ALT:
				*doomKey = KEY_LALT;
				break;
			case ADOS_KEY_RSHIFT:
			case ADOS_KEY_LSHIFT:
				*doomKey = KEY_RSHIFT;
				break;
		break;
			// case :				
			// 	*doomKey = KEY_STRAFE_L;
			// 	break;
			// case :				
			// 	*doomKey = KEY_STRAFE_R;
		break;
			case ADOS_KEY_SPACE:				
				*doomKey = KEY_USE;
		break;
			case ADOS_KEY_LCONTROL:
				*doomKey = KEY_FIRE;
				break;
			case ADOS_KEY_ESC:				
				*doomKey = KEY_ESCAPE;
				break;
			case ADOS_KEY_ENTER:				
				*doomKey = KEY_ENTER;
				break;
			case ADOS_KEY_TAB:				
				*doomKey = KEY_TAB;
				break;
			case ADOS_KEY_F1:
				*doomKey = KEY_F1;
				break;
			case ADOS_KEY_F2:
				*doomKey = KEY_F2;
				break;
			case ADOS_KEY_F3:
				*doomKey = KEY_F3;
				break;
			case ADOS_KEY_F4:
				*doomKey = KEY_F4;
				break;
			case ADOS_KEY_F5:
				*doomKey = KEY_F5;
				break;
			case ADOS_KEY_F6:
				*doomKey = KEY_F6;
				break;
			case ADOS_KEY_F7:
				*doomKey = KEY_F7;
				break;
			case ADOS_KEY_F8:
				*doomKey = KEY_F8;
				break;
			case ADOS_KEY_F9:
				*doomKey = KEY_F9;
				break;
			case ADOS_KEY_F10:
				*doomKey = KEY_F10;
				break;
			case ADOS_KEY_F11:
				*doomKey = KEY_F11;
				break;
			case ADOS_KEY_F12:
				*doomKey = KEY_F12;
				break;
			default:
				*doomKey = kbdus[scancode & 0x7F];
				break;
		}
		*pressed = ! (scancode >> 7);
		// printf ("DoomKey: %d", *doomKey);
		// printf ("Pressed: %d", *pressed);
		return 1;
	}

	return 0;
}


void DG_SetWindowTitle(const char *title)
{
}


int main(int argc, char **argv)
{
	printf("Starting doom\n");
	doomgeneric_Create(argc, argv);

	while (true)
	{
		doomgeneric_Tick();
	}

	return 0;
}
