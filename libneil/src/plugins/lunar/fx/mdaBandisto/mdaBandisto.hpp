#if !defined(LUNAR_PLUGIN_LOCALS_H)
#define LUNAR_PLUGIN_LOCALS_H
#define LUNAR_USE_LOCALS
#if defined(__cplusplus)
extern "C" {
#endif // __cplusplus
typedef struct _lunar_attributes {
} lunar_attributes_t;
typedef struct _lunar_globals {
	float *fParam1_p;
	float *fParam2_p;
	float *fParam3_p;
	float *fParam4_p;
	float *fParam5_p;
	float *fParam6_p;
	float *fParam7_p;
	float *fParam8_p;
	float *fParam9_p;
	float *fParam10_p;
} lunar_globals_t;
typedef struct _lunar_global_values {
	float fParam1_p;
	float fParam2_p;
	float fParam3_p;
	float fParam4_p;
	float fParam5_p;
	float fParam6_p;
	float fParam7_p;
	float fParam8_p;
	float fParam9_p;
	float fParam10_p;
} lunar_global_values_t;
typedef struct _lunar_track {
} lunar_track_t;
typedef struct _lunar_track_values {
} lunar_track_values_t;
typedef struct _lunar_controllers {
} lunar_controllers_t;
typedef struct _lunar_controller_values {
} lunar_controller_values_t;
#if defined(__cplusplus)
}
#endif // __cplusplus
#endif // LUNAR_PLUGIN_LOCALS_H
