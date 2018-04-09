#include "dict.h"

// ====  DICT_DICTIONARY  ====

/**
Set the dictionary for an object.
Connect the dictionary's outlet to the object and bang the dictionary.
t_object *x:  The Max object
t_symbol *dict_sym:  The name of the dictionary as a symbol
Returns:
gensym("") if the dictionary is not found
dict_sym otherwise
Example:  Just add the following in the interface method:
x->dict_sym = dict_dictionary(x, dict_sym);
*/
t_symbol *dict_dictionary(void *x, t_symbol *dict_sym)
{
	TRACE("dict_dictionary");
	
	t_dictionary *dict = dictobj_findregistered_retain(dict_sym);

	MY_ASSERT_RETURN(!dict, gensym(""), "dictionary:  Dictionary \"%s\" not found.", dict_sym->s_name);

	POST("dictionary:  Dictionary \"%s\" linked to the the object.", dict_sym->s_name);
	dictobj_release(dict);
	return dict_sym;
}

// ====  DICT_SAVE_PROTECT  ====

/**
Save a structure into a sub-sub-dictionary, checking for write protection.
  t_object *x:  The Max object
	t_symbol *dict_root_sym:  Name of the root dictionary of the Max object
	t_symbol *dict_sub_sym:  Name of the sub-dictionary corresponding to the array of structures
	t_symbol *cmd_sym:  Save command, for posting
	t_int32 offset:  Argument index offset due to length of save command
	void *struct_ptr:  Pointer to the structure to save from
	t_atom *argv_dict_sub_sub:  Atom holding the name of the sub-sub-dictionary to save the structure into
	t_atom *argv_prot:  Atom holding an optional protection command: "protect", "override" or NULL
	t_dict_save dict_save_func:  Specific function used to save a structure into a sub-sub-dictionary

	Example:  dict_save_protect((t_object *)x, x->dict_sym, gensym("states"), gensym("state save"), 1, state, argv + 2, argv + 3, _state_dict_save);
*/
t_my_err dict_save_protect(void *x, t_symbol *dict_root_sym, t_symbol *dict_sub_sym, t_symbol *cmd_sym, t_int32 offset,
	void *struct_ptr, t_atom *argv_dict_sub_sub, t_atom *argv_prot, t_dict_save dict_save_func)
{
	TRACE("dict_save_protect");

	// Check the argument which holds the name of the sub-sub-dictionary to save the structure into
	t_symbol *dict_sub_sub_sym = atom_getsym(argv_dict_sub_sub);
	MY_ASSERT_ERR(dict_sub_sub_sym == gensym(""), ERR_ARG1, "%s:  Arg %i:  Symbol expected: the name under which to save.", cmd_sym->s_name, offset + 1);

	// Check the argument which holds an optional protection command: "protect", "override"...
	t_symbol *prot_sym = gensym("");
	if (argv_prot) {
		prot_sym = atom_getsym(argv_prot);
		MY_ASSERT_ERR((prot_sym != gensym("protect")) && (prot_sym != gensym("override")), ERR_ARG2,
			"%s:  Arg %i:  Optional symbol expected:  \"protect\" / \"override\".", cmd_sym->s_name, offset + 2); }

	// ... or NULL when no protection argument is passed
	else { prot_sym = gensym("no_arg"); }

	// Get the root dictionary and check that it exists
	t_dictionary *dict_root = NULL;
	dict_root = dictobj_findregistered_retain(dict_root_sym);
	MY_ASSERT_ERR(!dict_root, ERR_DICT_NONE, "%s:  Root dictionary \"%s\" not found. Impossible to proceed.", cmd_sym->s_name, dict_root_sym->s_name);
	
	// Get the sub-dictionary corresponding to the array of structures
	t_dictionary *dict_sub = NULL;
	dictionary_getdictionary(dict_root, dict_sub_sym, (t_object **)&dict_sub);

	// If it does not exist create one and append it to the main dictionary
	// Do not free because ownership is passed on to the root dictionary
	if (!dict_sub) {
		dict_sub = dictionary_new();
		dictionary_appenddictionary(dict_root, dict_sub_sym, (t_object *)dict_sub);	}
	
	// Get the sub-sub-dictionary to save the structure into
	t_dictionary *dict_sub_sub = NULL;
	dictionary_getdictionary(dict_sub, dict_sub_sub_sym, (t_object **)&dict_sub_sub);

	// Consider the case where write protection will block the save:
	// there is no protection argument, the dictionary already exist...
	if ((prot_sym == gensym("no_arg")) && (dict_sub_sub)) {

		// Get the protection status from the sub-sub-dictionary: true or false
		t_symbol *get_prot = gensym("");
		dictionary_getsym(dict_sub_sub, gensym("protected"), &get_prot);
		
		// ... and the dictionary is protected
		if (get_prot == gensym("true")) {
			dictobj_release(dict_root);
			MY_ERR("%s:  Arg %i:  Unable to save due to write protection. Use \"protect\" or \"override\".", cmd_sym->s_name, offset + 2);
			return ERR_DICT_PROTECT; } }

	// Set the new protection status for the dictionary
	t_symbol *is_prot;
	if (prot_sym == gensym("protect")) { is_prot = gensym("true"); }
	else { is_prot = gensym("false"); }

	// Create a sub-sub-dictionary and append it by calling the specific save function for the structure
	t_my_err err = dict_save_func(struct_ptr, dict_sub, dict_sub_sub_sym, is_prot);
	switch (err) {
	case ERR_NONE: break;
	case ERR_ALLOC: MY_ERR("%s:  Allocation error in specific saving function.", cmd_sym->s_name); break;
	default: MY_ERR("%s:  Unknown error %i from specific saving function.", cmd_sym->s_name, err); break; }

	// Release the root dictionary and return the error value
	dictobj_release(dict_root);
	return err;
}

