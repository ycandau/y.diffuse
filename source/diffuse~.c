/**
 @file
 diffuse~ - Diffuse a number of input channels to a number of output channels
 Yves Candau - ycandau@gmail.com
 
 @ingroup	myExternals
 */

// TO DO:
// Detect empty signal vectors and skip processing.
// Create safe and safe ramping functions (type checking... vs speed)


// ========  HEADER FILES  ========

#include "diffuse~.h"

// ========  GLOBAL CLASS POINTER AND STATIC VARIABLES  ========

static t_class *diffuse_class = NULL;

// ========  INITIALIZATION ROUTINE  ========

// ========  MAIN  ========

int C74_EXPORT main(void)
{
	t_class *c = class_new("diffuse~", (method)diffuse_new, (method)diffuse_free, (long)sizeof(t_diffuse), 0L, A_GIMME, 0);

	// ====  MAX MSP METHODS  ====

	class_addmethod(c, (method)diffuse_dsp64,		"dsp64",	A_CANT,	0);
	class_addmethod(c, (method)diffuse_assist,	"assist", A_CANT,	0);

	// ==== DIFFUSE METHODS ====

	class_addmethod(c, (method)diffuse_bang,			 "bang",								0);
	class_addmethod(c, (method)diffuse_dictionary, "dictionary", A_SYM,		0);
	class_addmethod(c, (method)diffuse_get,				 "get",									0);
	class_addmethod(c, (method)diffuse_master,		 "master",		 A_FLOAT, 0);
	class_addmethod(c, (method)diffuse_gain_out,	 "gain_out",	 A_GIMME, 0);
	class_addmethod(c, (method)diffuse_output,		 "output",		 A_SYM,		0);
	class_addmethod(c, (method)diffuse_set,				 "set",				 A_GIMME,	0);

	// ====  CHANNEL METHODS  ====

	class_addmethod(c, (method)channel_channel,		 "channel",		 A_GIMME, 0);
	class_addmethod(c, (method)channel_gain_in,		 "gain_in",		 A_GIMME, 0);
	class_addmethod(c, (method)channel_mute_ramp,	 "mute_ramp",	 A_GIMME, 0);

	// ====  STATE METHODS  ====

	class_addmethod(c, (method)state_state,				 "state",				 A_GIMME,	0);
	class_addmethod(c, (method)state_ramp_to,			 "ramp_to",			 A_GIMME,	0);
	class_addmethod(c, (method)state_ramp_between, "ramp_between", A_GIMME,	0);
	class_addmethod(c, (method)state_ramp_max,		 "ramp_max",		 A_GIMME,	0);
	class_addmethod(c, (method)state_circular,		 "circular",		 A_GIMME,	0);
	class_addmethod(c, (method)state_velocity,		 "velocity",		 A_GIMME,	0);
	class_addmethod(c, (method)state_velocity_all, "velocity_all", A_FLOAT,	0);
	class_addmethod(c, (method)state_freeze,			 "freeze",			 A_GIMME,	0);
	class_addmethod(c, (method)state_freeze_all,	 "freeze_all",	 A_LONG,	0);
	
	class_dspinit(c);
	class_register(CLASS_BOX, c);
	diffuse_class = c;
	
	return 0;
}

