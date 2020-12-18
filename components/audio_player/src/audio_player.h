/*
 * audio_player.h
 *
 *  Created on: 12.03.2017
 *      Author: michaelboeckling
 */

#ifndef INCLUDE_AUDIO_PLAYER_H_
#define INCLUDE_AUDIO_PLAYER_H_

#include <sys/types.h>

int audio_stream_consumer(const char *recv_buf, ssize_t bytes_read, void *user_data);

typedef enum component_status
{
    UNINITIALIZED,
    INITIALIZED,
    RUNNING,
    STOPPED
} component_status_e;

typedef enum player_command
{
    CMD_NONE,
    CMD_START,
    CMD_STOP
} player_command_e;

typedef enum buffer_pref
{
    BUF_PREF_FAST,
    BUF_PREF_SAFE
} buffer_pref_e;

typedef enum content_type
{
    MIME_UNKNOWN = 1,
    OCTET_STREAM,
    AUDIO_AAC,
    AUDIO_MP4,
    AUDIO_MPEG
} content_type_e;

typedef struct media_stream
{
    content_type_e content_type;
    bool eof;
} media_stream_s;

typedef struct player
{
    player_command_e command;
    player_command_e decoder_command;
    component_status_e decoder_status;
    buffer_pref_e buffer_pref;
    media_stream_s *media_stream;
} player_s;

component_status_e get_player_status();

void audio_player_init(player_s *player_config);
void audio_player_start();
void audio_player_stop();
void audio_player_destroy();

#endif /* INCLUDE_AUDIO_PLAYER_H_ */