// ====  DICT_SAVE  ====

/**
Save a structure into a sub-sub-dictionary, no write protection.
t_object *x:  The Max object
t_symbol *dict_root_sym:  Name of the root dictionary of the Max object
t_symbol *dict_sub_sym:  Name of the sub-dictionary corresponding to the array of structures
t_symbol *cmd_sym:  Save command, for posting
t_int32 offset:  Argument index offset due to length of save command
void *struct_ptr:  Pointer to the structure to save from
t_atom *argv_dict_sub_sub:  Atom holding the name of the sub-sub-dictionary to save the structure into
t_dict_save dict_save_func:  Specific function used to save a structure into a sub-sub-dictionary

Example:  dict_save((t_object *)x, x->dict_sym, gensym("states"), gensym("state save"), 1, state, argv + 2, _state_dict_save);
*/
t_my_err dict_save(void *x, t_symbol *dict_root_sym, t_symbol *dict_sub_sym, t_symbol *cmd_sym, t_int32 offset,
	void *struct_ptr, t_atom *argv_dict_sub_sub, t_dict_save dict_save_func)
{
	TRACE("dict_save");

	// Check the argument which holds the name of the sub-sub-dictionary to save the structure into
	t_symbol *dict_sub_sub_sym = atom_getsym(argv_dict_sub_sub);
	MY_ASSERT_ERR(dict_sub_sub_sym == gensym(""), ERR_ARG1, "%s:  Arg %i:  Symbol expected: the name under which to save.", cmd_sym->s_name, offset + 1);

	// Get the root dictionary and check that it exists
	t_dictionary *dict_root = NULL;
	dict_root = dictobj_findregistered_retain(dict_root_sym);
	MY_ASSERT_ERR(!dict_root, ERR_DICT_NONE, "%s:  Root dictionary \"%s\" not found. Impossible to proceed.", cmd_sym->s_name, dict_root_sym->s_name);
	
	// Get the sub-dictionary corresponding to the array of structures
	t_dictionary *dict_sub = NULL;
	dictionary_getdictionary(dict_root, dict_sub_sym, (t_object **)&dict_sub);

	// If it does not exist create one and append it to the main dictionary
	// Do not free because ownership is passed on to the root dictionary
	if (!dict_sub) {
		dict_sub = dictionary_new();
		dictionary_appenddictionary(dict_root, dict_sub_sym, (t_object *)dict_sub);	}

	// Create a sub-sub-dictionary and append it by calling the specific save function for the structure
	t_my_err err = dict_save_func(struct_ptr, dict_sub, dict_sub_sub_sym, gensym(""));
	switch (err) {
	case ERR_NONE: break;
	case ERR_ALLOC: MY_ERR("%s:  Allocation error in specific saving function.", cmd_sym->s_name); break;
	default: MY_ERR("%s:  Unknown error %i from specific saving function.", cmd_sym->s_name, err); break; }

	// Release the root dictionary and return the error value
	dictobj_release(dict_root);
	return err;
}