// ========  NEW INSTANCE ROUTINE: DIFFUSE_NEW  ========
/**
Called when the object is created.
Arguments:
  (int: input channels) (int: output channels) [int: storage slots]
*/
void *diffuse_new(t_symbol *sym, t_int32 argc, t_atom *argv)
{
	// ==== MAX initializations

	t_diffuse *x = NULL;
	x = (t_diffuse *)object_alloc(diffuse_class);
	
	if (x == NULL) {
		MY_ERR("Object allocation failed.");
		return NULL; }

	TRACE("diffuse_new");

	// ==== Arguments
	// (int: input channels) (int: output channels) [int: storage slots]

	// If two arguments are provided
	if ((argc == 2)
			&& (atom_gettype(argv) == A_LONG) && (atom_getlong(argv) >= 1)
			&& (atom_gettype(argv + 1) == A_LONG) && (atom_getlong(argv + 1) >= 1)) {

		x->channel_cnt = (t_int32)atom_getlong(argv);
		x->out_cnt		 = (t_int32)atom_getlong(argv + 1);
		x->state_cnt	 = STATE_CNT_DEF; }

	// If three arguments are provided
	else if ((argc == 3)
			&& (atom_gettype(argv) == A_LONG) && (atom_getlong(argv) >= 1)
			&& (atom_gettype(argv + 1) == A_LONG) && (atom_getlong(argv + 1) >= 1)
			&& (atom_gettype(argv + 2) == A_LONG) && (atom_getlong(argv + 2) >= 1)) {

		x->channel_cnt = (t_int32)atom_getlong(argv);
		x->out_cnt		 = (t_int32)atom_getlong(argv + 1);
		x->state_cnt	 = (t_int32)atom_getlong(argv + 2); }

	// Otherwise the arguments are invalid and the default values are used
	else {
		x->channel_cnt = CHANNEL_CNT_DEF;
		x->out_cnt		 = OUT_CNT_DEF;
		x->state_cnt	 = STATE_CNT_DEF;

		MY_ERR("diffuse_new:  Invalid arguments. The object expects:");
		MY_ERR2("  (int: input channels) (int: output channels) [int: storage slots]");
		MY_ERR2("    Arg 0:  Number of input channels. Default: %i", CHANNEL_CNT_DEF);
		MY_ERR2("    Arg 1:  Number of output channels. Default: %i", OUT_CNT_DEF);
		MY_ERR2("    Arg 2:  Optional:  Number of storage slots for states. Default: %i", STATE_CNT_DEF); }

	// ==== Inlets and oulets

	// Create the MSP inlets
	dsp_setup((t_pxobject *)x, x->channel_cnt);			

	// The last outlet is for messages
	x->outl_mess = outlet_new((t_object*)x, NULL);

	// Create the signal outlets
	for (t_int32 ch = 0; ch < x->out_cnt; ch++) {
		outlet_new((t_object *)x, "signal"); }

	// Separate the arrays for inlets and outlets
	x->obj.z_misc |= Z_NO_INPLACE;

	// ==== Initialization

	// Ramping parameter, function, and inverse function
	x->ramp_param = 4;
	x->ramp_func = ramp_exp;
	x->ramp_inv_func = ramp_exp_inv;

	// Crossfade parameter, function, and inverse function
	x->xfade_param = -3;
	x->xfade_func = xfade_sinus;
	x->xfade_inv_func = xfade_sinus_inv;

	// Set the array pointers to NULL
	x->channel_arr = NULL;
	x->state_arr = NULL;
	x->out_gain = NULL;
	x->outp_mess_arr = NULL;

	// Allocate the array of input channels and test
	x->channel_arr = (t_channel *)sysmem_newptr(sizeof(t_channel) * x->channel_cnt);
	if (!x->channel_arr) {
		MY_ERR("diffuse_new:  Allocation failed for the input channels.");
		diffuse_free(x);
		return NULL; }

	// Initialize and allocate each channel
	for (int ch = 0; ch < x->channel_cnt; ch++) { _channel_init(x, x->channel_arr + ch); }
	for (int ch = 0; ch < x->channel_cnt; ch++) {
		if (_channel_alloc(x, x->channel_arr + ch, 0, 0) != ERR_NONE) {
			MY_ERR("diffuse_new:  Allocation failed for the input channels.");
			diffuse_free(x);
			return NULL; } }
	
	// Allocate the array of states and test
	x->state_arr = _state_arr_new(x->state_cnt, &(x->state_cnt), x->out_cnt);
	if (!x->state_arr) {
		MY_ERR("diffuse_new:  Allocation failed for the array of states.");
		diffuse_free(x);
		return NULL; }

	// Initialize and allocate the temporary state used for calculations
	_state_init(x->state_tmp);
	if (_state_alloc(x->state_tmp, x->out_cnt, 0, 0) != ERR_NONE) {
		MY_ERR("diffuse_new:  Allocation failed for state_tmp.");
		diffuse_free(x);
		return NULL; }

	// Allocate the vector of gains and test
	x->out_gain = (t_double *)sysmem_newptr(sizeof(t_double)* x->out_cnt);
	if (!x->out_gain) {
		MY_ERR("diffuse_new:  Allocation failed for the output gains.");
		diffuse_free(x);
		return NULL; }

	// Amplitude variables
	x->master = 1.0;
	for (t_int32 ch = 0; ch < x->out_cnt; ch++) { x->out_gain[ch] = 1.0; }

	// Samplerates
	x->samplerate = sys_getsr();
	x->msr				= x->samplerate / 1000;

	// Variables for message output
	x->outp_channel = x->channel_arr;
	x->outp_type = OUTP_TYPE_DB;

	// Allocate the ouput message array
	x->outp_mess_arr = (t_atom *)sysmem_newptr(sizeof(t_atom) * x->out_cnt);
	if (!x->outp_mess_arr) {
		MY_ERR("diffuse_new:  Allocation failed for the output message array.");
		diffuse_free(x);
		return NULL; }
	
	// The name of the dictionary is empty for now
	x->dict_sym = gensym("");

	// Post a creation message
	POST("diffuse_new:  diffuse~ object created:");
	POST("  %i input channels, %i output channels, %i storage slots for states",
		x->channel_cnt, x->out_cnt, x->state_cnt);

	return (x);
}

// ========  METHOD: DIFFUSE_FREE  ========
// Called when the object is deleted

void diffuse_free(t_diffuse *x)
{
	TRACE("diffuse_free");
	
	if (x->channel_arr) {
		for (t_int32 ch = 0; ch < x->channel_cnt; ch++) {	_channel_free(x, x->channel_arr + ch); }
		sysmem_freeptr(x->channel_arr); }

	if (x->state_arr) {	_state_arr_free(&(x->state_arr), &(x->state_cnt)); }

	_state_free(x->state_tmp);

	if (x->out_gain) { sysmem_freeptr(x->out_gain); }
	if (x->outp_mess_arr) { sysmem_freeptr(x->outp_mess_arr); }

	dsp_free((t_pxobject *)x);
}

