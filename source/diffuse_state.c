#include "diffuse~.h"

// ====  _STATE_INIT  ====

//******************************************************************************
//  Initialize a state. Call before _state_alloc to set all array pointers to NULL.
//
void _state_init(t_state* state) {

  // Initialize the symbols to the empty symbol
  state->name = gensym("null");

  // Initialize the array pointers to NULL
  state->A_arr    = NULL;
  state->U_rm_arr = NULL;
  state->U_xf_arr = NULL;
  state->U_cur    = NULL;

  // Initialize the count to -1
  state->cnt = -1;
  state->index = -1;
}

// ====  _STATE_ALLOC  ====

//******************************************************************************
//  Allocate arrays for a state.
//  Call only after _state_init to make sure the array pointers were set NULL.
//  Returns:
//  ERR_NONE:  Succesful initialization
//  ERR_COUNT:  Invalid count argument, should be one at least
//  ERR_ALLOC:  Failed allocation
//
t_my_err _state_alloc(t_state* state, t_int32 param_cnt, t_double u, t_double a) {

  // The count should not be negative
  if (param_cnt < 0) {
    state->cnt = -1;
    return ERR_COUNT;
  }

  // ... if it is equal to 0:
  else if (param_cnt == 0) {
    state->cnt = 0;
    return ERR_NONE;
  }

  // ... if it is at least 1:
  else {

    // Allocate the arrays
    state->A_arr = (t_double*)sysmem_newptr(sizeof(t_double) * param_cnt);
    state->U_rm_arr = (t_double*)sysmem_newptr(sizeof(t_double) * param_cnt);
    state->U_xf_arr = (t_double*)sysmem_newptr(sizeof(t_double) * param_cnt);
    state->U_cur = state->U_xf_arr;

    // Then test the allocation
    if (state->A_arr && state->U_rm_arr && state->U_xf_arr) {

      // Set the count
      state->cnt = param_cnt;

      // Initialize the array values
      for (t_int32 res = 0; res < state->cnt; res++) {
        state->A_arr[res] = a;
        state->U_rm_arr[res] = u;
        state->U_xf_arr[res] = u;
      }

      return ERR_NONE;
    }

    // Otherwise there was an allocation error
    else {
      state->cnt = -1;
      if (state->A_arr) { sysmem_freeptr(state->A_arr); state->A_arr = NULL; }
      if (state->U_rm_arr) { sysmem_freeptr(state->U_rm_arr); state->U_rm_arr = NULL; }
      if (state->U_xf_arr) { sysmem_freeptr(state->U_xf_arr); state->U_xf_arr = NULL; }
      return ERR_ALLOC;
    }
  }
}

// ====  _STATE_FREE  ====

//******************************************************************************
//  Free one state
//
void _state_free(t_state* state) {

  state->cnt = 0;

  if (state->A_arr) {
    sysmem_freeptr(state->A_arr);
    state->A_arr = NULL;
  }

  if (state->U_rm_arr) {
    sysmem_freeptr(state->U_rm_arr);
    state->U_rm_arr = NULL;
  }

  if (state->U_xf_arr) {
    sysmem_freeptr(state->U_xf_arr);
    state->U_xf_arr = NULL;
  }
}

// ====  _STATE_ARR_NEW  ====

//******************************************************************************
//  Create an array of states
//  cnt:  number of states in the array, at least 1
//  state_cnt:  pointer to the element count of the array
//  Returns a pointer to an array of states or NULL
//
t_state* _state_arr_new(t_int32 _state_cnt, t_int32* state_cnt, t_int32 param_cnt) {

  // Test that the count for the array is at least one
  if (_state_cnt < 1) { return NULL; }

  // Allocate the array of states and test the allocation
  t_state* state_arr = (t_state*)sysmem_newptr(sizeof(t_state) * _state_cnt);
  if (!state_arr) { return NULL; }

  // Initialize and then allocate each state
  for (t_int32 st = 0; st < _state_cnt; st++) {
    _state_init(state_arr + st);
    (state_arr + st)->index = st;
  }
  for (t_int32 st = 0; st < _state_cnt; st++) {
    if (_state_alloc(state_arr + st, param_cnt, 0, 0) != ERR_NONE) { return NULL; }
  }

  // Set the count and return the allocated pointer
  *state_cnt = _state_cnt;
  return state_arr;
}

// ====  _STATE_ARR_FREE  ====

//******************************************************************************
//  Free an array of states
//
void _state_arr_free(t_state** state_arr, t_int32* state_cnt) {

  // Test if the array of storage slots is not yet allocated
  if (!*state_arr) { return; }

  // Otherwise proceed to free each state
  for (t_int32 st = 0; st < *state_cnt; st++) { _state_free(*state_arr + st); }

  // Free the array of states and set the count to 0
  sysmem_freeptr(*state_arr);
  *state_arr = NULL;
  *state_cnt = 0;
}

// ====  _STATE_FIND  ====