// ====  DICT_LOAD  ====

/**
Load a structure from a sub-sub-dictionary.
t_object *x:  The Max object
t_symbol *dict_root_sym:  Name of the root dictionary of the Max object
t_symbol *dict_sub_sym:  Name of the sub-dictionary corresponding to the array of structures
t_symbol *cmd_sym:  Load command, for posting
t_int32 offset:  Argument index offset due to length of load command
void *struct_ptr:  Pointer to the structure to load into
t_atom *argv_dict_sub_sub:  Atom holding the name of the sub-sub-dictionary to load from
t_dict_load dict_load_func:  Specific function used to load a dictionary into a structure

Example:  dict_load((t_object *)x, x->dict_sym, gensym("states"), gensym("state load"), 1, state, argv + 2, _state_dict_load);
*/
t_my_err dict_load(void *x, t_symbol *dict_root_sym, t_symbol *dict_sub_sym, t_symbol *cmd_sym, t_int32 offset,
	void *struct_ptr, t_atom *argv_dict_sub_sub, t_dict_load dict_load_func)
{
	TRACE("dict_load");

	// Check the argument which holds the name of the sub-sub-dictionary to load into the structure
	t_symbol *dict_sub_sub_sym = atom_getsym(argv_dict_sub_sub);
	MY_ASSERT_ERR(dict_sub_sub_sym == gensym(""), ERR_ARG0,
		"%s:  Arg %i:  Symbol expected: the name of the subdictionary to load.", cmd_sym->s_name, offset);

	// Get the root dictionary and check that it exists
	t_dictionary *dict_root = NULL;
	dict_root = dictobj_findregistered_retain(dict_root_sym);
	MY_ASSERT_ERR(!dict_root, ERR_DICT_NONE,
		"%s:  Root dictionary \"%s\" not found. Impossible to proceed.", cmd_sym->s_name, dict_root_sym->s_name);

	// Get the sub-dictionary and check that it exists
	t_dictionary *dict_sub = NULL;
	dictionary_getdictionary(dict_root, dict_sub_sym, (t_object **)&dict_sub);
	if (!dict_sub) {
		dictobj_release(dict_root);
		MY_ERR("%s:  Subdictionary \"%s\" not found. Impossible to load from.", cmd_sym->s_name, dict_sub_sym->s_name);
		return ERR_DICT_NONE; }

	// Get the sub-sub-dictionary to load from and check that it exists
	t_dictionary *dict_sub_sub = NULL;
	dictionary_getdictionary(dict_sub, dict_sub_sub_sym, (t_object **)&dict_sub_sub);
	if (!dict_sub_sub) {
		dictobj_release(dict_root);
		MY_ERR("%s:  Dictionary \"%s\" not found. Impossible to load from.", cmd_sym->s_name, dict_sub_sub_sym->s_name);
		return ERR_DICT_NONE; }

	// Load from the sub-sub-dictionary by calling the specific load function for the structure
	t_my_err err = dict_load_func(dict_sub_sub, struct_ptr);
	switch (err) {
	case ERR_NONE: break;
	case ERR_ALLOC: MY_ERR("%s:  Allocation error in specific loading function.", cmd_sym->s_name, dict_sub_sub_sym->s_name); break;
	default: MY_ERR("%s:  Unknown error %i from specific loading function.", cmd_sym->s_name, err); break;	}

	// Release the root dictionary and return the error value
	dictobj_release(dict_root);
	return err;
}