// ========  METHOD: DIFFUSE_DSP64  ========
// Called when the DAC is enabled

void diffuse_dsp64(t_diffuse *x, t_object *dsp64, t_int32 *count, t_double samplerate, long maxvectorsize, long flags)
{
	TRACE("diffuse_dsp64");
	POST("Samplerate = %.0f - Maxvectorsize = %i", samplerate, maxvectorsize);

	object_method(dsp64, gensym("dsp_add64"), x, diffuse_perform64, 0, NULL);

	// Recalculate everything that depends on the samplerate
	x->samplerate = samplerate;
	x->msr				= x->samplerate / 1000;
}

// ========  METHOD: DIFFUSE_PERFORM64  ========

void diffuse_perform64(t_diffuse *x, t_object *dsp64, t_double **in_arr, long numins, t_double **out_arr, long numouts, long sampleframes, long flags, void *userparam)
{
	// Set all the output vectors to zero
	for (t_int32 out = 0; out < x->out_cnt; out++) {
		for (t_int32 smp = 0; smp < sampleframes; smp++) {
			out_arr[out][smp] = 0; } }

	//  ####  LOOP THROUGH THE INPUT CHANNELS  ####

	for (t_int32 in = 0; in < x->channel_cnt; in++) {

		t_channel *channel = x->channel_arr + in;

		// If the channel is off don't do anything in this loop
		if (!channel->is_on) { continue; }
		
		// Local variables
		t_int32 chunk_len = -1;
		t_int32 smp_left = 0;
		t_int32 smp_proc = 0;
		t_int32 smp_left_x_vel = 0;
		t_int32 cntd_d_vel = 0;
		t_double gain = 0.0;
		t_double d_ampl = 0.0;
		t_double dA = 0.0;
		t_double A_U_dU = 0.0;

		t_double *sig_in = NULL;
		t_double *sig_out = NULL;

		// We are tracking where we are using:
		//   smp_left:      the number of samples left to process in this perform cycle
		//   chunk_len:     the number of samples to process in a chunk,
		//							    until end of perform cycle or end of countdown, whichever comes first, cannot be 0
		//   channel->cntd: the total number of sampleframes left to process (unscaled by the velocity)

		// ####  LOOP THROUGH THE CHUNKS  ####

		smp_left = sampleframes;
		while (smp_left) {

			// == Temporary variables - scaling by the velocity
			smp_left_x_vel = (t_int32)(smp_left * channel->velocity);
			cntd_d_vel = (t_int32)(channel->cntd / channel->velocity);					// cannot be 0, unless cntd is 0
			if ((cntd_d_vel == 0) && (channel->cntd != 0)) { cntd_d_vel = 1; }	// correct for rounding down to 0 when cntd is not 0

			// Keep track of the number of samples processed so far
			smp_proc = sampleframes - smp_left;

			// == Determine the chunk length and update the countdown and smp_left accordingly
			// == Five cases depending on the countdown
					
			// == If the bank is set to freeze
			// == process the whole audio vector with no ramping or countdown
			if (channel->is_frozen) { chunk_len = sampleframes; smp_left = 0; }

			// == Zero countdown:  Iterate the mode and skip this chunk loop
			else if (channel->cntd == 0) {
				_state_iterate(x, channel);
				continue; }

			// == Indefinite countdown:  The chunk is the whole length of the perform cycle
			else if (channel->cntd == INDEFINITE) { chunk_len = smp_left; smp_left = 0; }

			// == Countdown extends beyond perform cycle:  The chunk is the whole length of the perform cycle
			else if (channel->cntd > smp_left_x_vel) { chunk_len = smp_left; smp_left = 0; channel->cntd -= smp_left_x_vel; }
			// No velocity version:
			// else if (reson->cntd > smp_left) { chunk_len = smp_left; smp_left = 0; reson->cntd -= chunk_len; }

			// == Countdown shorter than perform cycle:  Keep processing chunks and mode changes
			else { chunk_len = cntd_d_vel; smp_left -= chunk_len; channel->cntd = 0; }		// smp_left never gets to -1 in spite of rounding
			// No velocity version:
			// else { chunk_len = reson->cntd; smp_left -= chunk_len; reson->cntd = 0; }

			// POST("smp_left: %i - smp_proc: %i - chunk_len: %i - cntd: %i", smp_left, smp_proc, chunk_len, channel->cntd);

			// ####  LOOP THROUGH THE OUTPUT CHANNELS  ####

			for (t_int32 out = 0; out < x->out_cnt; out++) {

				// Initialize the input and output pointers to the current position in the vectors
				sig_in = in_arr[in] + smp_proc;
				sig_out = out_arr[out] + smp_proc;

				// Calculate the non ramping gain: master, input channel and output channel
				gain = x->master * channel->gain * x->out_gain[out];
				
				// >>>>  IF THE CHANNEL IS FIXED, FROZEN OR INDEFINITE

				// == Add values without ramping
				if ((channel->mode_type == MODE_TYPE_FIX) || (channel->is_frozen) || (channel->cntd == INDEFINITE)) {

					// If one of the gains is 0 skip the sample loop
					// Could be from: master, gain input, output gain, or input-output multiplier
					if ((gain == 0) || (channel->A_cur[out] == 0)) { continue; }

					// ####  LOOP THROUGH THE SAMPLES  ####

					for (t_int32 smp = 0; smp < chunk_len; smp++) {

						*sig_out += *sig_in * channel->A_cur[out] * gain;
						sig_in++;  sig_out++; } }

				// >>>>  IF THE CHANNEL IS RAMPING

				// == Add values with ramping
				else if (channel->mode_type == MODE_TYPE_VAR) {

					// Calculate dA: linear ramping of amplitude over the chunk length

					// Increment the normalized ordinate value U by dU for the chunk length:
					// recalculate each chunk to avoid cumulative errors
					// alternative would be to calculate dU once when the ramp is created
					channel->U_cur[out] += chunk_len * (channel->U_targ[out] - channel->U_cur[out]) / cntd_d_vel;		// cntd_d_vel cannot be 0

					// Calculate A(U + dU): the target amplitude value at the end of the chunk length
					A_U_dU = channel->interp_func(channel->U_cur[out], channel->interp_param);

					// If one of the gains is 0 update A_cur, and skip the sample loop 
					// Could be from: master, gain input, output gain, or input to output multiplier
					if ((gain == 0) || ((channel->A_cur[out] == 0) && ((channel->A_targ[out] == 0)))) {
						channel->A_cur[out] = A_U_dU;	continue; }
					
					// Calculate dA
					dA = (A_U_dU - channel->A_cur[out]) / chunk_len;		// chunk_len cannot be 0

					// ####  LOOP THROUGH THE SAMPLES  ####
					
					for (t_int32 smp = 0; smp < chunk_len; smp++) {

						*sig_out += *sig_in * channel->A_cur[out] * gain;
						sig_in++;  sig_out++;
					
						// Increment to ramp the amplitude gain
						channel->A_cur[out] += dA; } } 

				// == OTHERWISE
				// == Post a message error
				else { MY_ERR("diffuse_perform64:  Invalid mode type."); }

			} // End the loop through the output channels
		}		// End the loop through the chunks
	}			// End the loop through the input channels

	// Send the output message
	if (x->outp_type != OUTP_TYPE_OFF) {
		atom_setdouble_array(x->out_cnt, x->outp_mess_arr, x->out_cnt, x->outp_channel->A_cur);
		outlet_anything(x->outl_mess, gensym("output"), x->out_cnt, x->outp_mess_arr); }
}

