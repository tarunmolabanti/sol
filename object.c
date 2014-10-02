#include "sol.h"

#include <stdlib.h>
#include <string.h>

sol_object_t *sol_cast_int(sol_state_t *state, sol_object_t *obj) {
	sol_object_t *res, *ls;
	if(sol_is_int(obj)) return obj;
	ls = sol_new_list(state);
	sol_list_insert(state, &ls, 0, obj);
	res = obj->ops->toint(state, ls);
	sol_obj_free(ls);
	sol_obj_free(obj);
	return res;
}

sol_object_t *sol_cast_float(sol_state_t *state, sol_object_t *obj) {
	sol_object_t *res, *ls;
	if(sol_is_float(obj)) return obj;
	ls = sol_new_list(state);
	sol_list_insert(state, &ls, 0, obj);
	res = obj->ops->tofloat(state, ls);
	sol_obj_free(ls);
	sol_obj_free(obj);
	return res;
}

sol_object_t *sol_cast_string(sol_state_t *state, sol_object_t *obj) {
	sol_object_t *res, *ls;
	if(sol_is_string(obj)) return obj;
	ls = sol_new_list(state);
	sol_list_insert(state, &ls, 0, obj);
	res = obj->ops->tostring(state, ls);
	sol_obj_free(ls);
	sol_obj_free(obj);
	return res;
}

// This will not fail here; error checking is done in sol_state_init().

sol_object_t *sol_new_singlet(sol_state_t *state) {
	sol_object_t *res = malloc(sizeof(sol_object_t));
	if(res) {
		res->type = SOL_SINGLET;
		res->refcnt = 0;
		res->ops = &(state->NullOps);
	}
	return sol_incref(res);
}

// And, now, for the rest of the checked stuff...

sol_object_t *sol_alloc_object(sol_state_t *state) {
	sol_object_t *res = malloc(sizeof(sol_object_t));
	if(!res) {
		sol_set_error(state, state->OutOfMemory);
		return sol_incref(state->None);
	}
	res->refcnt = 0;
	res->ops = &(state->NullOps);
	return sol_incref(res);
}

void sol_init_object(sol_state_t *state, sol_object_t *obj) {
	if(obj->ops->init) obj->ops->init(state, obj);
}

void sol_obj_free(sol_object_t *obj) {
	if(sol_decref(obj) <= 0) {
		sol_obj_release(obj);
	}
}

void sol_obj_release(sol_object_t *obj) {
    if(obj->ops->free) obj->ops->free(NULL, obj);
    free(obj);
}

sol_object_t *sol_new_int(sol_state_t *state, long i) {
	sol_object_t *res = sol_alloc_object(state);
	if(sol_has_error(state)) {
		sol_obj_free(res);
		return sol_incref(state->None);
	}
	res->type = SOL_INTEGER;
	res->ival = i;
	res->ops = &(state->IntOps);
	sol_init_object(state, res);
	return res;
}

sol_object_t *sol_new_float(sol_state_t *state, double f) {
	sol_object_t *res = sol_alloc_object(state);
	if(sol_has_error(state)) {
		sol_obj_free(res);
		return sol_incref(state->None);
	}
	res->type = SOL_FLOAT;
	res->fval = f;
	res->ops = &(state->FloatOps);
	sol_init_object(state, res);
	return res;
}

sol_object_t *sol_new_string(sol_state_t *state, const char *s) {
	sol_object_t *res = sol_alloc_object(state);
	if(sol_has_error(state)) {
		sol_obj_free(res);
		return sol_incref(state->None);
	}
	res->type = SOL_STRING;
	res->str = strdup(s);
	if(!res->str) {
		sol_obj_free(res);
		sol_set_error(state, state->OutOfMemory);
		return sol_incref(state->None);
	}
	res->ops = &(state->StringOps);
	sol_init_object(state, res);
	return res;
}

sol_object_t *sol_f_str_free(sol_state_t *state, sol_object_t *obj) {
	free(obj->str);
	return obj;
}