// ====  DICT_DELETE_PROTECT  ====

/**
Delete a sub-sub-dictionary, checking for write protection.
t_object *x:  The Max object
t_symbol *dict_root_sym:  Name of the root dictionary of the Max object
t_symbol *dict_sub_sym:  Name of the sub-dictionary corresponding to the array of structures
t_symbol *cmd_sym:  Delete command, for posting
t_int32 offset:  Argument index offset due to length of delete command
t_atom *argv_dict_sub_sub:  Atom holding the name of the sub-sub-dictionary to delete
t_atom *argv_prot:  Atom holding an optional protection command: "override" or NULL

Example:  dict_delete_protect((t_object *)x, x->dict_sym, gensym("states"), gensym("state delete"), 1, argv + 2, argv + 3);
*/
t_my_err dict_delete_protect(void *x, t_symbol *dict_root_sym, t_symbol *dict_sub_sym, t_symbol *cmd_sym, t_int32 offset,
	t_atom *argv_dict_sub_sub, t_atom *argv_prot)
{
	TRACE("dict_delete_protect");

	// Check the argument which holds the name of the sub-sub-dictionary to delete
	t_symbol *dict_sub_sub_sym = atom_getsym(argv_dict_sub_sub);
	MY_ASSERT_ERR(dict_sub_sub_sym == gensym(""), ERR_ARG0,
		"%s:  Arg %i:  Symbol expected: the name of the subdictionary to delete.", cmd_sym->s_name, offset);

	// Check the argument which holds an optional protection command: "override"...
	t_symbol *prot_sym = gensym("");
	if (argv_prot) {
		prot_sym = atom_getsym(argv_prot);
		MY_ASSERT_ERR(prot_sym != gensym("override"), ERR_ARG1,
			"%s:  Arg %i:  Optional symbol expected:  \"override\".", cmd_sym->s_name, offset + 1); }

	// ... or NULL when no protection argument is passed
	else { prot_sym = gensym("no_arg"); }

	// Get the root dictionary and check that it exists
	t_dictionary *dict_root = NULL;
	dict_root = dictobj_findregistered_retain(dict_root_sym);
	MY_ASSERT_ERR(!dict_root, ERR_DICT_NONE,
		"%s:  Root dictionary \"%s\" not found. Impossible to proceed.", cmd_sym->s_name, dict_root_sym->s_name);
	
	// Get the sub-dictionary and check that it exists
	t_dictionary *dict_sub = NULL;
	dictionary_getdictionary(dict_root, dict_sub_sym, (t_object **)&dict_sub);
	if (!dict_sub) {
		dictobj_release(dict_root);
		MY_ERR("%s:  Subdictionary \"%s\" not found. Impossible to delete from.", cmd_sym->s_name, dict_sub_sym->s_name);
		return ERR_DICT_NONE; }

	// Get the sub-sub-dictionary to delete and check that it exists
	t_dictionary *dict_sub_sub = NULL;
	dictionary_getdictionary(dict_sub, dict_sub_sub_sym, (t_object **)&dict_sub_sub);
	if (!dict_sub_sub) {
		dictobj_release(dict_root);
		MY_ERR("%s:  Dictionary \"%s\" not found. Impossible to delete.", cmd_sym->s_name, dict_sub_sub_sym->s_name);
		return ERR_DICT_NONE; }

	// Consider the case where write protection will block the delete:
	// there is no protection argument, the dictionary already exist...
	if ((prot_sym == gensym("no_arg")) && (dict_sub_sub)) {

		// Get the protection status from the sub-sub-dictionary: true or false
		t_symbol *get_prot = gensym("");
		dictionary_getsym(dict_sub_sub, gensym("protected"), &get_prot);
		
		// ... and the dictionary is protected
		if (get_prot == gensym("true")) {
			dictobj_release(dict_root);
			MY_ERR("%s:  Arg %i:  Unable to delete due to write protection. Use \"override\".", cmd_sym->s_name, offset + 1);
			return ERR_DICT_PROTECT; } }

	// Delete the entry
	dictionary_deleteentry(dict_sub, dict_sub_sub_sym);

	// Release the root dictionary and return the error value
	dictobj_release(dict_root);
	return ERR_NONE;
}