// ========  METHOD: DIFFUSE_ASSIST  ========

void diffuse_assist(t_diffuse *x, void *b, long msg, t_int32 arg, char *str)
{
	//TRACE("diffuse_assist");
	
	if (msg == ASSIST_INLET) {

		if (arg == 0) { sprintf(str, "Inlet %i: All purpose and Input Channel 0 (list / signal)", arg); }
		else if ((arg >= 1) && (arg < x->channel_cnt)) { sprintf(str, "Inlet %i: Input Channel %i (signal)", arg, arg); } }
	
	else if (msg == ASSIST_OUTLET) {

		if ((arg >= 0) && (arg < x->channel_cnt)) { sprintf(str, "Outlet %i: Output Channel %i (signal)", arg, arg); }
		else if (arg == x->channel_cnt) { sprintf(str, "Outlet %i: All purpose messages (list)", arg); } }
}

// ========  DIFFUSE METHODS  ========

// ====  DIFFUSE_BANG  ====

void diffuse_bang(t_diffuse *x)
{
	TRACE("bang");
}

// ====  DIFFUSE_DICTIONARY  ====

void diffuse_dictionary(t_diffuse *x, t_symbol *dict_sym)
{
	TRACE("dictionary");

	x->dict_sym = dict_dictionary(x, dict_sym);
}

// ====  DIFFUSE_GET  ====
/**
  diffuse (int: in count) (int: out count) (int: state count) (float: master)
		(float: out gain) {x N} (sym: dictionary)
*/
void diffuse_get(t_diffuse *x)
{
	TRACE("get");

	// Output a message with information about the object
	//   diffuse (int: index) (float: gain) {x N} (float: velocity) (float: gain) (sym: on/off) (sym: frozen/active)
	t_atom *mess_arr = (t_atom *)sysmem_newptr(sizeof(t_atom)* (x->out_cnt + 5));
	MY_ASSERT(!mess_arr, "get:  Allocation failed for the output message array.");
	t_atom *atom = mess_arr;

	atom_setlong(atom++, x->channel_cnt);
	atom_setlong(atom++, x->out_cnt);
	atom_setlong(atom++, x->state_cnt);
	atom_setfloat(atom++, x->master);
	for (t_int32 ch = 0; ch < x->out_cnt; ch++){ atom_setfloat(atom++, x->out_gain[ch]); }
	atom_setsym(atom++, x->dict_sym);

	outlet_anything(x->outl_mess, gensym("diffuse"), 5 + x->out_cnt, mess_arr);
	sysmem_freeptr(mess_arr);
}

