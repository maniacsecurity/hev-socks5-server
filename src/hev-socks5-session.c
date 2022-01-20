/*
 ============================================================================
 Name        : hev-socks5-session.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2017 - 2021 hev
 Description : Socks5 Session
 ============================================================================
 */

#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <hev-memory-allocator.h>

#include "hev-logger.h"
#include "hev-config.h"

#include "hev-socks5-session.h"

HevSocks5Session *
hev_socks5_session_new (int fd)
{
    HevSocks5Session *self;
    int res;

    self = hev_malloc0 (sizeof (HevSocks5Session));
    if (!self)
        return NULL;

    res = hev_socks5_session_construct (self, fd);
    if (res < 0) {
        hev_free (self);
        return NULL;
    }

    LOG_D ("%p socks5 session new", self);

    return self;
}

void
hev_socks5_session_terminate (HevSocks5Session *self)
{
    LOG_D ("%p socks5 session terminate", self);

    hev_socks5_set_timeout (HEV_SOCKS5 (self), 0);
    hev_task_wakeup (self->task);
}

static int
hev_socks5_session_bind (HevSocks5 *self, int sock)
{
    //const char *addr = hev_config_get_bind_address ();
    struct sockaddr_in saddr = { 0 };
    /*int res;

    LOG_D ("%p socks5 session bind", self);

    if (!addr)
        return 0;

    saddr.sin6_family = AF_INET6;
    res = inet_pton (AF_INET6, addr, &saddr.sin6_addr);
    if (res == 0)
        return -1;
    */
    int fd = HEV_SOCKS5 (self)->fd;
    socklen_t len = sizeof (saddr);
    LOG_D ("%p socks5 session bind fd %x", self, fd);
    getpeername(fd, (struct sockaddr *)&saddr, &len);
    char ip[30] = {0};
    inet_ntop(AF_INET, &saddr.sin_addr, ip, sizeof(ip));
    LOG_D ("%p socks5 session bind addr %s", self, ip);
    return bind (sock, (struct sockaddr *)&saddr, sizeof (saddr));
}

int
hev_socks5_session_construct (HevSocks5Session *self, int fd)
{
    int read_write_timeout;
    int connect_timeout;
    const char *user;
    const char *pass;
    int res;

    res = hev_socks5_server_construct (&self->base, fd);
    if (res < 0)
        return -1;

    LOG_D ("%p socks5 session construct", self);

    HEV_OBJECT (self)->klass = HEV_SOCKS5_SESSION_TYPE;

    user = hev_config_get_auth_username ();
    pass = hev_config_get_auth_password ();
    if (user && pass)
        hev_socks5_set_auth_user_pass (HEV_SOCKS5 (&self->base), user, pass);

    connect_timeout = hev_config_get_misc_connect_timeout ();
    read_write_timeout = hev_config_get_misc_read_write_timeout ();

    hev_socks5_set_timeout (HEV_SOCKS5 (self), read_write_timeout);
    hev_socks5_server_set_connect_timeout (&self->base, connect_timeout);

    return 0;
}

static void
hev_socks5_session_destruct (HevObject *base)
{
    HevSocks5Session *self = HEV_SOCKS5_SESSION (base);

    LOG_D ("%p socks5 session destruct", self);

    HEV_SOCKS5_SERVER_TYPE->finalizer (base);
}

HevObjectClass *
hev_socks5_session_class (void)
{
    static HevSocks5SessionClass klass;
    HevSocks5SessionClass *kptr = &klass;
    HevObjectClass *okptr = HEV_OBJECT_CLASS (kptr);

    if (!okptr->name) {
        HevSocks5Class *skptr;

        memcpy (kptr, HEV_SOCKS5_SERVER_TYPE, sizeof (HevSocks5ServerClass));

        okptr->name = "HevSocks5Session";
        okptr->finalizer = hev_socks5_session_destruct;

        skptr = HEV_SOCKS5_CLASS (kptr);
        skptr->binder = hev_socks5_session_bind;
    }

    return okptr;
}