// ====  DICT_DELETE  ====

/**
Delete a sub-sub-dictionary, no write protection.
t_object *x:  The Max object
t_symbol *dict_root_sym:  Name of the root dictionary of the Max object
t_symbol *dict_sub_sym:  Name of the sub-dictionary corresponding to the array of structures
t_symbol *cmd_sym:  Delete command, for posting
t_int32 offset:  Argument index offset due to length of delete command
t_atom *argv_dict_sub_sub:  Atom holding the name of the sub-sub-dictionary to delete

Example:  dict_delete((t_object *)x, x->dict_sym, gensym("states"), gensym("state delete"), 1, argv + 2);
*/
t_my_err dict_delete(void *x, t_symbol *dict_root_sym, t_symbol *dict_sub_sym, t_symbol *cmd_sym, t_int32 offset,
	t_atom *argv_dict_sub_sub)
{
	TRACE("dict_delete");

	// Check the argument which holds the name of the sub-sub-dictionary to delete
	t_symbol *dict_sub_sub_sym = atom_getsym(argv_dict_sub_sub);
	MY_ASSERT_ERR(dict_sub_sub_sym == gensym(""), ERR_ARG0,
		"%s:  Arg %i:  Symbol expected: the name of the subdictionary to delete.", cmd_sym->s_name, offset);

	// Get the root dictionary and check that it exists
	t_dictionary *dict_root = NULL;
	dict_root = dictobj_findregistered_retain(dict_root_sym);
	MY_ASSERT_ERR(!dict_root, ERR_DICT_NONE,
		"%s:  Root dictionary \"%s\" not found. Impossible to proceed.", cmd_sym->s_name, dict_root_sym->s_name);
	
	// Get the sub-dictionary and check that it exists
	t_dictionary *dict_sub = NULL;
	dictionary_getdictionary(dict_root, dict_sub_sym, (t_object **)&dict_sub);
	if (!dict_sub) {
		dictobj_release(dict_root);
		MY_ERR("%s:  Subdictionary \"%s\" not found. Impossible to delete from.", cmd_sym->s_name, dict_sub_sym->s_name);
		return ERR_DICT_NONE; }

	// Get the sub-sub-dictionary to delete and check that it exists
	t_dictionary *dict_sub_sub = NULL;
	dictionary_getdictionary(dict_sub, dict_sub_sub_sym, (t_object **)&dict_sub_sub);
	if (!dict_sub_sub) {
		dictobj_release(dict_root);
		MY_ERR("%s:  Dictionary \"%s\" not found. Impossible to delete.", cmd_sym->s_name, dict_sub_sub_sym->s_name);
		return ERR_DICT_NONE; }

	// Delete the entry
	dictionary_deleteentry(dict_sub, dict_sub_sub_sym);

	// Release the root dictionary and return the error value
	dictobj_release(dict_root);
	return ERR_NONE;
}

// ====  DICT_RENAME_PROTECT  ====

