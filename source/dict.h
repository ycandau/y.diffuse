#ifndef YC_DICT_H_
#define YC_DICT_H_

// ========  HEADER FILES  ========

#include "max_util.h"
#include "ext_dictobj.h"

// ========  TYPEDEF  ========

//******************************************************************************
//  Function pointer types used for saving/loading a specific structure to/from a dictionary
//  void *:  A pointer to the structure to save
//  t_dictionary *:  A pointer to the dictionary holding the array of structures
//  t_symbol *:  The name under which to save the structure
//  t_symbol *:  A symbol to indicate the write protection status: "true" or "false"
//
typedef t_my_err (*t_dict_save) (void *, t_dictionary *, t_symbol *, t_symbol *);
typedef t_my_err (*t_dict_load) (t_dictionary *, void *);

// ====  DICT_DICTIONARY  ====

//******************************************************************************
//  Set the dictionary for an object.
//  Connect the dictionary's outlet to the object and bang the dictionary.
//  t_object* x:  The Max object
//  t_symbol* dict_sym:  The name of the dictionary as a symbol
//  Returns:
//  gensym("") if the dictionary is not found
//  dict_sym otherwise
//  Example:  Just add the following in the interface method:
//  x->dict_sym = dict_dictionary(x, dict_sym);
//
t_symbol* dict_dictionary(void* x, t_symbol* dict_sym);

// ====  DICT_SAVE_PROTECT  ====

//******************************************************************************
//  Save a structure into a sub-sub-dictionary, checking for write protection.
//  t_object* x:  The Max object
//  t_symbol* dict_root_sym:  Name of the root dictionary of the Max object
//  t_symbol* dict_sub_sym:  Name of the sub-dictionary corresponding to the array of structures
//  t_symbol* cmd_sym:  Save command, for posting
//  t_int32 offset:  Argument index offset due to length of save command
//  void* struct_ptr:  Pointer to the structure to save from
//  t_atom* argv_dict_sub_sub:  Atom holding the name of the sub-sub-dictionary to save the structure into
//  t_atom* argv_prot:  Atom holding an optional protection command: "protect", "override" or NULL
//  t_dict_save dict_save_func:  Specific function used to save a structure into a sub-sub-dictionary
//
//  Example:  dict_save_protect((t_object*)x, x->dict_sym, gensym("states"), gensym("state save"), 1, state, argv + 2, argv + 3, _state_dict_save);
//
t_my_err dict_save_protect(void* x, t_symbol* dict_root_sym, t_symbol* dict_sub_sym, t_symbol* cmd_sym, t_int32 offset,
  void* struct_ptr, t_atom* argv_dict_sub_sub, t_atom* argv_prot, t_dict_save dict_save_func);

// ====  DICT_SAVE  ====

//******************************************************************************
//  Save a structure into a sub-sub-dictionary, no write protection.
//  t_object* x:  The Max object
//  t_symbol* dict_root_sym:  Name of the root dictionary of the Max object
//  t_symbol* dict_sub_sym:  Name of the sub-dictionary corresponding to the array of structures
//  t_symbol* cmd_sym:  Save command, for posting
//  t_int32 offset:  Argument index offset due to length of save command
//  void* struct_ptr:  Pointer to the structure to save from
//  t_atom* argv_dict_sub_sub:  Atom holding the name of the sub-sub-dictionary to save the structure into
//  t_dict_save dict_save_func:  Specific function used to save a structure into a sub-sub-dictionary
//
//  Example:  dict_save((t_object*)x, x->dict_sym, gensym("states"), gensym("state save"), 1, state, argv + 2, _state_dict_save);
//
t_my_err dict_save(void* x, t_symbol* dict_root_sym, t_symbol* dict_sub_sym, t_symbol* cmd_sym, t_int32 offset,
  void* struct_ptr, t_atom* argv_dict_sub_sub, t_dict_save dict_save_func);

// ====  DICT_LOAD  ====

//******************************************************************************
//  Load a structure from a sub-sub-dictionary.
//  t_object* x:  The Max object
//  t_symbol* dict_root_sym:  Name of the root dictionary of the Max object
//  t_symbol* dict_sub_sym:  Name of the sub-dictionary corresponding to the array of structures
//  t_symbol* cmd_sym:  Load command, for posting
//  t_int32 offset:  Argument index offset due to length of load command
//  void* struct_ptr:  Pointer to the structure to load into
//  t_atom* argv_dict_sub_sub:  Atom holding the name of the sub-sub-dictionary to load from
//  t_dict_load dict_load_func:  Specific function used to load a dictionary into a structure
//
//  Example:  dict_load((t_object*)x, x->dict_sym, gensym("states"), gensym("state load"), 1, state, argv + 2, _state_dict_load);
//
t_my_err dict_load(void* x, t_symbol* dict_root_sym, t_symbol* dict_sub_sym, t_symbol* cmd_sym, t_int32 offset,
  void* struct_ptr, t_atom* argv_dict_sub_sub, t_dict_load dict_load_func);

