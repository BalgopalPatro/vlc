/*****************************************************************************
 * vlc_input.h: Core input structures
 *****************************************************************************
 * Copyright (C) 1999-2006 the VideoLAN team
 * $Id$
 *
 * Authors: Christophe Massiot <massiot@via.ecp.fr>
 *          Laurent Aimar <fenrir@via.ecp.fr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#if !defined( __LIBVLC__ )
  #error You are not libvlc or one of its plugins. You cannot include this file
#endif

/* __ is need because conflict with <vlc/input.h> */
#ifndef _VLC__INPUT_H
#define _VLC__INPUT_H 1

#include <vlc_es.h>
#include <vlc_meta.h>
#include <vlc_epg.h>

struct vlc_meta_t;

/*****************************************************************************
 * input_item_t: Describes an input and is used to spawn input_thread_t objects
 *****************************************************************************/
struct info_t
{
    char *psz_name;            /**< Name of this info */
    char *psz_value;           /**< Value of the info */
};

struct info_category_t
{
    char   *psz_name;      /**< Name of this category */
    int    i_infos;        /**< Number of infos in the category */
    struct info_t **pp_infos;     /**< Pointer to an array of infos */
};

struct input_item_t
{
    VLC_GC_MEMBERS
    int        i_id;                 /**< Identifier of the item */

    char       *psz_name;            /**< text describing this item */
    char       *psz_uri;             /**< mrl of this item */
    vlc_bool_t  b_fixed_name;        /**< Can the interface change the name ?*/

    int        i_options;            /**< Number of input options */
    char       **ppsz_options;       /**< Array of input options */

    mtime_t    i_duration;           /**< Duration in milliseconds*/

    uint8_t    i_type;               /**< Type (file, disc, ...) */
    vlc_bool_t b_prefers_tree;      /**< Do we prefer being displayed as tree*/

    int        i_categories;         /**< Number of info categories */
    info_category_t **pp_categories; /**< Pointer to the first info category */

    int         i_es;                /**< Number of es format descriptions */
    es_format_t **es;                /**< Es formats */

    input_stats_t *p_stats;          /**< Statistics */
    int           i_nb_played;       /**< Number of times played */

    vlc_meta_t *p_meta;

    vlc_mutex_t lock;                /**< Lock for the item */
};

#define ITEM_TYPE_UNKNOWN       0
#define ITEM_TYPE_AFILE         1
#define ITEM_TYPE_VFILE         2
#define ITEM_TYPE_DIRECTORY     3
#define ITEM_TYPE_DISC          4
#define ITEM_TYPE_CDDA          5
#define ITEM_TYPE_CARD          6
#define ITEM_TYPE_NET           7
#define ITEM_TYPE_PLAYLIST      8
#define ITEM_TYPE_NODE          9
#define ITEM_TYPE_NUMBER        10

static inline void input_ItemInit( vlc_object_t *p_o, input_item_t *p_i )
{
    memset( p_i, 0, sizeof(input_item_t) );
    p_i->psz_name = NULL;
    p_i->psz_uri = NULL;
    TAB_INIT( p_i->i_es, p_i->es );
    TAB_INIT( p_i->i_options, p_i->ppsz_options );
    TAB_INIT( p_i->i_categories, p_i->pp_categories );

    p_i->i_type = ITEM_TYPE_UNKNOWN;
    p_i->b_fixed_name = VLC_TRUE;

    p_i->p_stats = NULL;
    p_i->p_meta = NULL;

    vlc_mutex_init( p_o, &p_i->lock );
}

static inline void input_ItemCopyOptions( input_item_t *p_parent,
                                          input_item_t *p_child )
{
    int i;
    for( i = 0 ; i< p_parent->i_options; i++ )
    {
        char *psz_option= strdup( p_parent->ppsz_options[i] );
        p_child->i_options++;
        p_child->ppsz_options = (char **)realloc( p_child->ppsz_options,
                                                  p_child->i_options *
                                                  sizeof( char * ) );
        p_child->ppsz_options[p_child->i_options-1] = psz_option;
    }
}

VLC_EXPORT( void, input_ItemAddOption,( input_item_t *, const char * ) );
VLC_EXPORT( void, input_ItemAddOptionNoDup,( input_item_t *, const char * ) );

