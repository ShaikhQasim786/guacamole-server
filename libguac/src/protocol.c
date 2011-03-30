
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is libguac.
 *
 * The Initial Developer of the Original Code is
 * Michael Jumper.
 * Portions created by the Initial Developer are Copyright (C) 2010
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#if defined(HAVE_CLOCK_GETTIME) || defined(HAVE_NANOSLEEP)
#include <time.h>
#endif

#ifndef HAVE_CLOCK_GETTIME
#include <sys/time.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <cairo/cairo.h>

#include <sys/types.h>

#ifdef __MINGW32__
#include <winsock2.h>
#else
#include <sys/socket.h>
#endif

#include "guacio.h"
#include "protocol.h"

char* guac_escape_string(const char* str) {

    char* escaped;
    char* current;

    int i;
    int length = 0;

    /* Determine length */
    for (i=0; str[i] != '\0'; i++) {
        switch (str[i]) {

            case ';':
            case ',':
            case '\\':
                length += 2;
                break;

            default:
                length++;

        }
    }

    /* Allocate new */
    escaped = malloc(length+1);

    current = escaped;
    for (i=0; str[i] != '\0'; i++) {
        switch (str[i]) {

            case ';':
                *(current++) = '\\';
                *(current++) = 's';
                break;

            case ',':
                *(current++) = '\\';
                *(current++) = 'c';
                break;

            case '\\':
                *(current++) = '\\';
                *(current++) = '\\';
                break;

            default:
                *(current++) = str[i];
        }

    }

    *current = '\0';

    return escaped;

}

char* guac_unescape_string_inplace(char* str) {

    char* from;
    char* to;

    from = to = str;
    for (;;) {

        char c = *(from++);

        if (c == '\\') {

            c = *(from++);
            if (c == 's')
                *(to++) = ';';

            else if (c == 'c')
                *(to++) = ',';

            else if (c == '\\')
                *(to++) = '\\';

            else if (c == '\0') {
                *(to++) = '\\';
                break;
            }

            else {
                *(to++) = '\\';
                *(to++) = c;
            }
        }

        else if (c == '\0')
            break;

        else
            *(to++) = c;

    }

    *to = '\0';

    return str;

}

int guac_send_args(GUACIO* io, const char** args) {

    int i;

    if (guac_write_string(io, "args:")) return -1;

    for (i=0; args[i] != NULL; i++) {

        char* escaped;

        if (i > 0) {
            if (guac_write_string(io, ","))
                return -1;
        }

        escaped = guac_escape_string(args[i]); 
        if (guac_write_string(io, escaped)) {
            free(escaped);
            return -1;
        }
        free(escaped);

    }

    return guac_write_string(io, ";");

}

int guac_send_name(GUACIO* io, const char* name) {

    char* escaped = guac_escape_string(name); 

    if (
        guac_write_string(io, "name:")
        || guac_write_string(io, escaped)
        || guac_write_string(io, ";")
       ) {
        free(escaped);
        return -1;
    }

    free(escaped);
    return 0;

}

int guac_send_size(GUACIO* io, int w, int h) {

    return
           guac_write_string(io, "size:")
        || guac_write_int(io, w)
        || guac_write_string(io, ",")
        || guac_write_int(io, h)
        || guac_write_string(io, ";");

}

int guac_send_clipboard(GUACIO* io, const char* data) {

    char* escaped = guac_escape_string(data); 

    if (
           guac_write_string(io, "clipboard:")
        || guac_write_string(io, escaped)
        || guac_write_string(io, ";")
       ) {
        free(escaped);
        return -1;
    }

    free(escaped);
    return 0;

}

int guac_send_error(GUACIO* io, const char* error) {

    char* escaped = guac_escape_string(error); 

    if (
           guac_write_string(io, "error:")
        || guac_write_string(io, escaped)
        || guac_write_string(io, ";")
       ) {
        free(escaped);
        return -1;
    }

    free(escaped);
    return 0;

}

int guac_send_sync(GUACIO* io, long timestamp) {

    return 
           guac_write_string(io, "sync:")
        || guac_write_int(io, timestamp)
        || guac_write_string(io, ";");

}

int guac_send_copy(GUACIO* io,
        int srcl, int srcx, int srcy, int w, int h,
        guac_composite_mode_t mode, int dstl, int dstx, int dsty) {

    return
           guac_write_string(io, "copy:")
        || guac_write_int(io, srcl)
        || guac_write_string(io, ",")
        || guac_write_int(io, srcx)
        || guac_write_string(io, ",")
        || guac_write_int(io, srcy)
        || guac_write_string(io, ",")
        || guac_write_int(io, w)
        || guac_write_string(io, ",")
        || guac_write_int(io, h)
        || guac_write_string(io, ",")
        || guac_write_int(io, mode)
        || guac_write_string(io, ",")
        || guac_write_int(io, dstl)
        || guac_write_string(io, ",")
        || guac_write_int(io, dstx)
        || guac_write_string(io, ",")
        || guac_write_int(io, dsty)
        || guac_write_string(io, ";");

}

cairo_status_t __guac_write_png(void* closure, const unsigned char* data, unsigned int length) {

    GUACIO* io = (GUACIO*) closure;

    if (guac_write_base64(io, data, length) < 0)
        return CAIRO_STATUS_WRITE_ERROR;

    return CAIRO_STATUS_SUCCESS;

}