//******************************************************************************
//  Find a state within the array of states
//  Returns a pointer to the state or NULL
//
t_state* _state_find(t_state* state_arr, t_int32 state_cnt, t_atom* atom) {

  // Test if the array of storage slots is not yet allocated
  // If the atom is not an integer
  if ((!state_arr) || (atom_gettype(atom) != A_LONG)) { return NULL; }

  // Test if the index of the slot is within range
  t_int32 state_ind = (t_int32)atom_getlong(atom);
  if ((state_ind < 0) || (state_ind >= state_cnt)) { return NULL; }

  return (state_arr + state_ind);
}

// ====  _STATE_CALC_ABSC  ====

//******************************************************************************
//  Calculate the abscissa values for the current ramping and crossfade functions
//
void _state_calc_absc(t_diffuse* x, t_state * state) {

for (t_int32 ch = 0; ch < state->cnt; ch++) {
  state->U_rm_arr[ch] = x->ramp_inv_func(state->A_arr[ch], x->ramp_param);
  state->U_xf_arr[ch] = x->xfade_inv_func(state->A_arr[ch], x->xfade_param);
}
}

// ====  _STATE_STORE  ====

//******************************************************************************
//  Set a storage slot to the current values from a channel
//  Returns:
//  ERR_NONE:  Storing successful
//  ERR_COUNT:  Invalid count argument, should be one at least
//  ERR_ALLOC:  Failed allocation
//
t_my_err _state_store(t_diffuse* x, t_channel* channel, t_state* state, t_symbol* name) {

  // Copy the current values from the channel into the state
  for (t_int32 ch = 0; ch < state->cnt; ch++) { state->A_arr[ch] = channel->A_cur[ch]; }

  // Calculate the abscissa values
  _state_calc_absc(x, state);

  // Set the name of the state
  state->name = name;

  return ERR_NONE;
}

// ====  _STATE_ITERATE  ====

//******************************************************************************
//  Iterate the channel when the countdown reaches 0
//
void _state_iterate(t_diffuse* x, t_channel* channel) {

  // Output in case it is not muted
  if (!channel->is_mute_ramp) {

    // Output a message with information about the state
    //   end_ramp (int: channel index) (int: mode type)
    t_atom mess_arr[2];
    atom_setlong(mess_arr, channel - x->channel_arr);
    atom_setlong(mess_arr + 1, channel->state_ind);
    outlet_anything(x->outl_mess, gensym("end_ramp"), 2, mess_arr);
  }

  // Update
  switch (channel->mode_type) {
  case MODE_TYPE_VAR:

    channel->cntd = INDEFINITE;
    channel->mode_type = MODE_TYPE_FIX;
    //POST("Channel %i:  End or ramp:  ", channel - x->channel_arr);

    for (t_int32 ch = 0; ch < channel->out_cnt; ch++) {
      //POST("%i: U_cur = %f - U_targ = %f - A_cur = %f - A_targ = %f",
      //  ch, channel->U_cur[ch], channel->U_targ[ch], channel->A_cur[ch], channel->A_targ[ch]);
      channel->U_cur[ch] = channel->U_targ[ch];
      channel->A_cur[ch] = channel->A_targ[ch];
    }

    break;
  }
}

// ====  _STATE_DICT_SAVE  ====

//******************************************************************************
//  To save a state into a dictionary. Passed as a function pointer argument to dict_save
//  Returns ERR_NONE or ERR_ALLOC
//
t_my_err _state_dict_save(t_state* state, t_dictionary* dict_arr_states, t_symbol* state_sym, t_symbol* is_prot) {

  t_dictionary* dict_state = dictionary_sprintf("@name %s @count %i", state_sym->s_name, state->cnt);
  dictionary_appenddictionary(dict_arr_states, state_sym, (t_object*)dict_state);

  // Create an array of atoms for temporary storage
  t_atom* atom_arr = (t_atom*)sysmem_newptr(sizeof(t_atom)* state->cnt);
  if (!atom_arr) { return ERR_ALLOC; }

  // Create subdictionaries for the abscissa and ordinate arrays, and append them to the state dictionary
  atom_setdouble_array(state->cnt, atom_arr, state->cnt, state->A_arr);
  dictionary_appendatoms(dict_state, gensym("ordinate"), state->cnt, atom_arr);

  sysmem_freeptr(atom_arr);
  return ERR_NONE;
}

// ====  _STATE_DICT_LOAD  ====

