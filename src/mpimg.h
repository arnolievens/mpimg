/**
 * @author      : Arno Lievens (arnolievens@gmail.com)
 * @created     : 26/06/2021
 * @filename    : mpimg.h
 */


#ifndef MPIMG_H
#define MPIMG_H

#include <mpd/client.h>
#include <stdint.h>
#include <unistd.h>

#define UNUSED(x)               (void)(x)
#define LENGTH(a)               sizeof(a)/sizeof(a[0])

/**
 * operational mode
 */
typedef enum idlemode_t {
    NONE,                       /**< run once */
    IDLE,                       /**< run once but wait for status update */
    IDLELOOP,                   /**< wait for status update and loop forever */
} idlemode_t;

/**
 * program options set by args
 */
typedef struct options_t {
    char *host;                 /**< mpd server address or hostname */
    unsigned int port;          /**< mpd server port */
    int verbose;                /**< print more info to stdout */
    const char* output_file;    /**< write image to this file */
    const char* song;           /**< when provided this image will be fetched */
    idlemode_t mode;            /**< normal, idle or idleloop */
} options_t;


/**
 * print halp!
 */
extern void print_usage(void);

/**
 * print song tags
 *
 * @param[in] song uri
 * @return status 0 for succes, -1 for fail
 */
extern int print_song(const struct mpd_song* song);

/**
 * download albumart for given song
 *
 * caller must free data
 *
 * @global conn mpd server
 * @global options contains program options set by args
 * @param[out] data pointer to image data
 * @param[in[ uri song we want image for
 * @return total size of data or -1 on error
 */
extern ssize_t get_albumart(uint8_t** data, const char* uri);

/**
 * print output to file or stdout
 *
 * @global options contains program options set by args
 * @parm[in] data binary data containing image
 * @param[in] len size of data
 * @return status 0 for succes, -1 for fail
 */
extern int print_output(const uint8_t* data, size_t len);

/**
 * get albumart and write to file
 *
 * run immediately or wait for update before fetching
 * run once or loop forever
 *
 * @global options contains program options set by args
 * @global conn mpd server
 * @return status 0 for succes, -1 for fail
 */
extern int run(void);

#endif

// vim:ft=c
