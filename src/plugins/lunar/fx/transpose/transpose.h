#if !defined(LUNAR_PLUGIN_LOCALS_H)
#define LUNAR_PLUGIN_LOCALS_H
#define LUNAR_USE_LOCALS
#if defined(__cplusplus)
extern "C" {
#endif // __cplusplus
typedef struct _lunar_globals {
	float *otrans;
	float *ntrans;
	float *hquantize;
	float *q0;
	float *q1;
	float *q2;
	float *q3;
	float *q4;
	float *q5;
	float *q6;
	float *q7;
	float *q8;
	float *q9;
	float *q10;
	float *q11;
} lunar_globals_t;
typedef struct _lunar_global_values {
	float otrans;
	float ntrans;
	float hquantize;
	float q0;
	float q1;
	float q2;
	float q3;
	float q4;
	float q5;
	float q6;
	float q7;
	float q8;
	float q9;
	float q10;
	float q11;
} lunar_global_values_t;
typedef struct _lunar_track {
} lunar_track_t;
typedef struct _lunar_track_values {
} lunar_track_values_t;
typedef struct _lunar_controllers {
	float *note1;
	float *note2;
	float *note3;
	float *note4;
	float *note5;
	float *note6;
	float *note7;
	float *note8;
	float *lnote1;
	float *lnote2;
	float *lnote3;
	float *lnote4;
	float *lnote5;
	float *lnote6;
	float *lnote7;
	float *lnote8;
} lunar_controllers_t;
typedef struct _lunar_controller_values {
	float note1;
	float note2;
	float note3;
	float note4;
	float note5;
	float note6;
	float note7;
	float note8;
	float lnote1;
	float lnote2;
	float lnote3;
	float lnote4;
	float lnote5;
	float lnote6;
	float lnote7;
	float lnote8;
} lunar_controller_values_t;
#if defined(__cplusplus)
}
#endif // __cplusplus
#endif // LUNAR_PLUGIN_LOCALS_H