//******************************************************************************
//  Load a state from a dictionary. Passed as a function pointer argument to dict_load
//  Returns:
//  ERR_NONE:  Succesful initialization
//  ERR_COUNT:  Invalid count argument, should be one at least
//  ERR_ALLOC:  Failed allocation (from _state_init)
//
t_my_err _state_dict_load(t_dictionary* dict_state, t_state* state) {

  // Get the number of state values from the dictionary
  t_atom_long a_count = 0;
  dictionary_getlong(dict_state, gensym("count"), &a_count);
  if ((a_count < 1) || (a_count != state->cnt)) { return ERR_COUNT; }

  // Get "name" and "from" from the dictionary
  dictionary_getsym(dict_state, gensym("name"), &(state->name));

  // Get the "abscissa" and "ordinate" arrays from the dictionary
  t_atom* atom_arr = NULL;
  long a_long;

  dictionary_getatoms(dict_state, gensym("ordinate"), &a_long, &atom_arr);
  if (a_long != state->cnt) { return ERR_COUNT; }
  for (t_int32 param = 0; param < state->cnt; param++) { state->A_arr[param] = atom_getfloat(atom_arr + param); }

  // If all is successfull free the array of atoms and return
  return ERR_NONE;
}

// ====  STATE_STATE  ====

