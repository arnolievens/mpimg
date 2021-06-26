/*******************************************************************************
 * @author      : Arno Lievens (arnolievens@gmail.com)
 * @created     : 20/06/2021
 * @filename    : mpimg.c
 ******************************************************************************/

#include <errno.h>
#include <getopt.h>
#include <mpd/client.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mpimg.h"

#define ENV_HOST                "MPD_HOST"
#define ENV_PORT                "MPD_PORT"
#define ENV_ALBUMART            "MPD_ALBUMART"


/*******************************************************************************
 *                                  global
 ******************************************************************************/

/**
 * environment variables
 */
extern char **environ;

/**
 * connection to mpd server
 */
static struct mpd_connection *conn;

static options_t options;

/**
 * getopts options
 */
static const char* short_options = "H,p:,o:,s:,v,h";
static const struct option long_options[] = {
    {"host",    required_argument, NULL, 'H'},
    {"port",    required_argument, NULL, 'p'},
    {"output",  required_argument, NULL, 'o'},
    {"song",    required_argument, NULL, 's'},
    {"verbose", no_argument,       NULL, 'v'},
    {"help",    no_argument,       NULL, 'h'},
    {NULL,      0,                 NULL, 0}
};


/*******************************************************************************
 *                                functions
 ******************************************************************************/


void print_usage(void)
{
    const char* usage[] = {
        "usage: mpimg [options]",
        "",
        "   fetch mpd albumartwork and save to file ('-' for stdout)",
        "   mpd server settings are read from environment variables:",
        "       MPD_PORT",
        "       MPD_HOST",
        "",
        "   output filename can be set by:",
        "       MPD_ALBUMART",
        "",
        "   by default, mpimg fetches the albumart of the current song",
        "   you can download any cover file (-s) by providing the song URI",
        "   command-line options override environment variables",
        "",
        "options:",
        "   -H, --host          mpd host",
        "   -p, --port          mpd port",
        "   -o, --output        output file",
        "   -s, --song          song URI",
        "   -h, --help          this menu",
        "",
        "other commands:",
        "   idle:",
        "       wait until mpd player event occurs",
        "   idleloop:",
        "       simalar to idle but re-enters \"idle\" state and loops forever",
        "",
    };

    for (size_t i = 0; i < LENGTH(usage); i++)
        printf ("%s\n", usage[i]);
}

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

ssize_t get_albumart(uint8_t** data, const char* uri)
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
            fprintf(stderr, "failed to transmit artwork request: %s\n",
                    strerror(errno));
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
                fprintf(stderr, "failed to allocate memory: %s\n",
                        strerror(errno));
                return -1;
            }
        }

        /* request chunk, starting at offset */
        if (!mpd_recv_binary(conn, (*data)+offset, len)) {
            fprintf(stderr, "failed receiving binary: %s\n",
                    strerror(errno));
            return -1;
        }

        mpd_response_finish(conn);
    }

    return (ssize_t)totlen;
}

int print_output(const uint8_t* data, size_t len)
{
    FILE *fd;

    /* '-' indicates stdout */
    if ((strcmp(options.output_file, "-") == 0)) {
        fd = stdout;

    /* regular output file */
    } else {
        fd = fopen(options.output_file, "w");
        if (!fd) {
            fprintf(stderr, "error opening file \"%s\": %s\n",
                    options.output_file, strerror(errno));
            return -1;
        }
    }

    fwrite(data, 1, (size_t)len, fd);
    fclose(fd);
    return 0;
}