// ====  DIFFUSE_MASTER  ====

/**
  master (float: master gain)
*/
void diffuse_master(t_diffuse *x, double master)
{
	TRACE("master");

	x->master = (t_double)master;
}

// ====  DIFFUSE_GAIN_OUT  ====

/**
  gain_out (int: output channel index) (float: channel gain)
*/
void diffuse_gain_out(t_diffuse *x, t_symbol *sym, t_int32 argc, t_atom *argv)
{
	TRACE("gain_out");

	MY_ASSERT(argc != 2, "gain_out:  2 args expected:  gain_out (int: output channel index) (float: channel gain)");

	// Argument 0 should reference an ouput channel
	MY_ASSERT(atom_gettype(argv) != A_LONG, "gain_out:  Arg 0:  Int [0-%i] expected: the index of the output channel.", x->out_cnt - 1);
	t_int32 index = (t_int32)atom_getlong(argv);
	MY_ASSERT((index < 0) || (index >= x->out_cnt), "gain_out:  Arg 0 : Int [0-%i] expected : the index of the output channel.", x->out_cnt - 1);

	// Argument 1 should be a float between 0 and 1
	MY_ASSERT((atom_gettype(argv + 1) != A_FLOAT) && (atom_gettype(argv + 1) != A_LONG),
		"gain_out:  Arg 1:  Float [0-1] expected: the gain of the output channel.");
	t_double gain = (t_double)atom_getfloat(argv + 1);
	MY_ASSERT(gain < 0, "gain_out:  Arg 1 : Positive float expected : the gain of the output channel.");

	x->out_gain[index] = gain;
}

// ====  DIFFUSE_OUTPUT  ====

void diffuse_output(t_diffuse *x, t_symbol *outp_type)
{
	TRACE("output");

	if (outp_type == gensym("off")) { x->outp_type = OUTP_TYPE_OFF; }
	else if (outp_type == gensym("db")) { x->outp_type = OUTP_TYPE_DB; }
	else if (outp_type == gensym("ampl")) { x->outp_type = OUTP_TYPE_AMPL; }
	else { MY_ERR("output:  Arg 0:   \"off\", \"db\" or \"ampl\" expected"); }
}

// ====  DIFFUSE_SET  ====

void diffuse_set(t_diffuse *x, t_symbol *sym, t_int32 argc, t_atom *argv)
{
	TRACE("set");

	// Argument 0 should be a command
	MY_ASSERT((argc < 1) || (atom_gettype(argv) != A_SYM),
		"set:  Arg 0:  Command expected: ramp / xfade.");
	t_symbol *cmd = atom_getsym(argv);

	// ====  RAMP:  Set the ramping function for all channels  ====
	// set ramp [sym: linear / poly / exp / sigmoid] [float: ramping parameter]

	if (cmd == gensym("ramp")) {

		// To set just the ramping parameter
		if ((argc == 2) && ((atom_gettype(argv + 1) == A_LONG) || (atom_gettype(argv + 1) == A_FLOAT))) {
			x->ramp_param = atom_getfloat(argv + 1); }

		else if (((argc == 2) && (atom_gettype(argv + 1) == A_SYM)) ||
			((argc == 3) &&
				(atom_gettype(argv + 1) == A_SYM) &&
				((atom_gettype(argv + 2) == A_LONG) || (atom_gettype(argv + 2) == A_FLOAT)))) {

			t_symbol *ramp_type = atom_getsym(argv + 1);
			
			if (ramp_type == gensym("linear")) {
				x->ramp_func = ramp_linear;
				x->ramp_inv_func = ramp_linear_inv; }

			else if (ramp_type == gensym("poly")) {
				x->ramp_func = ramp_poly;
				x->ramp_inv_func = ramp_poly_inv; }

			else if (ramp_type == gensym("exp")) {
				x->ramp_func = ramp_exp;
				x->ramp_inv_func = ramp_exp_inv; }

			else if (ramp_type == gensym("sigmoid")) {
				x->ramp_func = ramp_sigmoid;
				x->ramp_inv_func = ramp_sigmoid_inv; }

			else {
				MY_ASSERT(1, "set ramp:  Arg 1:  Ramp type expected: linear / poly / exp / sigmoid"); }
		
			if (argc == 3) {
				x->ramp_param = atom_getfloat(argv + 2); } }

		else {
			MY_ASSERT(1, "set ramp:  Expects:  set ramp [sym: linear / poly / exp / sigmoid] [float: ramping parameter]"); } }

	// ====  XFADE:  Set the crossfading function for all channels  ====
	// set xfade [sym: linear / sqrt / sinus] [float: crossfade parameter]

	else if (cmd == gensym("xfade")) {

		if ((argc == 2) && ((atom_gettype(argv + 1) == A_LONG) || (atom_gettype(argv + 1) == A_FLOAT))) {
			x->xfade_param = atom_getfloat(argv + 1); }

		else if (((argc == 2) && (atom_gettype(argv + 1) == A_SYM)) ||
			((argc == 3) &&
				(atom_gettype(argv + 1) == A_SYM) &&
				((atom_gettype(argv + 2) == A_LONG) || (atom_gettype(argv + 2) == A_FLOAT)))) {

			t_symbol *xfade_type = atom_getsym(argv + 1);
			
			if (xfade_type == gensym("linear")) {
				x->xfade_func = xfade_linear;
				x->xfade_inv_func = xfade_linear_inv; }

			else if (xfade_type == gensym("sqrt")) {
				x->xfade_func = xfade_sqrt;
				x->xfade_inv_func = xfade_sqrt_inv; }

			else if (xfade_type == gensym("sinus")) {
				x->xfade_func = xfade_sinus;
				x->xfade_inv_func = xfade_sinus_inv; }

			else {
				MY_ASSERT(1, "set xfade:  Arg 1:  Crossfade type expected: linear / sqrt / sinus"); }
		
			if (argc == 3) {
				x->xfade_param = atom_getfloat(argv + 2); } }

		else {
			MY_ASSERT(1, "set xfade:  Expects:  set xfade [sym: linear / sqrt / sinus] [float: crossfade parameter]"); } }

	else {
		MY_ASSERT(1, "set:  Arg 0:  Command expected: ramp / xfade."); }

	// Update the states
	for (t_int32 st = 0; st < x->state_cnt; st++) {
		_state_calc_absc(x, x->state_arr + st); }

	// Update the channels
	// XXX for (t_int32 ch = 0; ch < x->channel_cnt; ch++) {
	//	_channel_calc_absc(x, x->channel_arr); }
}