// ====  DICT_DELETE_PROTECT  ====

//******************************************************************************
//  Delete a sub-sub-dictionary, checking for write protection.
//  t_object* x:  The Max object
//  t_symbol* dict_root_sym:  Name of the root dictionary of the Max object
//  t_symbol* dict_sub_sym:  Name of the sub-dictionary corresponding to the array of structures
//  t_symbol* cmd_sym:  Delete command, for posting
//  t_int32 offset:  Argument index offset due to length of delete command
//  t_atom* argv_dict_sub_sub:  Atom holding the name of the sub-sub-dictionary to delete
//  t_atom* argv_prot:  Atom holding an optional protection command: "override" or NULL
//
//  Example:  dict_delete_protect((t_object*)x, x->dict_sym, gensym("states"), gensym("state delete"), 1, argv + 2, argv + 3);
//
t_my_err dict_delete_protect(void* x, t_symbol* dict_root_sym, t_symbol* dict_sub_sym, t_symbol* cmd_sym, t_int32 offset,
  t_atom* argv_dict_sub_sub, t_atom* argv_prot);

// ====  DICT_DELETE  ====

//******************************************************************************
//  Delete a sub-sub-dictionary, no write protection.
//  t_object* x:  The Max object
//  t_symbol* dict_root_sym:  Name of the root dictionary of the Max object
//  t_symbol* dict_sub_sym:  Name of the sub-dictionary corresponding to the array of structures
//  t_symbol* cmd_sym:  Delete command, for posting
//  t_int32 offset:  Argument index offset due to length of delete command
//  t_atom* argv_dict_sub_sub:  Atom holding the name of the sub-sub-dictionary to delete
//
//  Example:  dict_delete((t_object*)x, x->dict_sym, gensym("states"), gensym("state delete"), 1, argv + 2);
//
t_my_err dict_delete(void* x, t_symbol* dict_root_sym, t_symbol* dict_sub_sym, t_symbol* cmd_sym, t_int32 offset,
  t_atom* argv_dict_sub_sub);

// ====  DICT_RENAME_PROTECT  ====

//******************************************************************************
//  Rename a sub-sub-dictionary, checking for write protection.
//  t_object* x:  The Max object
//  t_symbol* dict_root_sym:  Name of the root dictionary of the Max object
//  t_symbol* dict_sub_sym:  Name of the sub-dictionary corresponding to the array of structures
//  t_symbol* cmd_sym:  Rename command, for posting
//  t_int32 offset:  Argument index offset due to length of rename command
//  t_atom* argv_dict_sub_sub:  Atom holding the name of the sub-sub-dictionary to rename
//  t_atom* argv_prot:  Atom holding an optional protection command: "override" or NULL
//
//  Example:  dict_rename_protect((t_object*)x, x->dict_sym, gensym("states"), gensym("state rename"), 1, argv + 2, argv + 3);
//
t_my_err dict_rename_protect(void* x, t_symbol* dict_root_sym, t_symbol* dict_sub_sym, t_symbol* cmd_sym, t_int32 offset,
  t_atom* argv_dict_sub_sub, t_atom* argv_new_dict_sub_sub, t_atom* argv_prot);

// ====  DICT_RENAME  ====

//******************************************************************************
//  Delete a sub-sub-dictionary, no write protection.
//  t_object* x:  The Max object
//  t_symbol* dict_root_sym:  Name of the root dictionary of the Max object
//  t_symbol* dict_sub_sym:  Name of the sub-dictionary corresponding to the array of structures
//  t_symbol* cmd_sym:  Rename command, for posting
//  t_int32 offset:  Argument index offset due to length of Rename command
//  t_atom* argv_dict_sub_sub:  Atom holding the name of the sub-sub-dictionary to rename
//
//  Example:  dict_rename((t_object*)x, x->dict_sym, gensym("states"), gensym("state rename"), 1, argv + 2);
//
t_my_err dict_rename(void* x, t_symbol* dict_root_sym, t_symbol* dict_sub_sym, t_symbol* cmd_sym, t_int32 offset,
  t_atom* argv_dict_sub_sub, t_atom* argv_new_dict_sub_sub);

// ========  END OF HEADER FILE  ========

#endif
