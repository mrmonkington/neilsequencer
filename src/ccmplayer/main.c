/*

example ccm/bmx player using the C interface

*/

#include <stdio.h>

#if defined(_WIN32)
#include <conio.h>
#elif defined(POSIX)

#include <sys/select.h>

#endif // defined(POSIX)

#include "../libzzub/libzzub.h"

int main(int argc, char** argv) {
	zzub_player_t *player;
	int res;
	
	if (argc != 2) {
		printf("syntax: ccmplayer <path to ccm file>\n");
		return 0;
	}
	
	player = zzub_player_create();

	zzub_player_add_plugin_path(player, "/usr/local/lib64/zzub/");
	zzub_player_add_plugin_path(player, "/usr/local/lib/zzub/");
	zzub_player_add_plugin_path(player, "/usr/lib64/zzub/");
	zzub_player_add_plugin_path(player, "/usr/lib/zzub/");
	
	printf("ccmplayer: initializing player...\n");
	res = zzub_player_initialize(player, 44100);
	if (res) {
		fprintf(stderr, "ccmplayer: error %i initializing player.\n", res);
		return 1;
	}
	zzub_audiodriver_set_samplerate(player, 44100);
	
	res = zzub_audiodriver_create(player, -1, -1);
	if (res) {
		fprintf(stderr, "ccmplayer: error %i creating audiodriver.\n", res);
		return 1;
	}

	zzub_audiodriver_enable(player, 1);

	printf("ccmplayer: loading %s...\n", argv[1]);
	res = zzub_player_load_ccm(player, argv[1]);
	if (res) {
		fprintf(stderr, "ccmplayer: error %i loading %s.\n", res);
		return 1;
	}

	printf("ccmplayer: playing. press a key to quit.\n");

	zzub_player_set_state(player, zzub_player_state_playing);

#if defined(_WIN32)
	getch();
#else
	getchar();
#endif

	zzub_player_set_state(player, zzub_player_state_stopped);

	zzub_audiodriver_destroy(player);


	return 0;
}
