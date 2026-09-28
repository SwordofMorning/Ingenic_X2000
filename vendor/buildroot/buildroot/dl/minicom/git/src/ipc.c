/*
 * ipc.c	talk to the keyserv process.
 *
 *		Entry point:
 *
 *		keyserv(command, arg)
 *		command can be KINSTALL, KUNINSTALL, or a command for keyserv.
 *		arg is a 1 byte argument.
 *
 *		This file is part of the minicom communications package,
 *		Copyright 1991-1995 Miquel van Smoorenburg.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <strings.h>

#include "port.h"
#include "minicom.h"
#include "intl.h"

int read_buf(int fd, char *buf, int bufsize)
{
  int i = read(fd, buf, bufsize - 1);

#ifdef USE_SOCKET
  if (i < 1 && portfd_is_socket && portfd == fd) {
    term_socket_close();
    i = 0;
  }
#endif /* USE_SOCKET */

  buf[i > 0 ? i : 0] = 0;

  return i;
}

/* Check if there is IO pending. */
static int check_io(int fd1, int fd2, int tmout, char *buf,
                    int bufsize, int *bytes_read)
{
  int n = 0, i;
  struct timeval tv;
  fd_set fds;

  tv.tv_sec = tmout / 1000;
  tv.tv_usec = (tmout % 1000) * 1000L;

  i = fd1;
  if (fd2 > fd1)
    i = fd2;

  FD_ZERO(&fds);
  if (fd1 >= 0)
    FD_SET(fd1, &fds);
  else
    fd1 = 0;
  if (fd2 >= 0)
    FD_SET(fd2, &fds);
  else
    fd2 = 0;

  if (fd2 == 0 && io_pending)
    n = 2;
  else if (select(i + 1, &fds, NULL, NULL, &tv) > 0)
    n = 1 * (FD_ISSET(fd1, &fds) > 0) + 2 * (FD_ISSET(fd2, &fds) > 0);

  /* If there is data put it in the buffer. */
  if (buf) {
    if ((n & 1) == 1)
      i = read_buf(fd1, buf, bufsize);
    else
      i = 0;

    if (bytes_read)
      *bytes_read = i;
  }

  return n;
}

int check_io_frontend(char *buf, int buf_size, int *bytes_read)
{
  return check_io(portfd_connected(), 0, 1000, buf, buf_size, bytes_read);
}

bool check_io_input(int timeout_ms)
{
  return check_io(-1, 0, timeout_ms, NULL, 0, NULL) & 2;
}

int keyboard(int cmd, int arg)
{
  switch (cmd) {
    case KSTART:
    case KSTOP:
      break;
    case KSIGIO:
      break;
    case KGETKEY:
      return wxgetch();
    case KSETESC:
      escape = arg;
      break;
    case KSETBS:
      vt_set(-1, -1, -1, arg, -1, -1, -1, -1, -1);
      break;
    case KCURST:
      vt_set(-1, -1, -1, -1, -1, NORMAL, -1, -1, -1);
      break;
    case KCURAPP:
      vt_set(-1, -1, -1, -1, -1, APPL, -1, -1, -1);
      break;
    default:
      /* The rest is only meaningful if a keyserv runs. */
      break;
  }
  return 0;
}