//******************************************************************************
//  Interface method to call:  new / free / resize / set / name / get / post / store / save / load / rename / delete
//
void state_state(t_diffuse* x, t_symbol* sym, t_int32 argc, t_atom* argv) {

  TRACE("state_state");

  // Argument 0 should be a command
  MY_ASSERT((argc < 1) || (atom_gettype(argv) != A_SYM),
    "state:  Arg 0:  Command expected: new / free / resize / get / post / store / save / load / rename / delete.");
  t_symbol* cmd = atom_getsym(argv);

  // Test that the array of states exists
  MY_ASSERT((cmd != gensym("new")) && (cmd != gensym("rename")) && (cmd != gensym("delete"))
    && (!x->state_arr), "state:  No array of states available.");

  // ====  NEW:  Allocate a new array of states  ====
  // state new (int: state count)

  if (cmd == gensym("new")) {

    MY_ASSERT(x->state_arr, "state new:  An array of states already exists.");
    MY_ASSERT(argc != 2, "state new:  2 args expected:  state new (int: array size)");
    MY_ASSERT(atom_gettype(argv + 1) != A_LONG, "state new:  Arg 1:  Int expected for the number of states.");

    t_int32 state_cnt = (t_int32)atom_getlong(argv + 1);
    MY_ASSERT(state_cnt < 1, "state new:  Arg 1:  Value of at least 1 expected for the number of states.");

    x->state_arr = _state_arr_new(state_cnt, &(x->state_cnt), x->out_cnt);
    MY_ASSERT(!x->state_arr, "state new:  Failed to allocate an array of states.");

    POST("state new:  Array of %i states created.", x->state_cnt);
  }

  // ====  FREE:  Free the array of states  ====
  // state free

  else if (cmd == gensym("free")) {

    MY_ASSERT(argc != 1, "state free:  1 args expected:  state free");

    _state_arr_free(&(x->state_arr), &(x->state_cnt));

    POST("state free:  Array of states freed.");
  }

  // ====  RESIZE:  Resize the array of storage slots  ====

  else if ((cmd == gensym("resize")) && (argc == 3) && (atom_gettype(argv + 2) == A_LONG)) {
    ;
  }

  // ====  SET:  Set the state values  ====
  // state set (int: state index) (float: [0-1] gain) {x N}

  else if (cmd == gensym("set")) {

    MY_ASSERT(argc != x->out_cnt + 2, "state set:  %i args expected:  state set (int: state index) (float: gain) {x %i}", x->out_cnt + 2, x->out_cnt);

    // Argument 1 should reference a non empty state
    t_state* state = _state_find(x->state_arr, x->state_cnt, argv + 1);
    MY_ASSERT(!state, "state set:  Arg 1:  State not found.");

    // Test the state values
    for (t_int32 ch = 2; ch < state->cnt + 2; ch++) {
      MY_ASSERT(((atom_gettype(argv + ch) != A_FLOAT) && (atom_gettype(argv + ch) != A_LONG))
        || (atom_getfloat(argv + ch) < 0) || (atom_getfloat(argv + ch) > 1),
        "state set:  Arg %i:  Float [0-1] expected for the gain.", ch);
      }

    // Set the ordinate values and calculate the abscissa values
    for (t_int32 ch = 0; ch < state->cnt; ch++) { state->A_arr[ch] = atom_getfloat(argv + ch + 2); }
    _state_calc_absc(x, state);
  }

  // ====  NAME:  Set the state name  ====
  // state name (int: state index) (sym: state name)

  else if (cmd == gensym("name")) {

    MY_ASSERT(argc != 3, "state name:  3 args expected:  state name (int: state index) (sym: state name)");

    // Argument 1 should reference a non empty state
    t_state* state = _state_find(x->state_arr, x->state_cnt, argv + 1);
    MY_ASSERT(!state, "state name:  Arg 1:  State not found.");

    // Argument 2 should be a symbol with the name of the state
    MY_ASSERT(atom_gettype(argv + 2) != A_SYM, "state name:  Arg 2:  Symbol expected for the name of the state.");

    state->name = atom_getsym(argv + 2);
  }

  // ====  GET:  get information on a state as a message  ====
  // state get (int: state index)

  else if (cmd == gensym("get")) {

    MY_ASSERT(argc != 2, "state get:  2 args expected:  state get (int: state index)");

    // Argument 1 should reference a non empty state
    t_state* state = _state_find(x->state_arr, x->state_cnt, argv + 1);
    MY_ASSERT(!state, "state get:  Arg 1:  State not found.");

    // Output a message with information about the state
    //   state (int: index) (sym: name) (int: count) (float: gain) {x N}
    t_atom* mess_arr = (t_atom*)sysmem_newptr(sizeof(t_atom)* (x->out_cnt + 3));
    MY_ASSERT(!mess_arr, "state get:  Allocation failed for the output message array.");
    t_atom* atom = mess_arr;

    atom_setlong(atom++, state - x->state_arr);
    atom_setsym(atom++, state->name);
    atom_setlong(atom++, state->cnt);
    for (t_int32 ch = 0; ch < x->out_cnt; ch++) { atom_setfloat(atom++, state->A_arr[ch]);  }

    outlet_anything(x->outl_mess, gensym("state"), x->out_cnt + 3, mess_arr);
    sysmem_freeptr(mess_arr);
  }

  // ====  POST:  Post information on the array of states  ====
  // state post (int: state index / sym: all)

  else if (cmd == gensym("post")) {

    MY_ASSERT(argc != 2, "state post:  2 args expected:  state post (int: state index / sym: all)");

    // If Arg 1 is a symbol and equal to "all"
    if ((atom_gettype(argv + 1) == A_SYM) && (atom_getsym(argv + 1) == gensym("all"))) {

      // Post information on all the states
      POST("The array of states has %i elements.", x->state_cnt);
      t_state* state = NULL;
      for (t_int32 st = 0; st < x->state_cnt; st++) {
        state = x->state_arr + st;
        POST("  State %i:  Name: %s - Count: %i", st, state->name->s_name, state->cnt);
      }
    }

    // ... If Arg 1 is an int
    else if (atom_gettype(argv + 1) == A_LONG) {

      // Find the state
      t_state* state = _state_find(x->state_arr, x->state_cnt, argv + 1);
      MY_ASSERT(!state, "state post:  Arg 1:  State not found.");

      // Post detailed information on one state
      POST("The array of states has %i elements.", x->state_cnt);
      POST("State %i:  Name: %s - Count: %i", state - x->state_arr, state->name->s_name, state->cnt);

      for (t_int32 param = 0; param < state->cnt; param++) {
        POST("  Value %i:  A = %f, U ramp = %f, U xfade = %f",
          param, state->A_arr[param], state->U_rm_arr[param], state->U_xf_arr[param]);
        }
      }

    // ... Otherwise the arguments are invalid
    else { MY_ASSERT(argc != 2, "state post:  Invalid args:  state post (int: state index / sym: all)"); }
  }

  // ====  STORE:  Store the values from a channel into a state  ====
  // state store (int: channel index) (int: state index) (sym: state name)

  else if (cmd == gensym("store")) {

    MY_ASSERT(argc != 4, "state store:  4 args expected:  state store (int: channel index) (int: state index) (sym: state name)");

    // Argument 1 should reference a channel
    t_channel* channel = _channel_find(x, argv + 1);
    MY_ASSERT(!channel, "state store:  Arg 1:  Channel not found");

    // Argument 2 should reference a state
    t_state* state = _state_find(x->state_arr, x->state_cnt, argv + 2);
    MY_ASSERT(!state, "state store:  Arg 2:  State not found");

    // Argument 3 should hold the name of the state as a symbol
    t_symbol* name = atom_getsym(argv + 3);
    MY_ASSERT(name == gensym(""), "state store:  Arg 3:  Symbol expected for the name of the state.")

    t_my_err err = _state_store(x, channel, state, name);
    MY_ASSERT(err != ERR_NONE, "Failed to allocate an array of states.");

    POST("state store:  Channel %i state stored in state %i as \"%s\"", channel - x->channel_arr, state - x->state_arr, state->name->s_name);
  }

  // ====  SAVE:  Save a state under a new name  ====
  // state save (int: state index) (sym: state name)

  else if (cmd == gensym("save")) {

    MY_ASSERT(argc != 3, "state save:  3 args expected:  state save (int: state index) (sym: state name)");

    // Argument 1 should reference a non empty state
    t_state* state = _state_find(x->state_arr, x->state_cnt, argv + 1);
    MY_ASSERT(!state, "state save:  Arg 1:  State not found.");
    MY_ASSERT(!state->cnt, "state save:  Arg 1:  The state is empty.");

    if (dict_save(x, x->dict_sym, gensym("states"), gensym("state save"), 1, state, argv + 2, _state_dict_save) == ERR_NONE) {
      POST("state save:  State %i saved as \"%s\" - Count: %i.", state - x->state_arr, atom_getsym(argv + 2)->s_name, state->cnt);
    }
  }

  // ====  LOAD:  Load a state  ====
  // state load (sym: state name) (int: state index)

  else if (cmd == gensym("load")) {

    MY_ASSERT(argc != 3, "state load:  3 args expected:  state load (sym: state name) (int: state index)");

    // Argument 2 should reference a state
    t_state* state = _state_find(x->state_arr, x->state_cnt, argv + 2);
    MY_ASSERT(!state, "state load:  Arg 2:  State not found.");

    if (dict_load(x, x->dict_sym, gensym("states"), gensym("state load"), 1, state, argv + 1, _state_dict_load) == ERR_NONE) {

      // Calculate the abscissa values
      _state_calc_absc(x, state);
      POST("state load:  State \"%s\" loaded into %i - Count: %i.", atom_getsym(argv + 1)->s_name, state - x->state_arr, state->cnt);
    }
  }

  // ====  DELETE:  Delete a state  ====
  // state delete (sym: state name)

  else if (cmd == gensym("delete")) {

    MY_ASSERT(argc != 2, "state delete:  2 args expected:  state delete (sym: state name)");

    if (dict_delete(x, x->dict_sym, gensym("states"), gensym("state delete"), 1, argv + 1) == ERR_NONE) {
      POST("state delete:  State \"%s\" deleted from the dictionary.", atom_getsym(argv + 1)->s_name);
    }
  }

  // ====  RENAME:  Rename a state  ====
  // state rename (sym: state1 name) (sym: state2 name)

  else if (cmd == gensym("rename")) {

    MY_ASSERT(argc != 3,
      "state rename:  3 args expected:  state rename (sym: state1 name) (sym: state2 name)");

    if (dict_rename(x, x->dict_sym, gensym("states"), gensym("state rename"), 1, argv + 1, argv + 2) == ERR_NONE) {
      POST("state rename:  State \"%s\" renamed to \"%s\".", atom_getsym(argv + 1)->s_name, atom_getsym(argv + 2)->s_name);
    }
  }

  // ====  Otherwise the command is invalid  ====

  else {
    MY_ERR("state:  Arg 0:  Command expected: new / free / resize / get / post / store / save / load / rename / delete.");
  }
}

