#if !defined(LUNAR_PLUGIN_LOCALS_H)
#define LUNAR_PLUGIN_LOCALS_H
#define LUNAR_USE_LOCALS
#if defined(__cplusplus)
extern "C" {
#endif // __cplusplus
typedef struct _lunar_attributes {
} lunar_attributes_t;
typedef struct _lunar_globals {
	float *paraWave;
	float *paraModWave;
	float *paraAttack;
	float *paraDecay;
	float *paraSustain;
	float *paraSustainv;
	float *paraRelease;
	float *paraMAttack;
	float *paraMDecay;
	float *paraMSustain;
	float *paraMSustainv;
	float *paraMRelease;
	float *paraModNote1D;
	float *paraModNote2D;
	float *paraModNote3D;
	float *paraModEnv1;
	float *paraModEnv2;
	float *paraModEnv3;
	float *paraRoute;
} lunar_globals_t;
typedef struct _lunar_global_values {
	float paraWave;
	float paraModWave;
	float paraAttack;
	float paraDecay;
	float paraSustain;
	float paraSustainv;
	float paraRelease;
	float paraMAttack;
	float paraMDecay;
	float paraMSustain;
	float paraMSustainv;
	float paraMRelease;
	float paraModNote1D;
	float paraModNote2D;
	float paraModNote3D;
	float paraModEnv1;
	float paraModEnv2;
	float paraModEnv3;
	float paraRoute;
} lunar_global_values_t;
typedef struct _lunar_track {
	float *note;
	float *volume;
} lunar_track_t;
typedef struct _lunar_track_values {
	float note;
	float volume;
} lunar_track_values_t;
typedef struct _lunar_controllers {
} lunar_controllers_t;
typedef struct _lunar_controller_values {
} lunar_controller_values_t;
#if defined(__cplusplus)
}
#endif // __cplusplus
#endif // LUNAR_PLUGIN_LOCALS_H