sol_object_t *sol_new_list(sol_state_t *state) {
	sol_object_t *res = sol_alloc_object(state);
	if(sol_has_error(state)) {
		sol_obj_free(res);
		return sol_incref(state->None);
	}
	res->type = SOL_LIST;
	res->lvalue = NULL;
	res->lnext = NULL;
	res->ops = &(state->ListOps);
	sol_init_object(state, res);
	return res;
}

int sol_list_len(sol_state_t *state, sol_object_t *list) {
	int i = 0;
	sol_object_t *cur = list;
	if(!sol_is_list(list)) {
		sol_obj_free(sol_set_error_string(state, "Compute length of non-list"));
		return -1;
	}
	while(cur && cur->lvalue) {
		i++;
		cur = cur->lnext;
	}
	return i;
}

sol_object_t *sol_list_sublist(sol_state_t *state, sol_object_t *list, int idx) {
	int i = 0;
	sol_object_t *cur = list;
	sol_object_t *copy;
	if(idx < 0) {
		return sol_set_error_string(state, "Create sublist at negative index");
	}
	while(cur && cur->lvalue && i < idx) {
		i++;
		cur = cur->lnext;
	}
	copy = sol_new_list(state);
	while(cur && cur->lvalue) {
		copy->lvalue = sol_incref(cur->lvalue);
		if(cur->lnext) {
			copy->lnext = sol_alloc_object(state);
			copy = copy->lnext;
			copy->type = SOL_LIST;
			copy->ops = &(state->ListOps);
		}
		cur = cur->lnext;
	}
	return copy;
}

sol_object_t *sol_list_get_index(sol_state_t *state, sol_object_t *list, int idx) {
	sol_object_t *cur = list;
	int i = 0;
	if(idx < 0) {
		return sol_set_error_string(state, "Get negative index");
	}
	while(cur && cur->lvalue && i < idx) {
		i++;
		cur = cur->lnext;
	}
	if(cur) {
		return sol_incref(cur->lvalue);
	} else {
		return sol_set_error_string(state, "Get out-of-bounds index");
	}
}

void sol_list_set_index(sol_state_t *state, sol_object_t *list, int idx, sol_object_t *obj) {
	sol_object_t *cur = list;
	int i = 0;
	if(idx < 0) {
		sol_obj_free(sol_set_error_string(state, "Set negative index"));
		return;
	}
	while(cur && cur->lvalue && i < idx) {
		i++;
		cur = cur->lnext;
	}
	if(cur) {
		sol_obj_free(cur->lvalue);
		cur->lvalue = sol_incref(obj);
	} else {
		sol_obj_free(sol_set_error_string(state, "Set out-of-bounds index"));
		return;
	}
}

void sol_list_insert(sol_state_t *state, sol_object_t **list, int idx, sol_object_t *obj) {
	sol_object_t *next = *list, *prev = NULL, *temp = sol_alloc_object(state);
	int i = 0;
	if(sol_has_error(state)) return;
	if(idx < 0) {
		sol_obj_free(sol_set_error_string(state, "Insert at negative index"));
		return;
	}
	temp->type = SOL_LIST;
	temp->ops = &(state->ListOps);
	temp->lvalue = sol_incref(obj);
	while(next && i < idx) {
		if(next->lvalue) i++;
		prev = next;
		next = next->lnext;
	}
	if(next) {
		temp->lnext = next;
		if(prev) {
			prev->lnext = temp;
		} else {
			free(*list);
			*list = temp;
			temp->lnext = next;
		}
	} else {
        if(prev) {
            prev->lnext = temp;
            temp->lnext = NULL;
        } else {
            sol_obj_free(temp->lvalue);
            sol_obj_free(temp);
            sol_obj_free(sol_set_error_string(state, "Out-of-bounds insert"));
            return;
        }
	}
}

sol_object_t *sol_list_remove(sol_state_t *state, sol_object_t **list, int idx) {
	sol_object_t *next = *list, *prev = NULL, *res, *temp;
	int i = 0;
	if(sol_has_error(state)) return sol_incref(state->None);
	if(idx < 0) {
		return sol_set_error_string(state, "Remove from negative index");
	}
	while(next && i < idx) {
		if(next->lvalue) i++;
		prev = next;
		next = next->lnext;
	}
	if(next) {
		if(prev) {
            res = next->lvalue;
			prev->lnext = next->lnext;
			free(next);
		} else {
            res = (*list)->lvalue;
			temp = *list;
			*list = (*list)->lnext;
			free(temp);
		}
		return res;
	} else {
		return sol_set_error_string(state, "Out-of-bounds remove");
	}
}

