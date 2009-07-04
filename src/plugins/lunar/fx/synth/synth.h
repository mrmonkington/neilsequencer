#if !defined(LUNAR_PLUGIN_LOCALS_H)
#define LUNAR_PLUGIN_LOCALS_H
#define LUNAR_USE_LOCALS
#if defined(__cplusplus)
extern "C" {
#endif // __cplusplus
typedef struct _lunar_attributes {
} lunar_attributes_t;
typedef struct _lunar_globals {
	float *waveform;
	float *attack;
	float *decay;
	float *freq;
	float *cutoff;
	float *res;
	float *accent;
	float *dist;
	float *amp;
	float *oct;
	float *pitchslide;
} lunar_globals_t;
typedef struct _lunar_global_values {
	float waveform;
	float attack;
	float decay;
	float freq;
	float cutoff;
	float res;
	float accent;
	float dist;
	float amp;
	float oct;
	float pitchslide;
} lunar_global_values_t;
typedef struct _lunar_track {
	float *note;
	float *slide;
	float *accent;
} lunar_track_t;
typedef struct _lunar_track_values {
	float note;
	float slide;
	float accent;
} lunar_track_values_t;
typedef struct _lunar_controllers {
} lunar_controllers_t;
typedef struct _lunar_controller_values {
} lunar_controller_values_t;
#if defined(__cplusplus)
}
#endif // __cplusplus
#endif // LUNAR_PLUGIN_LOCALS_H