// ====  _STATE_RAMP  ====

//******************************************************************************
//  Ramp a channel to a state
//  Used by the interface ramping methods
//
void _state_ramp(t_diffuse* x, t_channel* channel, t_state* state, t_int32 cntd, t_int32 offset) {

  // Set the countdown
  channel->cntd = cntd;
  channel->mode_type = MODE_TYPE_VAR;
  channel->state_ind = state->index;

  // Set all the target values to the state values
  t_int32 ch2;

  for (t_int32 ch1 = 0; ch1 < state->cnt; ch1++) {
    ch2 = (ch1 + offset) % x->out_cnt;
    channel->U_targ[ch2] = state->U_cur[ch1];
    channel->A_targ[ch2] = state->A_arr[ch1];
  }
}

// ====  STATE_RAMP_TO  ====

//******************************************************************************
//  Ramp a channel to a state
//  ramp_to (int: channel index) (int: state index) (float: time in ms) (sym: ramp or xfade)
//
void state_ramp_to(t_diffuse* x, t_symbol* sym, t_int32 argc, t_atom* argv) {

  TRACE("state_ramp_to");

  // The method expects three arguments
  MY_ASSERT(argc != 4, "ramp_to:  4 args expected:  ramp_to (int: channel index) (int: state index) (float: time in ms) (sym: ramp or xfade)");

  // Argument 0 should reference a channel
  t_channel* channel = _channel_find(x, argv);
  MY_ASSERT(!channel, "ramp_to:  Arg 0:  Channel not found.");

  // Argument 1 should reference a state
  t_state* state = _state_find(x->state_arr, x->state_cnt, argv + 1);
  MY_ASSERT(!state, "ramp_to:  Arg 1:  State not found.");

  // Argument 2 should be a positive float: the time in ms
  MY_ASSERT((atom_gettype(argv + 2) != A_FLOAT) && (atom_gettype(argv + 2) != A_LONG),
    "ramp_to:  Arg 2:  Positive float expected: time in ms.");

  t_double time = atom_getfloat(argv + 2);
  MY_ASSERT(time <= 0, "ramp_to:  Arg 2:  Positive float expected: time in ms.");

  // Argument 3 should be "ramp" or "xfade"
  t_symbol* interp_type = atom_getsym(argv + 3);

  if (interp_type == gensym("ramp")) {
    state->U_cur = state->U_rm_arr;
    channel->interp_func = x->ramp_func;
    channel->interp_inv_func = x->ramp_inv_func;
    channel->interp_param = x->ramp_param;
  }

  else if (interp_type == gensym("xfade")) {
    state->U_cur = state->U_xf_arr;
    channel->interp_func = x->xfade_func;
    channel->interp_inv_func = x->xfade_inv_func;
    channel->interp_param = x->xfade_param;
  }

  else { MY_ASSERT(1, "ramp_to:  Arg 3:  \"ramp\" or \"xfade\" expected."); }

  // Set the channel ramping values
  _state_ramp(x, channel, state, (t_int32)(time * x->msr), 0);
}