// ========  CHANNEL METHODS  ========

// ====  _CHANNEL_FIND  ====

/**
Look for a channel using an atom that contains an index.
Returns:
  A pointer to the channel
	NULL if no channel is found
*/
t_channel *_channel_find(t_diffuse *x, t_atom *argv)
{
	t_int32 index;

	// Test that the atom contains a valid int
	if ((atom_gettype(argv) == A_LONG)
			&& ((index = (t_int32)atom_getlong(argv)) >= 0)
			&& (index < x->channel_cnt)) {
		return (x->channel_arr + index); }

	// Otherwise return NULL
	else { return NULL; }
}

// ====  _CHANNEL_INIT  ====

/**
Initialize a channel. Call before _channel_alloc to set all array pointers to NULL.
*/
void _channel_init(t_diffuse *x, t_channel *channel)
{
	// Initialize the channel parameters
	channel->cntd = INDEFINITE;
	channel->velocity = 1.0;
	channel->gain = 1.0;

	channel->interp_func = x->xfade_func;
	channel->interp_inv_func = x->xfade_inv_func;
	channel->interp_param = x->xfade_param;

	channel->is_on = false;
	channel->is_frozen = false;
	channel->is_mute_ramp = false;

	channel->out_cnt = x->out_cnt;
	channel->state_ind = -1;

	channel->mode_type = MODE_TYPE_FIX;

	// Set the array pointers to NULL
	channel->U_cur = NULL;
	channel->A_cur = NULL;
	channel->U_targ = NULL;
	channel->A_targ = NULL;
}

// ====  _CHANNEL_ALLOC  ====

/**
Allocate arrays for a channel.
Call only after _channel_init to make sure the array pointers were set NULL.
Returns:
  ERR_NONE:  Succesful initialization
  ERR_ALLOC:  Failed allocation
*/
t_my_err _channel_alloc(t_diffuse *x, t_channel *channel, t_double u, t_double a)
{
	// Allocate the arrays
	channel->U_cur = (t_double*)sysmem_newptr(sizeof(t_double) * channel->out_cnt);
	channel->A_cur = (t_double*)sysmem_newptr(sizeof(t_double) * channel->out_cnt);
	channel->U_targ = (t_double*)sysmem_newptr(sizeof(t_double) * channel->out_cnt);
	channel->A_targ = (t_double*)sysmem_newptr(sizeof(t_double) * channel->out_cnt);

	// Test the allocations and free if one failed
	if (!channel->U_cur || !channel->A_cur || !channel->U_targ || !channel->A_targ) {
		_channel_free(x, channel);
		return ERR_ALLOC; }

	// Initialize the values in the arrays
	for (t_int32 param = 0; param < channel->out_cnt; param++) {
		channel->U_cur[param] = u;
		channel->A_cur[param] = a;
		channel->U_targ[param] = u;
		channel->A_targ[param] = a; }

	return ERR_NONE;
}

// ====  _CHANNEL_FREE  ====

/**
Free a channel.
*/
void _channel_free(t_diffuse *x, t_channel *channel)
{
	// Free the arrays and set to NULL
	if (channel->U_cur)  { sysmem_freeptr(channel->U_cur); channel->U_cur = NULL; }
	if (channel->A_cur)  { sysmem_freeptr(channel->A_cur); channel->A_cur = NULL; }
	if (channel->U_targ) { sysmem_freeptr(channel->U_targ); channel->U_targ = NULL; }
	if (channel->A_targ) { sysmem_freeptr(channel->A_targ); channel->A_targ = NULL; }
}

