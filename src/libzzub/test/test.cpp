#include <stdio.h>
#include <conio.h>
#include "buzzlib.h"
#include "buzzlibc.h"

using namespace zzub;

int main() {

	zzub_player player=zzub_player_create();

	zzub_player_add_audiodriver_path(player, "waveoutput/");
	zzub_player_add_plugin_path(player, "gear/");

	printf("Initializing... ");
	if (!zzub_player_initialize(player, 44100)) {
		printf("Error!\n");
		return 1;
	}
	printf("Done!\n");

	if (!zzub_audiodriver_get_count(player))
	{
		printf("No audio drivers found.\n");
		return 1;
	} 

	BAUDIODRIVER driver=zzub_audiodriver_create(player, 0);

	if (!driver) {
		printf("Could not create audiodriver");
		return 1;
	}

	printf("Loading... ");
	if (!zzub_player_load(player, "f:/audio/buze_play3.bmx")) {
		printf("Failed loading\n");
		return 1;
	}

	printf("Done!\n");
	printf("Playing. Press a key to quit.");

	zzub_player_set_state(player, player_state_playing);

	getch();

	zzub_player_set_state(player, player_state_stopped);

	zzub_audiodriver_destroy(driver);


	return 0;
}
