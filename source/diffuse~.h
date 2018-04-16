#ifndef YC_DIFFUSE_H_
#define YC_DIFFUSE_H_

// ========  INCLUDES  ========

#include "max_util.h"
#include "envelopes.h"
#include "dict.h"

// ========  DEFINES  ========

// Default values for the arguments
#define CHANNEL_CNT_DEF 2
#define OUT_CNT_DEF     2
#define STATE_CNT_DEF   10

#define INDEFINITE    -1    // Has to be negative to be intrinsically differentiated from valid countdown value

// ========  STRUCTURES  ========

typedef struct _state     t_state;
typedef struct _channel   t_channel;
typedef struct _diffuse   t_diffuse;

// ========  STRUCTURE:  STATE  ========
// Used to store a state

typedef struct _state {

  t_double* A_arr;      // Vector of abscissa values: 0 to 1
  t_double* U_rm_arr;   // Vector of ordinate values using the ramping function: 0 to 1
  t_double* U_xf_arr;   // Vector of ordinate values using the crossfade function: 0 to 1

  t_double* U_cur;  // Pointer used to select between ramping and crossfading

  t_int32 cnt;      // Number of output channels
  t_int32 index;    // Index of the state, _state_init sets to -1, _state_arr_new sets to index

  t_symbol* name;   // Name of the state

} t_state;

// ========  STRUCTURE:  CHANNEL  ========
// Used to store an input channel

typedef enum _mode_type {

  MODE_TYPE_OFF,    // The channel is off: no processing in the perform function
  MODE_TYPE_FIX,    // The channel is fixed: no ramping
  MODE_TYPE_VAR,    // The channel is variable: amplitude ramping

} t_mode_type;

typedef struct _channel {

  t_double* U_cur;    // Vector of N current abscissa values: 0 to 1
  t_double* A_cur;    // Vector of N current ordinate value: 0 to 1
  t_double* U_targ;   // Vector of N target abscissa values: 0 to 1
  t_double* A_targ;   // Vector of N target ordinate values: 0 to 1

  t_int32  cntd;      // Countdown in samples
  t_double velocity;  // Velocity multiplier to affect the rate of change
  t_double gain;      // Gain for the input channel

  t_ramp   interp_func;
  t_ramp   interp_inv_func;
  t_double interp_param;

  t_int32  out_cnt;   // Number of output channels
  t_int32  state_ind; // Index of the state ramping to

  t_bool is_on;         // Is the channel on or not
  t_bool is_frozen;     // Is the channel frozen or not
  t_bool is_mute_ramp;  // Send a message on ramp completion or not

  t_mode_type mode_type;

} t_channel;

// ========  STRUCTURE:  DIFFUSE  ========

typedef enum _output_type {

  OUTP_TYPE_OFF,
  OUTP_TYPE_DB,
  OUTP_TYPE_AMPL

} t_output_type;


typedef struct _diffuse {

  t_pxobject obj;           // Use t_pxobject for MSP objects

  void*    outl_mess;       // Last outlet: for messages

  t_channel* channel_arr;   // Array of input channels
  t_int32    channel_cnt;   // Number of input channels

  t_state* state_arr;       // Array of states
  t_int32  state_cnt;       // Number of states
  t_state  state_tmp[1];    // For temporary calculations

  t_double  master;         // Master gain
  t_int32   out_cnt;        // Number of output channels
  t_double* out_gain;       // Vector of gains for the output channels

  t_double ramp_param;      // Ramping parameter
  t_ramp   ramp_func;       // Ramping function
  t_ramp   ramp_inv_func;   // Inverse ramping function

  t_double xfade_param;     // Crossfade parameter
  t_ramp   xfade_func;      // Crossfade function
  t_ramp   xfade_inv_func;  // Inverse crossfade function

  t_double  samplerate;     // Stores the samplerate
  t_double  msr;            // The samplerate in milliseconds

  t_channel* outp_channel;  // Current channel for output
  t_output_type outp_type;  // Type of output
  t_atom*    outp_mess_arr; // Output message array

  t_symbol* dict_sym;       // Name of a dictionary for storage

} t_diffuse;