// ====  _CHANNEL_CALC_ABSC  ====

/**
Recalculate the abscissa values for a channel.
*/
void _channel_calc_absc(t_diffuse *x, t_channel *channel)
{
	for (t_int32 ch = 0; ch < channel->out_cnt; ch++) {
		channel->U_cur[ch] = channel->interp_inv_func(channel->A_cur[ch], channel->interp_param);
		channel->U_targ[ch] = channel->interp_inv_func(channel->A_targ[ch], channel->interp_param); }
}

// ====  CHANNEL_CHANNEL  ====

/**
Interface method to call:  set / get / post / on / off / current
*/
void channel_channel(t_diffuse *x, t_symbol *sym, t_int32 argc, t_atom *argv)
{
	TRACE("channel_channel");

	// Argument 0 should be a command
	MY_ASSERT((argc < 1) || (atom_gettype(argv) != A_SYM),
		"channel:  Arg 0:  Command expected: set / get / post / on / off / current.");
	t_symbol *cmd = atom_getsym(argv);

	// Test that the array of channels exists
	MY_ASSERT(!x->channel_arr, "channel:  No array of channels available.");
	
	// ====  SET:  Set the channel values  ====
	// channel set (int: channel index) (float: [0-1] gain) {x N}

	if (cmd == gensym("set")) {

		MY_ASSERT(argc != x->out_cnt + 2, "channel set:  %i args expected:  channel set (int: index) (float: [0-1] gain) {x %i}", x->out_cnt + 2, x->out_cnt);

		// Argument 1 should reference a channel
		t_channel *channel = _channel_find(x, argv + 1);
		MY_ASSERT(!channel, "channel set:  Arg 1:  Channel not found.");

		// Test that the following arguments are float and between 0 and 1
		for (t_int32 ch = 2; ch < x->out_cnt + 2; ch++) {
			MY_ASSERT((((atom_gettype(argv + ch) != A_FLOAT) && (atom_gettype(argv + ch) != A_LONG))
				|| (atom_getfloat(argv + ch) < 0) || (atom_getfloat(argv + ch) > 1)),
				"channel set:  Arg %i:  Float [0-1] expected for the gain.", ch); }

		// Set the channel values
		for (t_int32 ch = 0; ch < x->out_cnt; ch++) {
			channel->A_cur[ch] = atom_getfloat(argv + ch + 2); } }

	// ====  GET:  Get information on a channel as a message  ====
	// channel get (int: channel index)

	else if (cmd == gensym("get")) {

		MY_ASSERT(argc != 2, "channel get:  2 args expected:  channel get (int: channel index)");

		// Argument 1 should reference a channel
		t_channel *channel = _channel_find(x, argv + 1);
		MY_ASSERT(!channel, "channel get:  Arg 1:  Channel not found.");

		// Output a message with information about the channel
		//   channel (int: index) (float: gain) {x N} (float: velocity) (float: gain) (sym: on/off) (sym: frozen/active)
		t_atom *mess_arr = (t_atom *)sysmem_newptr(sizeof(t_atom) * (channel->out_cnt + 5));
		MY_ASSERT(!mess_arr, "channel get:  Allocation failed for the output message array.");
		t_atom *atom = mess_arr;

		atom_setlong(atom++, channel - x->channel_arr);
		for (t_int32 ch = 0; ch < channel->out_cnt; ch++){ atom_setfloat(atom++, channel->A_cur[ch]); }
		atom_setfloat(atom++, channel->velocity);
		atom_setfloat(atom++, channel->gain);
		atom_setsym(atom++, channel->is_on ? gensym("on") : gensym ("off"));
		atom_setsym(atom++, channel->is_frozen ? gensym("frozen") : gensym("active"));

		outlet_anything(x->outl_mess, gensym("channel"), 5 + channel->out_cnt, mess_arr);
		sysmem_freeptr(mess_arr); }

	// ====  POST:  Post information on the array of channels  ====
	// channel post (int: channel index / sym: all)

	else if (cmd == gensym("post")) {

		MY_ASSERT(argc != 2, "channel post:  2 args expected:  channel post (int: channel index / sym: all)");

		// If Arg 1 is a symbol and equal to "all"
		if ((atom_gettype(argv + 1) == A_SYM) && (atom_getsym(argv + 1) == gensym ("all"))) {

			// Post information on all the channels
			POST("There are %i input channels and %i output channels.  Master: %f", x->channel_cnt, x->out_cnt, x->master);
			t_channel *channel = NULL;

			for (t_int32 ch = 0; ch < x->channel_cnt; ch++) {
				channel = x->channel_arr + ch;
				POST("  Input %i:  Vel: %f - Gain: %f - Cntd: %i - %s - %s",
					ch, channel->velocity, channel->gain, channel->cntd, (channel->is_on ? gensym("on") : gensym("off"))->s_name,
					(channel->is_frozen ? gensym("frozen") : gensym("active"))->s_name); }

			for (t_int32 ch = 0; ch < x->channel_cnt; ch++) {
				POST("  Output %i:  Gain: %f", ch, x->out_gain[ch]); } }

		// ... If Arg 1 is an int
		else if (atom_gettype(argv + 1) == A_LONG) {

			// Find the channel
			t_channel *channel = _channel_find(x, argv + 1);
			MY_ASSERT(!channel, "channel post:  Arg 1:  Channel not found.");
			
			// Post detailed information on one channel
			POST("Input %i:  Velocity: %f - Gain: %f - %s - %s",
				channel - x->channel_arr, channel->velocity, channel->gain,
				(channel->is_on ? gensym("on") : gensym("off"))->s_name,
				(channel->is_frozen ? gensym("frozen") : gensym("active"))->s_name);

			for (t_int32 param = 0; param < channel->out_cnt; param++) {
				POST("  Value %i:  Current: U = %f - A = %f - Target: U = %f - A = %f",
					param, channel->U_cur[param], channel->A_cur[param], channel->U_targ[param], channel->A_targ[param]); } }
	
		// ... Otherwise the arguments are invalid
		else { MY_ASSERT(argc != 2, "channel post:  Invalid args:  channel post (int: channel index / sym: all)"); } }

	// ====  ON / OFF:  Set one or all of the arrays on or off  ====
	// channel (on / off) (int: channel index / sym: all)

	else if ((cmd == gensym("on")) || (cmd == gensym("off"))) {

		t_bool on_off = (cmd == gensym("on")) ? true : false;

		MY_ASSERT(argc != 2, "channel %s:  2 args expected:  channel (on / off) (int: channel index / sym: all)", cmd->s_name);

		// If Arg 1 is a symbol and equal to "all"
		if ((atom_gettype(argv + 1) == A_SYM) && (atom_getsym(argv + 1) == gensym("all"))) {

			// Set all the channels to on or off
			t_channel *channel = x->channel_arr + x->channel_cnt;
			do { channel--; channel->is_on = on_off; } while (channel != x->channel_arr); }

		// ... If Arg 1 is an int
		else if (atom_gettype(argv + 1) == A_LONG) {

			// Find the channel
			t_channel *channel = _channel_find(x, argv + 1);
			MY_ASSERT(!channel, "channel %s:  Arg 1:  Channel not found.", cmd->s_name);
			
			// Set the channel to on or off
			channel->is_on = on_off; } }

	// ====  CURRENT:  Set the current channel for output  ====
	// channel current (int: channel index)

	else if (cmd == gensym("current")) {

		MY_ASSERT(argc != 2, "channel current (int: channel index)");

		// Argument 1 should reference a channel
		t_channel *channel = _channel_find(x, argv + 1);
		MY_ASSERT(!channel, "channel current:  Arg 1:  Channel not found.");

		// Set the current channel for output
		x->outp_channel = channel; }

	// ====  Otherwise the command is invalid  ====

	else {
		MY_ERR("channel:  Arg 0:  Command expected: get / post / on / off / current."); }
}