// ====  STATE_RAMP_BETWEEN  ====

//******************************************************************************
//  Ramp a channel to an interpolated setting between two states
//  ramp_betweeen (int: channel) (int: state 1) (int: state 2) (float: interpolation) (float: time in ms) (sym: ramp or xfade)
//
void state_ramp_between(t_diffuse* x, t_symbol* sym, t_int32 argc, t_atom* argv) {

  TRACE("state_ramp_between");

  // The method expects five arguments
  MY_ASSERT(argc != 6, "ramp_between:  5 args expected:  ramp_betweeen (int: channel) (int: state 1) (int: state 2) (float: interpolation) (float: time in ms) (sym: ramp or xfade)");

  // Argument 0 should reference a channel
  t_channel* channel = _channel_find(x, argv);
  MY_ASSERT(!channel, "ramp_between:  Arg 0:  Channel not found.");

  // Argument 1 should reference a state
  t_state* state1 = _state_find(x->state_arr, x->state_cnt, argv + 1);
  MY_ASSERT(!state1, "ramp_between:  Arg 1:  State not found.");

  // Argument 2 should reference a state
  t_state* state2 = _state_find(x->state_arr, x->state_cnt, argv + 2);
  MY_ASSERT(!state2, "ramp_between:  Arg 2:  State not found.");

  // Argument 3 should be a float between 0 and 1: interpolation between the two states
  MY_ASSERT((atom_gettype(argv + 3) != A_FLOAT) && (atom_gettype(argv + 3) != A_LONG),
    "ramp_between:  Arg 3:  Float [0-1] expected: interpolation between the two states.");

  t_double interp = atom_getfloat(argv + 3);
  MY_ASSERT((interp < 0) || (interp > 1),
    "ramp_between:  Arg 3:  Float [0-1] expected: interpolation between the two states.");

  // Argument 4 should be the time in ms
  MY_ASSERT((atom_gettype(argv + 4) != A_FLOAT) && (atom_gettype(argv + 4) != A_LONG),
    "ramp_between:  Arg 4:  Positive float expected: time in ms.");

  t_double time = atom_getfloat(argv + 4);
  MY_ASSERT(time <= 0, "ramp_between:  Arg 4:  Positive float expected: time in ms.");

  // Argument 5 should be "ramp" or "xfade"
  t_symbol* interp_type = atom_getsym(argv + 5);

  if (interp_type == gensym("ramp")) {
    state1->U_cur = state1->U_rm_arr;
    state2->U_cur = state2->U_rm_arr;
    channel->interp_func = x->ramp_func;
    channel->interp_inv_func = x->ramp_inv_func;
    channel->interp_param = x->ramp_param;
  }

  else if (interp_type == gensym("xfade")) {
    state1->U_cur = state1->U_xf_arr;
    state2->U_cur = state2->U_xf_arr;
    channel->interp_func = x->xfade_func;
    channel->interp_inv_func = x->xfade_inv_func;
    channel->interp_param = x->xfade_param;
  }

  else { MY_ASSERT(1, "ramp_between:  Arg 5:  \"ramp\" or \"xfade\" expected."); }

  // Calculate the interpolated values from the abscissa
  for (t_int32 ch = 0; ch < channel->out_cnt; ch++) {
    x->state_tmp->U_cur[ch] = state1->U_cur[ch] + interp * (state2->U_cur[ch] - state1->U_cur[ch]);
    x->state_tmp->A_arr[ch] = channel->interp_func(x->state_tmp->U_cur[ch], channel->interp_param);
  }

  _state_ramp(x, channel, x->state_tmp, (t_int32)(time * x->msr), 0);
}

// ====  STATE_RAMP_MAX  ====