int run(void)
{
    ssize_t len = 0;
    uint8_t* albumart = NULL;
    struct mpd_song* song = NULL;
    const char* uri;

    /* idle loop */
    do {
        /* IDLE : wait for status update */
        if (options.verbose) printf("waiting for mpd server...\n");
        if (options.mode == IDLE || options.mode == IDLELOOP) {
            mpd_run_idle_mask(conn, MPD_IDLE_PLAYER);
        }

        /* song provided in arg */
        if (options.song) {
            uri = options.song;

        /* use current song */
        } else {
            song = mpd_run_current_song(conn);
            if (!song) {
                /* failed to get current song, try again next time */
                fprintf(stderr, "error requesting current song: %s\n",
                    strerror(errno));
                continue;
            }
            uri = mpd_song_get_uri(song);
        }

        /* fetch albumart */
        if (options.verbose) printf("fetching cover.jpg for \"%s\"\n", uri);
        len = get_albumart(&albumart, uri);

        if (len <= 0) {
            fprintf(stderr, "error retrieving albumart: %s\n",
                    strerror(errno));
            return -1;
        }

        if (print_output(albumart, (size_t)len) < 0) {
            fprintf(stderr, "error printing to file: %s\n",
                    strerror(errno));
            return -1;
        }

        /* cleanup */
        if (albumart) {
            free(albumart);
            albumart = NULL;
        }
        if (song) {
            free(song);
            song = NULL;
        }

    } while (options.mode == IDLELOOP);

    return 0;
}


/*******************************************************************************
 *                                  main
 ******************************************************************************/


int main(int argc, char **argv)
{
    /* ENV vars */
    while (*environ) {
        if (strstr(*environ, ENV_HOST)) {
            options.host = (*environ)+(strlen(ENV_HOST)+1);

        } else if (strstr(*environ, ENV_PORT)) {
            options.port = (unsigned int)atoi((*environ)+(strlen(ENV_PORT)+1));

        } else if (strstr(*environ, ENV_ALBUMART)) {
            options.output_file = (*environ)+(strlen(ENV_ALBUMART)+1);
        }

        environ++;
    }

    /* parse options */
    int oc;
    int oi = 0;
    while ((oc = getopt_long( argc, argv,
                    short_options, long_options, &oi)) != -1) {
        switch (oc) {
            case 'H': options.host = optarg; break;
            case 'p': options.port = (unsigned int)atoi(optarg); break;
            case 'o': options.output_file = optarg; break;
            case 's': options.song = optarg; break;
            case 'v': options.verbose = true; break;
            case 'h': print_usage(); exit(EXIT_SUCCESS);
            default: exit(EXIT_FAILURE);
        }
    }

    /* parse other commands */
    while (optind < argc) {

        if ((strcmp(argv[optind], "idle") == 0)) {
            options.mode = IDLE;

        } else if ((strcmp(argv[optind], "idleloop") == 0)) {
            options.mode = IDLELOOP;

        } else {
            fprintf(stderr, "invalid command: \"%s\"\n", argv[optind]);
            exit(EXIT_FAILURE);
        }

        optind++;
    }

    /* print options */
    if (options.verbose) {

        printf("host:   %s\n", options.host);

        printf("port:   %u\n", options.port);

        if (options.output_file)
            printf("output: %s\n", options.output_file);

        if (options.song)
            printf("song:   %s\n", options.song);

        switch (options.mode) {
            case IDLE: printf("mode:   idle\n"); break;
            case IDLELOOP: printf("mode:   idleloop\n"); break;
            case NONE:
            default: break;
        }
    }

    /* validate server settings */
    if (!options.host) {
        fprintf(stderr, "invalid host setting: \"%s\"\n", options.host);
        exit(EXIT_FAILURE);
    }
    if (!options.port) {
        fprintf(stderr, "invalid port setting: \"%u\"\n", options.port);
        exit(EXIT_FAILURE);
    }

    /* validate output file" */
    if (!options.output_file) {
        /* options.output_file = "cover.jpg"; */
        fprintf(stderr, "please provide output file setting\n");
        exit(EXIT_FAILURE);
    }

    /* connect to server */
    conn = mpd_connection_new(options.host, options.port, 0);
    enum mpd_error err = mpd_connection_get_error(conn);
    if (err) {
        printf("error code %u\n", err);
        exit(EXIT_FAILURE);
    }

    /* run */
    if (run() < 0) {
        if (conn) mpd_connection_free(conn);
        exit(EXIT_FAILURE);
    }

    if (conn) mpd_connection_free(conn);
    exit(EXIT_SUCCESS);
}