/**
Rename a sub-sub-dictionary, checking for write protection.
t_object *x:  The Max object
t_symbol *dict_root_sym:  Name of the root dictionary of the Max object
t_symbol *dict_sub_sym:  Name of the sub-dictionary corresponding to the array of structures
t_symbol *cmd_sym:  Rename command, for posting
t_int32 offset:  Argument index offset due to length of rename command
t_atom *argv_dict_sub_sub:  Atom holding the name of the sub-sub-dictionary to rename
t_atom *argv_prot:  Atom holding an optional protection command: "override" or NULL

Example:  dict_rename_protect((t_object *)x, x->dict_sym, gensym("states"), gensym("state rename"), 1, argv + 2, argv + 3);
*/
t_my_err dict_rename_protect(void *x, t_symbol *dict_root_sym, t_symbol *dict_sub_sym, t_symbol *cmd_sym, t_int32 offset,
	t_atom *argv_dict_sub_sub, t_atom *argv_new_dict_sub_sub, t_atom *argv_prot)
{
	TRACE("dict_rename_protect");

	// Check the argument which holds the name of the sub-sub-dictionary to rename
	t_symbol *dict_sub_sub_sym = atom_getsym(argv_dict_sub_sub);
	MY_ASSERT_ERR(dict_sub_sub_sym == gensym(""), ERR_ARG0,
		"%s:  Arg %i:  Symbol expected: the name of the subdictionary to rename.", cmd_sym->s_name, offset);

	// Check the argument which holds the new name for the sub-sub-dictionary to rename
	t_symbol *new_dict_sub_sub_sym = atom_getsym(argv_new_dict_sub_sub);
	MY_ASSERT_ERR(new_dict_sub_sub_sym == gensym(""), ERR_ARG1,
		"%s:  Arg %i:  Symbol expected: the new name for the subdictionary to rename.", cmd_sym->s_name, offset + 1);

	// Check the argument which holds an optional protection command: "override"...
	t_symbol *prot_sym = gensym("");
	if (argv_prot) {
		prot_sym = atom_getsym(argv_prot);
		MY_ASSERT_ERR(prot_sym != gensym("override"), ERR_ARG2,
			"%s:  Arg %i:  Optional symbol expected:  \"override\".", cmd_sym->s_name, offset + 2); }

	// ... or NULL when no protection argument is passed
	else { prot_sym = gensym("no_arg"); }

	// Get the root dictionary and check that it exists
	t_dictionary *dict_root = NULL;
	dict_root = dictobj_findregistered_retain(dict_root_sym);
	MY_ASSERT_ERR(!dict_root, ERR_DICT_NONE,
		"%s:  Root dictionary \"%s\" not found. Impossible to proceed.", cmd_sym->s_name, dict_root_sym->s_name);
	
	// Get the sub-dictionary and check that it exists
	t_dictionary *dict_sub = NULL;
	dictionary_getdictionary(dict_root, dict_sub_sym, (t_object **)&dict_sub);
	if (!dict_sub) {
		dictobj_release(dict_root);
		MY_ERR("%s:  Subdictionary \"%s\" not found. Impossible to rename.", cmd_sym->s_name, dict_sub_sym->s_name);
		return ERR_DICT_NONE; }

	// Get the sub-sub-dictionary to rename and check that it exists
	t_dictionary *dict_sub_sub = NULL;
	dictionary_getdictionary(dict_sub, dict_sub_sub_sym, (t_object **)&dict_sub_sub);
	if (!dict_sub_sub) {
		dictobj_release(dict_root);
		MY_ERR("%s:  Dictionary \"%s\" not found. Impossible to rename.", cmd_sym->s_name, dict_sub_sub_sym->s_name);
		return ERR_DICT_NONE; }

	// Consider the case where write protection will block the rename:
	// there is no protection argument, the dictionary already exist...
	if ((prot_sym == gensym("no_arg")) && (dict_sub_sub)) {

		// Get the protection status from the sub-sub-dictionary: true or false
		t_symbol *get_prot = gensym("");
		dictionary_getsym(dict_sub_sub, gensym("protected"), &get_prot);
		
		// ... and the dictionary is protected
		if (get_prot == gensym("true")) {
			dictobj_release(dict_root);
			MY_ERR("%s:  Arg %i:  Unable to rename due to write protection. Use \"override\".", cmd_sym->s_name, offset + 1);
			return ERR_DICT_PROTECT; } }

	// Chuck the entry and reappend it under a different key
	dictionary_chuckentry(dict_sub, dict_sub_sub_sym);
	dictionary_appenddictionary(dict_sub, new_dict_sub_sub_sym, (t_object *)dict_sub_sub);

	// Release the root dictionary and return the error value
	dictobj_release(dict_root);
	return ERR_NONE;
}