int guac_send_png(GUACIO* io, guac_composite_mode_t mode,
        int layer, int x, int y, cairo_surface_t* surface) {

    /* Write instruction and args */

    if (
           guac_write_string(io, "png:")
        || guac_write_int(io, mode)
        || guac_write_string(io, ",")
        || guac_write_int(io, layer)
        || guac_write_string(io, ",")
        || guac_write_int(io, x)
        || guac_write_string(io, ",")
        || guac_write_int(io, y)
        || guac_write_string(io, ",")
       ) {
        return -1;
    }

    /* Write surface */

    if (cairo_surface_write_to_png_stream(surface, __guac_write_png, io) != CAIRO_STATUS_SUCCESS) {
        return -1;
    }

    if (guac_flush_base64(io) < 0) {
        return -1;
    }

    /* Finish instruction */

    return guac_write_string(io, ";");

}


int guac_send_cursor(GUACIO* io, int x, int y, cairo_surface_t* surface) {

    /* Write instruction and args */

    if (
           guac_write_string(io, "cursor:")
        || guac_write_int(io, x)
        || guac_write_string(io, ",")
        || guac_write_int(io, y)
        || guac_write_string(io, ",")
       ) {
        return -1;
    }

    /* Write surface */

    if (cairo_surface_write_to_png_stream(surface, __guac_write_png, io) != CAIRO_STATUS_SUCCESS) {
        return -1;
    }

    if (guac_flush_base64(io) < 0) {
        return -1;
    }

    /* Finish instruction */

    return guac_write_string(io, ";");

}


int __guac_fill_instructionbuf(GUACIO* io) {

    int retval;
    
    /* Attempt to fill buffer */
    retval = recv(
            io->fd,
            io->instructionbuf + io->instructionbuf_used_length,
            io->instructionbuf_size - io->instructionbuf_used_length,
            0
    );

    if (retval < 0)
        return retval;

    io->instructionbuf_used_length += retval;

    /* Expand buffer if necessary */
    if (io->instructionbuf_used_length > io->instructionbuf_size / 2) {
        io->instructionbuf_size *= 2;
        io->instructionbuf = realloc(io->instructionbuf, io->instructionbuf_size);
    }

    return retval;

}

/* Returns new instruction if one exists, or NULL if no more instructions. */
int guac_read_instruction(GUACIO* io, guac_instruction* parsed_instruction) {

    int retval;
    int i = 0;
    int argcount = 0;
    int j;
    int current_arg = 0;
    
    /* Loop until a instruction is read */
    for (;;) {

        /* Search for end of instruction */
        for (; i < io->instructionbuf_used_length; i++) {

            /* Count arguments as we look for the end */
            if (io->instructionbuf[i] == ',')
                argcount++;
            else if (io->instructionbuf[i] == ':' && argcount == 0)
                argcount++;

            /* End found ... */
            else if (io->instructionbuf[i] == ';') {

                /* Parse new instruction */
                char* instruction = malloc(i+1);
                memcpy(instruction, io->instructionbuf, i+1);
                instruction[i] = '\0'; /* Replace semicolon with null terminator. */

                parsed_instruction->opcode = NULL;

                parsed_instruction->argc = argcount;
                parsed_instruction->argv = malloc(sizeof(char*) * argcount);

                for (j=0; j<i; j++) {

                    /* If encountered a colon, and no opcode parsed yet, set opcode and following argument */
                    if (instruction[j] == ':' && parsed_instruction->opcode == NULL) {
                        instruction[j] = '\0';
                        parsed_instruction->argv[current_arg++] = &(instruction[j+1]);
                        parsed_instruction->opcode = instruction;
                    }

                    /* If encountered a comma, set following argument */
                    else if (instruction[j] == ',') {
                        instruction[j] = '\0';
                        parsed_instruction->argv[current_arg++] = &(instruction[j+1]);
                    }
                }

                /* If no arguments, set opcode to entire instruction */
                if (parsed_instruction->opcode == NULL)
                    parsed_instruction->opcode = instruction;

                /* Found. Reset buffer */
                memmove(io->instructionbuf, io->instructionbuf + i + 1, io->instructionbuf_used_length - i - 1);
                io->instructionbuf_used_length -= i + 1;

                /* Done */
                return 1;
            }

        }

        /* No instruction yet? Get more data ... */
        retval = guac_select(io, GUAC_USEC_TIMEOUT);
        if (retval <= 0)
            return retval;

        /* If more data is available, fill into buffer */
        retval = __guac_fill_instructionbuf(io);
        if (retval < 0)  return retval; /* Error */
        if (retval == 0) return -1;     /* EOF */

    }

}

void guac_free_instruction_data(guac_instruction* instruction) {
    free(instruction->opcode);

    if (instruction->argv)
        free(instruction->argv);
}

void guac_free_instruction(guac_instruction* instruction) {
    guac_free_instruction_data(instruction);
    free(instruction);
}


int guac_instructions_waiting(GUACIO* io) {

    if (io->instructionbuf_used_length > 0)
        return 1;

    return guac_select(io, GUAC_USEC_TIMEOUT);
}

long guac_current_timestamp() {

#ifdef HAVE_CLOCK_GETTIME

    struct timespec current;

    /* Get current time */
    clock_gettime(CLOCK_REALTIME, &current);
    
    /* Calculate milliseconds */
    return current.tv_sec * 1000 + current.tv_nsec / 1000000;

#else

    struct timeval current;

    /* Get current time */
    gettimeofday(&current, NULL);
    
    /* Calculate milliseconds */
    return current.tv_sec * 1000 + current.tv_usec / 1000;

#endif

}

void guac_sleep(int millis) {

#ifdef HAVE_NANOSLEEP 
        struct timespec sleep_period;

        sleep_period.tv_sec = 0;
        sleep_period.tv_nsec = millis * 1000000L;

        nanosleep(&sleep_period, NULL);
#elif defined(__MINGW32__)
        Sleep(millis)
#else
#warning No sleep/nanosleep function available. Clients may not perform as expected. Consider patching libguac to add support for your platform.
#endif

}