static inline void input_ItemClean( input_item_t *p_i )
{
    int i;

    free( p_i->psz_name );
    free( p_i->psz_uri );
    if( p_i->p_stats )
    {
        vlc_mutex_destroy( &p_i->p_stats->lock );
        free( p_i->p_stats );
    }

    if( p_i->p_meta )
        vlc_meta_Delete( p_i->p_meta );

    for( i = 0; i < p_i->i_options; i++ )
    {
        if( p_i->ppsz_options[i] )
            free( p_i->ppsz_options[i] );
    }
    TAB_CLEAN( p_i->i_options, p_i->ppsz_options );

    for( i = 0; i < p_i->i_es; i++ )
    {
        es_format_Clean( p_i->es[i] );
        free( p_i->es[i] );
    }
    TAB_CLEAN( p_i->i_es, p_i->es );

    for( i = 0; i < p_i->i_categories; i++ )
    {
        info_category_t *p_category = p_i->pp_categories[i];
        int j;

        for( j = 0; j < p_category->i_infos; j++ )
        {
            struct info_t *p_info = p_category->pp_infos[j];

            if( p_info->psz_name )
                free( p_info->psz_name);
            if( p_info->psz_value )
                free( p_info->psz_value );
            free( p_info );
        }
        TAB_CLEAN( p_category->i_infos, p_category->pp_infos );

        if( p_category->psz_name ) free( p_category->psz_name );
        free( p_category );
    }
    TAB_CLEAN( p_i->i_categories, p_i->pp_categories );

    vlc_mutex_destroy( &p_i->lock );
}

VLC_EXPORT( char *, input_ItemGetInfo, ( input_item_t *p_i, const char *psz_cat,const char *psz_name ) );
VLC_EXPORT(int, input_ItemAddInfo, ( input_item_t *p_i, const char *psz_cat, const char *psz_name, const char *psz_format, ... ) );

#define input_ItemNew( a,b,c ) input_ItemNewExt( a, b, c, 0, NULL, -1 )
#define input_ItemNewExt(a,b,c,d,e,f) __input_ItemNewExt( VLC_OBJECT(a),b,c,d,e,f)
VLC_EXPORT( input_item_t *, __input_ItemNewExt, (vlc_object_t *, const char *, const char*, int, const char *const *, mtime_t i_duration )  );
VLC_EXPORT( input_item_t *, input_ItemNewWithType, ( vlc_object_t *, const char *, const char *e, int, const char *const *, mtime_t i_duration, int ) );

VLC_EXPORT( input_item_t *, input_ItemGetById, (playlist_t *, int ) );

/*****************************************************************************
 * Seek point: (generalisation of chapters)
 *****************************************************************************/
struct seekpoint_t
{
    int64_t i_byte_offset;
    int64_t i_time_offset;
    char    *psz_name;
    int     i_level;
};

static inline seekpoint_t *vlc_seekpoint_New( void )
{
    seekpoint_t *point = (seekpoint_t*)malloc( sizeof( seekpoint_t ) );
    point->i_byte_offset =
    point->i_time_offset = -1;
    point->i_level = 0;
    point->psz_name = NULL;
    return point;
}

static inline void vlc_seekpoint_Delete( seekpoint_t *point )
{
    if( !point ) return;
    if( point->psz_name ) free( point->psz_name );
    free( point );
}

static inline seekpoint_t *vlc_seekpoint_Duplicate( seekpoint_t *src )
{
    seekpoint_t *point = vlc_seekpoint_New();
    if( src->psz_name ) point->psz_name = strdup( src->psz_name );
    point->i_time_offset = src->i_time_offset;
    point->i_byte_offset = src->i_byte_offset;
    return point;
}

/*****************************************************************************
 * Title:
 *****************************************************************************/
typedef struct
{
    char        *psz_name;

    vlc_bool_t  b_menu;      /* Is it a menu or a normal entry */

    int64_t     i_length;   /* Length(microsecond) if known, else 0 */
    int64_t     i_size;     /* Size (bytes) if known, else 0 */

    /* Title seekpoint */
    int         i_seekpoint;
    seekpoint_t **seekpoint;

} input_title_t;

static inline input_title_t *vlc_input_title_New(void)
{
    input_title_t *t = (input_title_t*)malloc( sizeof( input_title_t ) );

    t->psz_name = NULL;
    t->b_menu = VLC_FALSE;
    t->i_length = 0;
    t->i_size   = 0;
    t->i_seekpoint = 0;
    t->seekpoint = NULL;

    return t;
}

static inline void vlc_input_title_Delete( input_title_t *t )
{
    int i;
    if( t == NULL )
        return;

    if( t->psz_name ) free( t->psz_name );
    for( i = 0; i < t->i_seekpoint; i++ )
    {
        if( t->seekpoint[i]->psz_name ) free( t->seekpoint[i]->psz_name );
        free( t->seekpoint[i] );
    }
    if( t->seekpoint ) free( t->seekpoint );
    free( t );
}

