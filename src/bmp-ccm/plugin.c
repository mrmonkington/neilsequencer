// bmp-ccm
// libzzub plugin for beep media player
// Copyright (C) 2006 The Aldrin Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <bmp/plugin.h>
#include <bmp/util.h>
#include <bmp/configfile.h>
#include <bmp/titlestring.h>

#include "../libzzub/libzzub.h"

zzub_player_t *info_player = 0;
zzub_player_t *player = 0;

static void bmp_ccm__init();
static int  bmp_ccm__is_our_file(char *filename);
static void bmp_ccm__play_file(char *filename);
static void bmp_ccm__stop();
static void bmp_ccm__pause(short p);
static void bmp_ccm__seek(int time);
static int  bmp_ccm__get_time();
static void bmp_ccm__cleanup();
static void bmp_ccm__get_song_info(char *filename, char **title, int *length);

InputPlugin ccm_ip =
{
	NULL,
	NULL,
	"bmp-ccm 0.1",
	bmp_ccm__init,
	NULL, // about box
	NULL, // configure
	bmp_ccm__is_our_file,
	NULL,
	bmp_ccm__play_file,
	bmp_ccm__stop,
	bmp_ccm__pause,
	bmp_ccm__seek,
	NULL,
	bmp_ccm__get_time,
	NULL,
	NULL,
	bmp_ccm__cleanup,
	NULL,
	NULL,
	NULL,
	NULL,
	bmp_ccm__get_song_info,
	NULL,
	NULL
};

InputPlugin *get_iplugin_info()
{
	ccm_ip.description = "CCM Plugin"; 
	return &ccm_ip;
}

static void init_player(zzub_player_t *p) {
	zzub_player_add_plugin_path(p, "/usr/local/lib64/zzub/");
	zzub_player_add_plugin_path(p, "/usr/local/lib/zzub/");
	zzub_player_add_plugin_path(p, "/usr/lib64/zzub/");
	zzub_player_add_plugin_path(p, "/usr/lib/zzub/");
	
	zzub_player_initialize(p, 44100);
}

void bmp_ccm__init() {
	player = zzub_player_create();
	init_player(player);

	info_player = zzub_player_create();
	init_player(info_player);
}

static int time_to_pos(zzub_player_t *p, int time) {
	float bpm, tpb;
	bpm = zzub_player_get_bpm(p);
	tpb = zzub_player_get_tpb(p);
	return (int)(((float)(time / 1000) * (bpm * tpb)) / 60.0f + 0.5f);
}

static int pos_to_time(zzub_player_t *p, int pos) {
	float bpm, tpb;
	bpm = zzub_player_get_bpm(p);
	tpb = zzub_player_get_tpb(p);
	return (int)(((float)(pos)*60.0f) / (bpm * tpb) + 0.5f) * 1000;
}

static int get_song_time(zzub_player_t *p) {
	int songend = zzub_player_get_song_end(p);
	if ((songend > 0) && (songend < (1<<24)))
		return pos_to_time(p, songend);
	return pos_to_time(p, zzub_player_get_loop_end(p));
}

int  bmp_ccm__is_our_file(char *filename) {
	char *ext;

	ext = strrchr(filename, '.');
	if (ext && !strcasecmp(ext, ".ccm"))
		return 1;
	return 0;
}

char *ccm_song_title(char *filename) {
	char *title;
	char *basename;
	title = g_malloc(strlen(filename) + 1);
	basename = strrchr(filename,'/');
	if (basename) {
		basename += 1;
	} else {
		basename = strrchr(filename,'\\');
		if (basename) {
			basename += 1;
		} else {
			basename = filename;
		}
	}
	strcpy(title, basename);
	return title;
}

static pthread_t play_thread = 0;
static char *song_title = 0;

static void *play_loop(void *arg) {
	float** workbuffer;
	int numsamples;
	float f;
	float *l; float *r;
	short buffer[256 * 2];
	short *p;
	int i;
	
	while (zzub_player_get_state(player) == zzub_player_state_playing) {
		numsamples = 256;
		workbuffer=zzub_player_work_stereo(player, &numsamples);
		if (numsamples)
		{
			l = workbuffer[0];
			r = workbuffer[1];
			p = &buffer[0];
			for (i = 0; i<numsamples; i++)
			{
				f=*l++;
				if (f>1) f=1.0f;
				if (f<-1) f=-1.0f;
				*p++ = (short)(f * 32767.0f);
				f=*r++;
				if (f>1) f=1.0f;
				if (f<-1) f=-1.0f;
				*p++ = (short)(f * 32767.0f);
			}
			ccm_ip.add_vis_pcm(ccm_ip.output->written_time(), FMT_S16_LE, 2, numsamples * 2 * sizeof(short), buffer);
			while(ccm_ip.output->buffer_free() < (numsamples * 2 * sizeof(short)))
				xmms_usleep(10000);
			ccm_ip.output->write_audio(buffer, numsamples * 2 * sizeof(short));
		} else {
			xmms_usleep(10000);
		}
	}
	ccm_ip.output->buffer_free();
	ccm_ip.output->buffer_free();
	pthread_exit(NULL);
	return 0;
}

void bmp_ccm__play_file(char *filename) {
	zzub_player_set_state(player, zzub_player_state_stopped);
	zzub_player_clear(player);
	zzub_player_load_ccm(player, filename);
	zzub_player_set_state(player, zzub_player_state_playing);
	
	if (ccm_ip.output->open_audio(FMT_S16_LE, 44100, 2)) {
		song_title = ccm_song_title(filename);
		ccm_ip.set_info(song_title, get_song_time(player), 44100 * 2 * 16, 44100, 2);

		pthread_create(&play_thread, NULL, play_loop, NULL);
	}
}
	
void bmp_ccm__stop() {
	zzub_player_set_state(player, zzub_player_state_stopped);
	zzub_player_clear(player);
	if (play_thread) {
		pthread_join(play_thread, NULL);
		play_thread = 0;
		ccm_ip.output->close_audio();
	}
}
	
void bmp_ccm__pause(short p) {
	ccm_ip.output->pause(p);
	//~ if (p) {
		//~ zzub_player_set_state(player, zzub_player_state_stopped);
	//~ } else {
		//~ zzub_player_set_state(player, zzub_player_state_playing);
	//~ }
}
	
void bmp_ccm__seek(int time) {	
	zzub_player_set_position(player, time_to_pos(player, time));
}
	
int  bmp_ccm__get_time() {
	/*if (zzub_player_get_state(player) != zzub_player_state_playing)
		return -1;
	if (!ccm_ip.output->buffer_playing())
		return -1;
	*/
	//pos_to_time(player, zzub_player_get_position(player));
	return ccm_ip.output->output_time();
	//return ccm_ip.output->output_time();
}

void bmp_ccm__cleanup() {
	zzub_audiodriver_destroy(player);
	player = 0;
	zzub_audiodriver_destroy(info_player);
	info_player = 0;
}
	
void bmp_ccm__get_song_info(char *filename, char **title, int *length) {
	char *basename = 0;
	zzub_player_load_ccm(info_player, filename);
	if (title) {
		*title = ccm_song_title(filename);
	}
	if (length) {
		*length = get_song_time(info_player);
	}
	zzub_player_clear(info_player);
}