sol_object_t *sol_list_copy(sol_state_t *state, sol_object_t *list) {
	sol_object_t *newls = sol_new_list(state), *cur = list;
	sol_object_t *res = newls;
	while(cur->lvalue) {
		newls->lvalue = sol_incref(cur->lvalue);
		if(cur->lnext) {
			newls->lnext = sol_alloc_object(state);
			if(sol_has_error(state)) return sol_incref(state->None);
			newls = newls->lnext;
			newls->type = SOL_LIST;
			newls->ops = &(state->ListOps);
		}
		cur = cur->lnext;
	}
	return res;
}

void sol_list_append(sol_state_t *state, sol_object_t *dest, sol_object_t *src) {
    sol_object_t *curd = dest, *curs = src;
    while(curd->lnext) curd = curd->lnext;
    while(curs && curs->lvalue) {
        curd->lnext = sol_alloc_object(state);
        if(sol_has_error(state)) return;
        curd = curd->lnext;
        curd->type = SOL_LIST;
        curd->ops = &(state->ListOps);
        curd->lvalue = sol_incref(curs->lvalue);
        curs = curs->lnext;
    }
}

sol_object_t *sol_f_list_free(sol_state_t *state, sol_object_t *list) {
    sol_object_t *cur = list, *prev;
    while(cur) {
        if(cur->lvalue) sol_obj_free(cur->lvalue);
        prev = cur;
        cur = cur->lnext;
        if(prev!=list) free(prev);
    }
}

sol_object_t *sol_new_map(sol_state_t *state) {
    sol_object_t *map = sol_alloc_object(state);
    if(sol_has_error(state)) return sol_incref(state->None);
    map->type = SOL_MAP;
    map->ops = &(state->MapOps);
    map->mkey = NULL;
    map->mval = NULL;
    map->mnext = NULL;
}

int sol_map_len(sol_state_t *state, sol_object_t *map) {
    int i = 0;
    sol_object_t *cur = map;
    while(cur) {
        if(cur->mkey) i++;
        cur = cur->mnext;
    }
    return i;
}

sol_object_t *sol_map_submap(sol_state_t *state, sol_object_t *map, sol_object_t *key) {
    sol_object_t *list = sol_new_list(state), *res = NULL, *cur = map, *cmp;
    sol_list_insert(state, &list, 0, key);
    while(cur) {
        if(cur->mkey) {
            sol_list_insert(state, &list, 1, cur->mkey);
            cmp = sol_cast_int(state, key->ops->cmp(state, list));
            sol_obj_free(sol_list_remove(state, &list, 1));
            if(cmp->ival) {
                res = cur;
                break;
            }
        }
        cur = cur->mnext;
    }
    if(res) {
        return sol_incref(res);
    } else {
        return sol_incref(state->None);
    }
}

sol_object_t *sol_map_get(sol_state_t *state, sol_object_t *map, sol_object_t *key) {
    sol_object_t *submap = sol_map_submap(state, map, key);
    sol_object_t *res;
    if(sol_is_map(submap)) {
        res = sol_incref(submap->mval);
        sol_obj_free(submap);
        return res;
    } else {
        return sol_incref(state->None);
    }
}