static inline input_title_t *vlc_input_title_Duplicate( input_title_t *t )
{
    input_title_t *dup = vlc_input_title_New( );
    int i;

    if( t->psz_name ) dup->psz_name = strdup( t->psz_name );
    dup->b_menu      = t->b_menu;
    dup->i_length    = t->i_length;
    dup->i_size      = t->i_size;
    dup->i_seekpoint = t->i_seekpoint;
    if( t->i_seekpoint > 0 )
    {
        dup->seekpoint = (seekpoint_t**)calloc( t->i_seekpoint,
                                                sizeof(seekpoint_t*) );

        for( i = 0; i < t->i_seekpoint; i++ )
        {
            dup->seekpoint[i] = vlc_seekpoint_Duplicate( t->seekpoint[i] );
        }
    }

    return dup;
}
/*****************************************************************************
 * Attachments
 *****************************************************************************/
struct input_attachment_t
{
    char *psz_name;
    char *psz_mime;
    char *psz_description;

    int  i_data;
    void *p_data;
};
static inline input_attachment_t *vlc_input_attachment_New( const char *psz_name,
                                                            const char *psz_mime,
                                                            const char *psz_description,
                                                            const void *p_data,
                                                            int i_data )
{
    input_attachment_t *a =
        (input_attachment_t*)malloc( sizeof(input_attachment_t) );
    if( !a )
        return NULL;
    a->psz_name = strdup( psz_name ? psz_name : "" );
    a->psz_mime = strdup( psz_mime ? psz_mime : "" );
    a->psz_description = strdup( psz_description ? psz_description : "" );
    a->i_data = i_data;
    a->p_data = NULL;
    if( i_data > 0 )
    {
        a->p_data = malloc( i_data );
        if( a->p_data && p_data )
            memcpy( a->p_data, p_data, i_data );
    }
    return a;
}
static inline input_attachment_t *vlc_input_attachment_Duplicate( const input_attachment_t *a )
{
    return vlc_input_attachment_New( a->psz_name, a->psz_mime, a->psz_description,
                                     a->p_data, a->i_data );
}
static inline void vlc_input_attachment_Delete( input_attachment_t *a )
{
    if( !a )
        return;
    free( a->psz_name );
    free( a->psz_mime );
    free( a->psz_description );
    if( a->p_data )
        free( a->p_data );
    free( a );
}
/*****************************************************************************
 * input defines/constants.
 *****************************************************************************/

/* "state" value */
enum input_state_e
{
    INIT_S,
    OPENING_S,
    BUFFERING_S,
    PLAYING_S,
    PAUSE_S,
    END_S,
    ERROR_S
};

/* "rate" default, min/max
 * A rate below 1000 plays the movie faster,
 * A rate above 1000 plays the movie slower.
 */
#define INPUT_RATE_DEFAULT  1000
#define INPUT_RATE_MIN       125            /* Up to 8/1 */
#define INPUT_RATE_MAX     32000            /* Up to 1/32 */

/* i_update field of access_t/demux_t */
#define INPUT_UPDATE_NONE       0x0000
#define INPUT_UPDATE_SIZE       0x0001
#define INPUT_UPDATE_TITLE      0x0010
#define INPUT_UPDATE_SEEKPOINT  0x0020
#define INPUT_UPDATE_META       0x0040

/* Input control XXX: internal */
#define INPUT_CONTROL_FIFO_SIZE    100

/** Get the input item for an input thread */
VLC_EXPORT(input_item_t*, input_GetItem, (input_thread_t*));

typedef struct input_thread_private_t input_thread_private_t;

/**
 * Main structure representing an input thread. This structure is mostly
 * private. The only public fields are READ-ONLY. You must use the helpers
 * to modify them
 */
struct input_thread_t
{
    VLC_COMMON_MEMBERS;

    vlc_bool_t  b_eof;
    vlc_bool_t b_preparsing;

    int i_state;
    vlc_bool_t b_can_pace_control;
    int64_t     i_time;     /* Current time */

    /* Internal caching common to all inputs */
    int i_pts_delay;

    /* All other data is input_thread is PRIVATE. You can't access it
     * outside of src/input */
    input_thread_private_t *p;
};

/*****************************************************************************
 * Prototypes
 *****************************************************************************/
