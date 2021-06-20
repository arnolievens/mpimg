/**
 * @author      : Arno Lievens (arnolievens@gmail.com)
 * @created     : 20/06/2021
 * @filename    : mptest.c
 */

#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#include <mpd/client.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ENV_HOST    "MPD_HOST"
#define ENV_PORT    "MPD_PORT"

extern char **environ;

/* mpd server properties */
struct {
    char *host;
    unsigned int port;
} server;

/* arg options */
struct option long_options[] = {
    {"host",    required_argument, NULL, 'h'},
    {"port",    required_argument, NULL, 'p'},
    {"quiet",   no_argument,       NULL, 'q'},
    {NULL,      0,                 NULL, 0}
};

struct {
    int quiet;
} options;

int print_song(const struct mpd_song* song)
{
    if (!song) {
        fprintf(stderr, "no song to print\n");
        return -1;
    }

    printf("%s\n%s\n%s\n%s\n%s\n%s\n%s\n",
            mpd_song_get_uri(song),
            mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
            mpd_song_get_tag(song, MPD_TAG_ALBUM, 0),
            mpd_song_get_tag(song, MPD_TAG_DATE, 0),
            mpd_song_get_tag(song, MPD_TAG_DISC, 0),
            mpd_song_get_tag(song, MPD_TAG_TRACK, 0),
            mpd_song_get_tag(song, MPD_TAG_TITLE, 0)
            );

    return 0;
}


/**
 * download albumart for given song
 *
 * caller must free data
 *
 * @param[out] data pointer to image data
 * @param[in] conn mpd connection
 * @param[in[ song song for whom we want image
 * @return total size of data or -1 on error
 */
ssize_t get_albumart(uint8_t** data, struct mpd_connection* conn, const char* uri)
{
    if (!uri) {
        fprintf(stderr, "no song to retrieve album art for\n");
        return -1;
    }

    size_t totlen = 0, len = 0, offset = 0;
    *data = NULL;

    while (!totlen || totlen > offset) {

        offset += len;

        /* send request */
        char offsetstr[100];
        sprintf(offsetstr, "%lu", offset);
        if (!mpd_send_command(conn, "albumart", uri, offsetstr, NULL)) {
            fprintf(stderr, "failed to transmit artwork request: %s\n", strerror(errno));
            return -1;
        }

        /* chunk headers */
        struct mpd_pair *pair;
        pair = mpd_recv_pair_named(conn, "size");
        if (pair) totlen = (size_t)atoi(pair->value);
        mpd_return_pair(conn, pair);

        pair = mpd_recv_pair_named(conn, "binary");
        if (pair) len = (size_t)atoi(pair->value);
        mpd_return_pair(conn, pair);

        /* first run, allocate data */
        if (!*data) {
            *data = malloc(totlen);
            if (!*data) {
                fprintf(stderr, "failed to allocate memory: %s\n", strerror(errno));
                return -1;
            }
        }

        /* request chunk, starting at offset */
        if (!mpd_recv_binary(conn, (*data)+offset, len)) {
            fprintf(stderr, "failed receiving binary: %s\n", strerror(errno));
            return -1;
        }

        mpd_response_finish(conn);
    }

    return (ssize_t)totlen;
}

int fooooooooooooooooooooo(int argc, char **argv)
{
    char* track = NULL;

    /* ENV VAR server host and port */
    while (*environ) {
        if (strstr(*environ, ENV_HOST)) {
            server.host = (*environ)+(strlen(ENV_HOST)+1);
        } else if (strstr(*environ, ENV_PORT)) {
            server.port = (unsigned int)atoi((*environ)+(strlen(ENV_PORT)+1));
        }
        environ++;
    }

    /* parse options */
    int oc;
    int oi = 0;
    while ((oc = getopt_long(argc, argv, "h:p:q", long_options, &oi)) != -1) {
        switch (oc) {
            case 'q': options.quiet = 1; break;
            case 'h': server.host = optarg; break;
            case 'p': server.port = (unsigned int)atoi(optarg); break;
            default: exit(1);
        }
    }

    if (optind < argc) {
        track = argv[optind];
        (void)(track);
    }

    /* connect to server and validate */
    struct mpd_connection *conn = mpd_connection_new(server.host, server.port, 0);
    enum mpd_error mpd_error = mpd_connection_get_error(conn);
    if(mpd_error){
        printf("error code %u\n", mpd_error);
        return 1;
    }

    uint8_t* albumart = NULL;
    struct mpd_song* song = NULL;


    while (true) {
        const char* uri;
        ssize_t len;

        song = mpd_run_current_song(conn);
        uri = mpd_song_get_uri(song);

        len = get_albumart(&albumart, conn, uri);

        if (len <= 0 || !albumart) {
            fprintf(stderr, "failed to fetch albumart\n");
            goto fail;
        }

        /* fwrite(albumart, 1, (size_t)len, stdout); */
        mpd_run_idle_mask(conn, MPD_IDLE_PLAYER);
    }


/* succes */
    if (albumart) free(albumart);
    if (song) mpd_song_free(song);
    if (conn) mpd_connection_free(conn);
    exit(EXIT_SUCCESS);

fail:
    if (albumart) free(albumart);
    if (song) mpd_song_free(song);
    if (conn) mpd_connection_free(conn);
    exit(EXIT_FAILURE);
}