// ====  DICT_RENAME  ====

/**
Delete a sub-sub-dictionary, no write protection.
t_object *x:  The Max object
t_symbol *dict_root_sym:  Name of the root dictionary of the Max object
t_symbol *dict_sub_sym:  Name of the sub-dictionary corresponding to the array of structures
t_symbol *cmd_sym:  Rename command, for posting
t_int32 offset:  Argument index offset due to length of Rename command
t_atom *argv_dict_sub_sub:  Atom holding the name of the sub-sub-dictionary to rename

Example:  dict_rename((t_object *)x, x->dict_sym, gensym("states"), gensym("state rename"), 1, argv + 2);
*/
t_my_err dict_rename(void *x, t_symbol *dict_root_sym, t_symbol *dict_sub_sym, t_symbol *cmd_sym, t_int32 offset,
	t_atom *argv_dict_sub_sub, t_atom *argv_new_dict_sub_sub)
{
	TRACE("dict_rename");

	// Check the argument which holds the name of the sub-sub-dictionary to rename
	t_symbol *dict_sub_sub_sym = atom_getsym(argv_dict_sub_sub);
	MY_ASSERT_ERR(dict_sub_sub_sym == gensym(""), ERR_ARG0,
		"%s:  Arg %i:  Symbol expected: the name of the subdictionary to rename.", cmd_sym->s_name, offset);

	// Check the argument which holds the new name for the sub-sub-dictionary to rename
	t_symbol *new_dict_sub_sub_sym = atom_getsym(argv_new_dict_sub_sub);
	MY_ASSERT_ERR(new_dict_sub_sub_sym == gensym(""), ERR_ARG1,
		"%s:  Arg %i:  Symbol expected: the new name for the subdictionary to rename.", cmd_sym->s_name, offset + 1);

	// Get the root dictionary and check that it exists
	t_dictionary *dict_root = NULL;
	dict_root = dictobj_findregistered_retain(dict_root_sym);
	MY_ASSERT_ERR(!dict_root, ERR_DICT_NONE,
		"%s:  Root dictionary \"%s\" not found. Impossible to proceed.", cmd_sym->s_name, dict_root_sym->s_name);
	
	// Get the sub-dictionary and check that it exists
	t_dictionary *dict_sub = NULL;
	dictionary_getdictionary(dict_root, dict_sub_sym, (t_object **)&dict_sub);
	if (!dict_sub) {
		dictobj_release(dict_root);
		MY_ERR("%s:  Subdictionary \"%s\" not found. Impossible to rename.", cmd_sym->s_name, dict_sub_sym->s_name);
		return ERR_DICT_NONE; }

	// Get the sub-sub-dictionary to rename and check that it exists
	t_dictionary *dict_sub_sub = NULL;
	dictionary_getdictionary(dict_sub, dict_sub_sub_sym, (t_object **)&dict_sub_sub);
	if (!dict_sub_sub) {
		dictobj_release(dict_root);
		MY_ERR("%s:  Dictionary \"%s\" not found. Impossible to rename.", cmd_sym->s_name, dict_sub_sub_sym->s_name);
		return ERR_DICT_NONE; }

	// Chuck the entry and reappend it under a different key
	dictionary_chuckentry(dict_sub, dict_sub_sub_sym);
	dictionary_appenddictionary(dict_sub, new_dict_sub_sub_sym, (t_object *)dict_sub_sub);

	// Release the root dictionary and return the error value
	dictobj_release(dict_root);
	return ERR_NONE;
}