//******************************************************************************
//  Ramp a channel to the maximum of a list of interpolated states
//  ramp_max (int: channel) [(int: state) (float: interpolation)] {x N} (float: time in ms) (sym: ramp or xfade)
//
void state_ramp_max(t_diffuse* x, t_symbol* sym, t_int32 argc, t_atom* argv) {

  TRACE("state_ramp_max");

  // The method expects (3 + 2*n) arguments
  MY_ASSERT((((argc % 2) != 1) || (argc < 4)),
    "ramp_max:  Expects:  ramp_max (int: channel) [(int: state) (float: interpolation)] {x N} (float: time in ms) (sym: ramp or xfade)");

  // Argument 0 should reference a channel
  t_channel* channel = _channel_find(x, argv);
  MY_ASSERT(!channel, "ramp_max:  Arg 0:  Channel not found.");

  // The penultimate argument should be the time in ms
  MY_ASSERT((atom_gettype(argv + argc - 2) != A_FLOAT) && (atom_gettype(argv + argc - 2) != A_LONG),
    "ramp_max:  Arg %i:  Positive float expected: time in ms.", argc - 2);

  t_double time = atom_getfloat(argv + argc - 2);
  MY_ASSERT(time <= 0, "ramp_max:  Arg %i:  Positive float expected: time in ms.", argc - 2);

  // The last argument should be "ramp" or "xfade"
  t_symbol* interp_type = atom_getsym(argv + argc - 1);

  if (interp_type == gensym("ramp")) {
    channel->interp_func = x->ramp_func;
    channel->interp_inv_func = x->ramp_inv_func;
    channel->interp_param = x->ramp_param;
  }

  else if (interp_type == gensym("xfade")) {
    channel->interp_func = x->xfade_func;
    channel->interp_inv_func = x->xfade_inv_func;
    channel->interp_param = x->xfade_param;
  }

  else { MY_ASSERT(1, "ramp_max:  Arg %i:  \"ramp\" or \"xfade\" expected.", argc - 1); }

  // The arguments from the second one should be [int, float] pairs
  t_int32 state_cnt = argc / 2 - 1;
  t_atom* atom = argv + 1;
  t_state* state = NULL;
  t_double interp;

  for (t_int32 res = 0; res < channel->out_cnt; res++) { x->state_tmp->U_cur[res] = 0; }

  while (state_cnt--) {

    // The first argument of the pair should reference a state
    state = _state_find(x->state_arr, x->state_cnt, atom++);
    MY_ASSERT(!state, "ramp_max:  Arg:  State not found.");

    // The second argument of the pair should be a float between 0 and 1: interpolation from 0
    MY_ASSERT((atom_gettype(atom) != A_FLOAT) && (atom_gettype(atom) != A_LONG),
      "ramp_max:  Arg:  Float [0-1] expected: interpolation between 0 and state");

    interp = atom_getfloat(atom++);
    MY_ASSERT((interp < 0) || (interp > 1),
      "ramp_max:  Arg:  Float [0-1] expected: interpolation between 0 and state");

    // Set which array to use: ramping or crossfading
    state->U_cur = (interp_type == gensym("ramp")) ? state->U_rm_arr : state->U_xf_arr;

    // Calculate the interpolated values and take the maximum
    for (t_int32 ch = 0; ch < channel->out_cnt; ch++) {
      x->state_tmp->U_cur[ch] = MAX(interp * state->U_cur[ch], x->state_tmp->U_cur[ch]);
    }
  }

  // Calculate the ordinate values
  for (t_int32 ch = 0; ch < channel->out_cnt; ch++) {
    x->state_tmp->A_arr[ch] = channel->interp_func(x->state_tmp->U_cur[ch], x->ramp_param);
  }

  _state_ramp(x, channel, x->state_tmp, (t_int32)(time * x->msr), 0);
}

// ====  STATE_CIRCULAR  ====

//******************************************************************************
//  Circular permutation and interpolation of channels
//  circular (int: channel first index) (int: channel count) (int: state index) (float: interpolation) (float: time in ms)
//
void state_circular(t_diffuse* x, t_symbol* sym, t_int32 argc, t_atom* argv) {

  TRACE("state_circular");

  // The method expects 5 arguments
  MY_ASSERT(argc != 5,
    "circular:  5 args expected:  circular (int: channel first index) (int: channel count) (int: state index) (float: interpolation) (float: time in ms)");

  // Argument 0 should reference a channel
  t_channel* channel = _channel_find(x, argv);
  MY_ASSERT(!channel, "ramp_max:  Arg 0:  Channel not found.");

  // Argument 1 should reference the number of input channels to permutate
  MY_ASSERT(atom_gettype(argv + 1) != A_LONG, "circular:  Arg 1:  Int expected: number of input channels to permutate.");
  t_int32 ch_cnt = (t_int32)atom_getlong(argv + 1);
  MY_ASSERT(((ch_cnt < 1) || ((t_int32)(channel - x->channel_arr) + ch_cnt > x->channel_cnt)),
    "circular:  Arg 1:  Invalid value: number of input channels to permutate.");

  // Argument 2 should reference a state
  t_state* state = _state_find(x->state_arr, x->state_cnt, argv + 2);
  MY_ASSERT(!state, "circular:  Arg 2:  State not found.");

  // Argument 3 should be a float: circular interpolation
  MY_ASSERT((atom_gettype(argv + 3) != A_FLOAT) && (atom_gettype(argv + 3) != A_LONG),
    "circular:  Arg 3:  Float expected: circular interpolation.");
  t_double interp = atom_getfloat(argv + 3);

  // Argument 4 should be the time in ms
  MY_ASSERT((atom_gettype(argv + 4) != A_FLOAT) && (atom_gettype(argv + 4) != A_LONG),
    "circular:  Arg 4:  Positive float expected: time in ms.");

  t_double time = atom_getfloat(argv + 4);
  MY_ASSERT(time <= 0, "circular:  Arg 4:  Positive float expected: time in ms.");

  // Calculate the integer and fractional parts for the circular interpolation
  t_int32 offset = (t_int32)floor(interp);
  interp -= offset;
  offset = (offset < 0) ? (offset % x->out_cnt) + 8 : (offset % x->out_cnt);
  POST("CIRC %i %f", offset, interp);

  state->U_cur = state->U_xf_arr;

  // Loop over the state values
  for (t_int32 ch = 0; ch < x->out_cnt; ch++) {
    x->state_tmp->U_cur[ch] = state->U_cur[ch] + interp * (state->U_cur[(ch + x->out_cnt - 1) % x->out_cnt] - state->U_cur[ch]);
    x->state_tmp->A_arr[ch] = x->xfade_func(x->state_tmp->U_cur[ch], x->xfade_param);
  }

  // Loop over the imput channels
  for (t_int32 inp = 0; inp < ch_cnt; inp++) {
    (channel + inp)->interp_func = x->xfade_func;
    (channel + inp)->interp_inv_func = x->xfade_inv_func;
    (channel + inp)->interp_param = x->xfade_param;
    _state_ramp(x, channel + inp, x->state_tmp, (t_int32)(time * x->msr), offset + inp);
  }
}