void sol_map_set(sol_state_t *state, sol_object_t *map, sol_object_t *key, sol_object_t *val) {
    sol_object_t *cur = map, *prev = NULL, *list = sol_new_list(state), *cmp;
    sol_list_insert(state, &list, 0, key);
    while(cur) {
        if(cur->mkey) {
            sol_list_insert(state, &list, 1, cur->mkey);
            cmp = sol_cast_int(state, key->ops->cmp(state, list));
            sol_obj_free(sol_list_remove(state, &list, 1));
            if(cmp->ival) {
                sol_obj_free(cur->mval);
                if(sol_is_none(state, val)) {
                    if(prev) {
                        prev->mnext = cur->mnext;
                        sol_obj_free(cur->mkey);
                        sol_obj_free(cur->mval);
                        sol_obj_free(cur);
                    } else {
                        sol_obj_free(cur->mkey);
                        sol_obj_free(cur->mval);
                        cur->mkey = NULL;
                        cur->mval = NULL;
                    }
                }
                cur->mval = sol_incref(val);
                return;
            }
        }
        prev = cur;
        cur = cur->mnext;
    }
    if(sol_is_none(state, val)) return;
    prev->mnext = sol_alloc_object(state);
    if(sol_has_error(state)) return;
    cur = prev->mnext;
    cur->type = SOL_MAP;
    cur->ops = &(state->MapOps);
    cur->mkey = sol_incref(key);
    cur->mval = sol_incref(val);
    cur->mnext = NULL;
}

void sol_map_set_existing(sol_state_t *state, sol_object_t *map, sol_object_t *key, sol_object_t *val) {
    sol_object_t *cur = map, *prev = NULL, *list = sol_new_list(state), *cmp;
    sol_list_insert(state, &list, 0, key);
    while(cur) {
        if(cur->mkey) {
            sol_list_insert(state, &list, 1, cur->mkey);
            cmp = sol_cast_int(state, key->ops->cmp(state, list));
            sol_obj_free(sol_list_remove(state, &list, 1));
            if(cmp->ival) {
                sol_obj_free(cur->mval);
                if(sol_is_none(state, val)) {
                    if(prev) {
                        prev->mnext = cur->mnext;
                        sol_obj_free(cur->mkey);
                        sol_obj_free(cur->mval);
                        sol_obj_free(cur);
                    } else {
                        sol_obj_free(cur->mkey);
                        sol_obj_free(cur->mval);
                        cur->mkey = NULL;
                        cur->mval = NULL;
                    }
                }
                cur->mval = sol_incref(val);
                return;
            }
        }
        prev = cur;
        cur = cur->mnext;
    }
}

sol_object_t *sol_map_copy(sol_state_t *state, sol_object_t *map) {
    sol_object_t *newmap = sol_new_map(state), *newcur = newmap, *cur = map;
    while(cur) {
        if(cur->mkey) {
            newcur->mkey = sol_incref(cur->mkey);
            newcur->mval = sol_incref(cur->mval);
            newcur->mnext = sol_alloc_object(state);
            newcur = newcur->mnext;
            newcur->type = SOL_MAP;
            newcur->ops = &(state->MapOps);
            newcur->mkey = NULL;
            newcur->mval = NULL;
            newcur->mnext = NULL;
        }
        cur = cur->mnext;
    }
    return newmap;
}

void sol_map_merge(sol_state_t *state, sol_object_t *dest, sol_object_t *src) {
    sol_object_t *cur = src;
    while(cur) {
        if(cur->mkey) {
            sol_map_set(state, dest, cur->mkey, cur->mval);
        }
        cur = cur->mnext;
    }
}

void sol_map_merge_existing(sol_state_t *state, sol_object_t *dest, sol_object_t *src) {
    sol_object_t *cur = src;
    while(cur) {
        if(cur->mkey) {
            sol_map_set_existing(state, dest, cur->mkey, cur->mval);
        }
        cur = cur->mnext;
    }
}

sol_object_t *sol_f_map_free(sol_state_t *state, sol_object_t *map) {
    sol_object_t *cur = map, *prev;
    while(cur) {
        if(cur->mkey) {
            sol_obj_free(cur->mkey);
            sol_obj_free(cur->mval);
        }
        prev = cur;
        cur = cur->mnext;
        if(prev!=map) free(prev);
    }
    return map;
}

sol_object_t *sol_new_cfunc(sol_state_t *state, sol_cfunc_t cfunc) {
    sol_object_t *res = sol_alloc_object(state);
    res->type = SOL_CFUNCTION;
    res->ops = &(state->CFuncOps);
    res->cfunc = cfunc;
    return res;
}

sol_object_t *sol_new_cdata(sol_state_t *state, void *cdata, sol_ops_t *ops) {
    sol_object_t *res = sol_alloc_object(state);
    res->type = SOL_CDATA;
    res->ops = ops;
    res->cdata = cdata;
    return res;
}