// ========  METHOD PROTOTYPES  ========

// ========  MAX MSP METHODS  ========

void* diffuse_new       (t_symbol* sym, t_int32 argc, t_atom* argv);
void  diffuse_free      (t_diffuse* x);
void  diffuse_dsp64     (t_diffuse* x, t_object* dsp64, t_int32* count, t_double samplerate, long maxvectorsize, long flags);
void  diffuse_perform64 (t_diffuse* x, t_object* dsp64, t_double** ins, long numins, t_double** outs, long numouts, long sampleframes, long flags, void* userparam);
void  diffuse_assist    (t_diffuse* x, void* b, long msg, t_int32 arg, char* str);

// ======== DIFFUSE METHODS ========

void diffuse_bang       (t_diffuse* x);
void diffuse_dictionary (t_diffuse* x, t_symbol* dict_sym);
void diffuse_get        (t_diffuse* x);
void diffuse_master     (t_diffuse* x, double master);
void diffuse_gain_out   (t_diffuse* x, t_symbol* sym, t_int32 argc, t_atom* argv);
void diffuse_output     (t_diffuse* x, t_symbol* outp_type);
void diffuse_set        (t_diffuse* x, t_symbol* sym, t_int32 argc, t_atom* argv);

// ========  CHANNEL METHODS  ========

t_channel* _channel_find  (t_diffuse* x, t_atom* argv);
void       _channel_init  (t_diffuse* x, t_channel* channel);
t_my_err   _channel_alloc (t_diffuse* x, t_channel* channel, t_double u, t_double a);
void       _channel_free  (t_diffuse* x, t_channel* channel);

void       _channel_calc_absc (t_diffuse* x, t_channel* channel);

void channel_channel   (t_diffuse* x, t_symbol* sym, t_int32 argc, t_atom* argv);
void channel_gain_in   (t_diffuse* x, t_symbol* sym, t_int32 argc, t_atom* argv);
void channel_mute_ramp (t_diffuse* x, t_symbol* sym, t_int32 argc, t_atom* argv);

// ========  STATE METHODS  ========

void     _state_init  (t_state* state);
t_my_err _state_alloc (t_state* state, t_int32 param_cnt, t_double u, t_double a);
void     _state_free  (t_state* state);

t_state* _state_arr_new  (t_int32 _state_cnt, t_int32* state_cnt, t_int32 param_cnt);
void     _state_arr_free (t_state** state_arr, t_int32* state_cnt);

t_state* _state_find      (t_state* state_arr, t_int32 state_cnt, t_atom* atom);
void     _state_calc_absc (t_diffuse* x, t_state * state);
t_my_err _state_store     (t_diffuse* x, t_channel* channel, t_state* state, t_symbol* name);
void     _state_ramp      (t_diffuse* x, t_channel* channel, t_state* state, t_int32 cntd, t_int32 offset);
void     _state_iterate   (t_diffuse* x, t_channel* channel);

t_my_err _state_dict_save (t_state* state, t_dictionary* dict_arr_states, t_symbol* state_sym, t_symbol* is_prot);
t_my_err _state_dict_load (t_dictionary* dict_state, t_state* state);

void state_state        (t_diffuse* x, t_symbol* sym, t_int32 argc, t_atom* argv);
void state_ramp_to      (t_diffuse* x, t_symbol* sym, t_int32 argc, t_atom* argv);
void state_ramp_between (t_diffuse* x, t_symbol* sym, t_int32 argc, t_atom* argv);
void state_ramp_max     (t_diffuse* x, t_symbol* sym, t_int32 argc, t_atom* argv);
void state_circular     (t_diffuse* x, t_symbol* sym, t_int32 argc, t_atom* argv);
void state_velocity     (t_diffuse* x, t_symbol* sym, t_int32 argc, t_atom* argv);
void state_velocity_all (t_diffuse* x, double velocity);
void state_freeze       (t_diffuse* x, t_symbol* sym, t_int32 argc, t_atom* argv);
void state_freeze_all   (t_diffuse* x, long is_frozen);

#endif