// ====  STATE_VELOCITY  ====

//******************************************************************************
//  Set the ramping velocity for a channel:
//  velocity (int: channel index) (float: velocity)
//
void state_velocity(t_diffuse* x, t_symbol* sym, t_int32 argc, t_atom* argv) {

  TRACE("state_velocity");

  // The method expects two arguments
  MY_ASSERT(argc != 2, "velocity:  2 args expected:  velocity (int: channel index) (float: velocity)");

  // Argument 0 should reference a channel
  t_channel* channel = _channel_find(x, argv);
  MY_ASSERT(!channel, "velocity:  Arg 0:  Channel not found.");

  // Argument 1 should be a float or int, for the velocity
  MY_ASSERT((atom_gettype(argv + 1) != A_FLOAT) && (atom_gettype(argv + 1) != A_LONG),
    "velocity:  Arg 1:  Positive float expected.");

  // The velocity should be positive
  t_double velocity = atom_getfloat(argv + 1);
  MY_ASSERT(velocity < 0, "velocity:  Arg 1:  Positive float expected.");

  channel->velocity = velocity;
}

// ====  STATE_VELOCITY_ALL  ====

//******************************************************************************
//  Set the ramping velocity for all channels:
//  velocity_all (float: velocity)
//
void state_velocity_all(t_diffuse* x, double velocity) {

  TRACE("state_velocity_all");

  // The velocity should be positive
  MY_ASSERT(velocity < 0, "velocity_all:  Arg 0:  Positive float expected.");

  // Set the velocity for all channels
  t_channel* channel = x->channel_arr + x->channel_cnt;
  do { channel--; channel->velocity = velocity; } while (channel != x->channel_arr);
}

// ====  STATE_FREEZE  ====

//******************************************************************************
//  Freeze or unfreeze a channel:
//  freeze (int: channel index) (int: 0 or 1)
//
void state_freeze(t_diffuse* x, t_symbol* sym, t_int32 argc, t_atom* argv) {

  TRACE("state_freeze");

  // The method expects two arguments
  MY_ASSERT(argc != 2, "freeze:  2 args expected:  freeze (int: channel index) (int: 0 or 1)");

  // Argument 0 should reference a channel
  t_channel* channel = _channel_find(x, argv);
  MY_ASSERT(!channel, "freeze:  Arg 0:  Channel not found.");

  // Argument 1 should be 0 or 1
  MY_ASSERT(atom_gettype(argv + 1) != A_LONG, "freeze:  Arg 1:  0 or 1 expected to freeze or unfreeze the state.");
  t_int32 is_frozen = (t_int32)(atom_getlong(argv + 1));
  MY_ASSERT((is_frozen != 0) && (is_frozen != 1), "freeze:  Arg 1:  0 or 1 expected to freeze or unfreeze the state.");

  channel->is_frozen = (is_frozen == 1) ? true : false;
}

// ====  STATE_FREEZE_ALL  ====

//******************************************************************************
//  Freeze or unfreeze all the channels:
//  freeze_all (int: 0 or 1)
//
void state_freeze_all(t_diffuse* x, long is_frozen) {

  TRACE("state_freeze_all");

  // Argument 0 should be 0 or 1
  MY_ASSERT((is_frozen != 0) && (is_frozen != 1), "freeze_all:  Arg 0:  0 or 1 expected to freeze or unfreeze the state.");

  // Freeze or unfreeze all the channels
  t_bool b = (is_frozen == 1) ? true : false;
  t_channel* channel = x->channel_arr + x->channel_cnt;
  do { channel--; channel->is_frozen = b; } while (channel != x->channel_arr);
}