#define input_CreateThread(a,b) __input_CreateThread(VLC_OBJECT(a),b)
VLC_EXPORT( input_thread_t *, __input_CreateThread, ( vlc_object_t *, input_item_t * ) );
#define input_Preparse(a,b) __input_Preparse(VLC_OBJECT(a),b)
VLC_EXPORT( int, __input_Preparse, ( vlc_object_t *, input_item_t * ) );

#define input_Read(a,b,c) __input_Read(VLC_OBJECT(a),b, c)
VLC_EXPORT( int, __input_Read, ( vlc_object_t *, input_item_t *, vlc_bool_t ) );
VLC_EXPORT( void,             input_StopThread,     ( input_thread_t * ) );
VLC_EXPORT( void,             input_DestroyThread,  ( input_thread_t * ) );

enum input_query_e
{
    /* input variable "position" */
    INPUT_GET_POSITION,         /* arg1= double *       res=    */
    INPUT_SET_POSITION,         /* arg1= double         res=can fail    */

    /* input variable "length" */
    INPUT_GET_LENGTH,           /* arg1= int64_t *      res=can fail    */

    /* input variable "time" */
    INPUT_GET_TIME,             /* arg1= int64_t *      res=    */
    INPUT_SET_TIME,             /* arg1= int64_t        res=can fail    */

    /* input variable "rate" (1 is DEFAULT_RATE) */
    INPUT_GET_RATE,             /* arg1= int *          res=    */
    INPUT_SET_RATE,             /* arg1= int            res=can fail    */

    /* input variable "state" */
    INPUT_GET_STATE,            /* arg1= int *          res=    */
    INPUT_SET_STATE,            /* arg1= int            res=can fail    */

    /* input variable "audio-delay" and "sub-delay" */
    INPUT_GET_AUDIO_DELAY,      /* arg1 = int* res=can fail */
    INPUT_SET_AUDIO_DELAY,      /* arg1 = int  res=can fail */
    INPUT_GET_SPU_DELAY,        /* arg1 = int* res=can fail */
    INPUT_SET_SPU_DELAY,        /* arg1 = int  res=can fail */

    /* Meta datas */
    INPUT_ADD_INFO,   /* arg1= char* arg2= char* arg3=...     res=can fail */
    INPUT_GET_INFO,   /* arg1= char* arg2= char* arg3= char** res=can fail */
    INPUT_DEL_INFO,   /* arg1= char* arg2= char*              res=can fail */
    INPUT_SET_NAME,   /* arg1= char* res=can fail    */

    /* Input config options */
    INPUT_ADD_OPTION,      /* arg1= char * arg2= char *  res=can fail*/

    /* Input properties */
    INPUT_GET_BYTE_POSITION,     /* arg1= int64_t *       res=    */
    INPUT_SET_BYTE_SIZE,         /* arg1= int64_t *       res=    */

    /* bookmarks */
    INPUT_GET_BOOKMARKS,   /* arg1= seekpoint_t *** arg2= int * res=can fail */
    INPUT_CLEAR_BOOKMARKS, /* res=can fail */
    INPUT_ADD_BOOKMARK,    /* arg1= seekpoint_t *  res=can fail   */
    INPUT_CHANGE_BOOKMARK, /* arg1= seekpoint_t * arg2= int * res=can fail   */
    INPUT_DEL_BOOKMARK,    /* arg1= seekpoint_t *  res=can fail   */
    INPUT_SET_BOOKMARK,    /* arg1= int  res=can fail    */

    /* Attachments */
    INPUT_GET_ATTACHMENTS, /* arg1=input_attachment_t***, arg2=int*  res=can fail */
    INPUT_GET_ATTACHMENT,  /* arg1=input_attachment_t**, arg2=char*  res=can fail */

    /* On the fly input slave */
    INPUT_ADD_SLAVE        /* arg1= char * */
};

VLC_EXPORT( int, input_vaControl,( input_thread_t *, int i_query, va_list  ) );
VLC_EXPORT( int, input_Control,  ( input_thread_t *, int i_query, ...  ) );

VLC_EXPORT( decoder_t *, input_DecoderNew, ( input_thread_t *, es_format_t *, vlc_bool_t b_force_decoder ) );
VLC_EXPORT( void, input_DecoderDelete, ( decoder_t * ) );
VLC_EXPORT( void, input_DecoderDecode,( decoder_t *, block_t * ) );

VLC_EXPORT( vlc_bool_t, input_AddSubtitles, ( input_thread_t *, char *, vlc_bool_t ) );

#endif