// ====  CHANNEL_GAIN_IN  ====

/**
  gain_in (int: input channel index) (float: channel gain)
*/
void channel_gain_in(t_diffuse *x, t_symbol *sym, t_int32 argc, t_atom *argv)
{
	TRACE("gain_in");

	MY_ASSERT(argc != 2, "gain_in:  2 args expected:  gain_in (int: input channel index) (float: channel gain)");

	// Argument 0 should reference a channel
	t_channel *channel = _channel_find(x, argv);
	MY_ASSERT(!channel, "gain_in:  Arg 0:  Channel not found.");

	// Argument 1 should be a float between 0 and 1
	MY_ASSERT((atom_gettype(argv + 1) != A_FLOAT) && (atom_gettype(argv + 1) != A_LONG), "gain_in:  Arg 1:  Float [0-1] expected: the gain of the input channel.");
	t_double gain = (t_double)atom_getfloat(argv + 1);
	MY_ASSERT(gain < 0, "gain_in:  Arg 1:  Positive float expected: the gain of the input channel.");

	channel->gain = gain;
}

// ====  CHANNEL_MUTE_RAMP  ====

/**
  mute_ramp (int: channel index) (int: 0 or 1)
*/
void channel_mute_ramp(t_diffuse *x, t_symbol *sym, t_int32 argc, t_atom *argv)
{
	TRACE("mute_ramp");

	MY_ASSERT(argc != 2, "mute_ramp:  2 args expected:  mute_ramp (int: channel index) (int: 0 or 1)");

	// Argument 0 should reference a channel
	t_channel *channel = _channel_find(x, argv);
	MY_ASSERT(!channel, "mute_ramp:  Arg 0:  Channel not found.");

	// Argument 1 should be 0 and 1
	MY_ASSERT(atom_gettype(argv + 1) != A_LONG, "mute_ramp:  Arg 1:  0 or 1 expected to mute end or ramp messages.");
	t_int32 is_mute_ramp = (t_int32)atom_getlong(argv + 1);
	MY_ASSERT((is_mute_ramp != 0) && (is_mute_ramp != 1), "mute_ramp:  Arg 1:  0 or 1 expected to mute end or ramp messages.");

	channel->is_mute_ramp = is_mute_ramp;
}
