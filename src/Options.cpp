/**
 * Option list functions
 *
 * @package vzlogger
 * @copyright Copyright (c) 2011, The volkszaehler.org project
 * @license http://www.gnu.org/licenses/gpl.txt GNU Public License
 * @author Steffen Vogel <info@steffenvogel.de>
 */
/*
 * This file is part of volkzaehler.org
 *
 * volkzaehler.org is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * volkzaehler.org is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with volkszaehler.org. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "Options.hpp"
#include <VZException.hpp>


Option::Option(const char *pKey)
		: _key(pKey)
{
	//_key = strdup(pKey);
}

Option::Option(const char *pKey, struct json_object *jso)
		: _key(pKey)
{
//std::cout<< "New option...."<< pKey << std::endl;
	int length;
	struct addressparam *param_ptr, mem_ptr;
	switch (json_object_get_type(jso)) {
			case json_type_string:	_value_string = json_object_get_string(jso);   break;
			case json_type_int:	    value.integer = json_object_get_int(jso);     break;
			case json_type_boolean:	value.boolean = json_object_get_boolean(jso); break;
			case json_type_double:	value.floating = json_object_get_double(jso); break;
			case json_type_array:
			if(!strcmp(pKey, "addresses"))
			{
				length = json_object_array_length(jso);
				//mem_ptr = malloc(sizeof(struct addressparam )*(length+1));
				//memset((void *)mem_ptr, 0, sizeof(struct addressparam)*(length+1));
				//param_ptr = (struct addressparam *)mem_ptr;
				param_ptr = new struct addressparam[length + 1];
				int i;
				for(i = 0; i< length; i++){
					struct json_object *cur_val;
					cur_val = json_object_array_get_idx(jso, i);
					param_ptr[i].function_code = (unsigned char)json_object_get_int(json_object_array_get_idx(cur_val,0));
					param_ptr[i].address = json_object_get_int(json_object_array_get_idx(cur_val,1));
					param_ptr[i].recalc_str = json_object_get_string(json_object_array_get_idx(cur_val,2));
					print(log_debug, "Added Function Code: %u, Address: %u, Recalc: %s", "Options", param_ptr->function_code, param_ptr->address, param_ptr->recalc_str);
				}
				param_ptr[length].function_code = 0xFF;
				value.addressparams = (struct addressparam *)param_ptr;
				break;
			}
			
			default:		throw vz::VZException("Not a valid Type");
	}

	_type = (type_t)json_object_get_type(jso);
}




Option::Option(const char *pKey, char *pValue)
		: _key(pKey)
		, _type(type_string)
		, _value_string(pValue)
{
//_key = strdup(pKey);
	//value.string = strdup(pValue);
}

Option::Option(const char *pKey, int pValue)
		: _key(pKey)
{
//_key = strdup(pKey);
	value.integer = pValue;
	_type = type_int;
}

Option::Option(const char *pKey, double pValue)
		: _key(pKey)
{
//_key = strdup(pKey);
	value.floating = pValue;
	_type = type_double;
}

Option::Option(const char *pKey, bool pValue)
		: _key(pKey)
{
//_key = strdup(pKey);
	value.boolean = pValue;
	_type = type_boolean;
}

Option::~Option() {
	/*if (_type == type_array)
		free((void *)value.addressparams);*/
//	if (_key != NULL) {
//		free(_key);
//	}

	//if (value.string != NULL && _type == type_string) {
	//	free((void*)(value.string));
	//}
}

Option::operator struct addressparam *() const {
	if (_type != type_array) throw vz::InvalidTypeException("Invalid type");

	return value.addressparams;
}


Option::operator const char *() const {
	if (_type != type_string) throw vz::InvalidTypeException("not a string");

	return _value_string.c_str();
}

Option::operator int() const {
	if (_type != type_int) throw vz::InvalidTypeException("Invalid type");

	return value.integer;
}

Option::operator double() const {
	if (_type != type_double) throw vz::InvalidTypeException("Invalid type");

	return value.floating;
}

Option::operator bool() const {
	if (_type != type_boolean) throw vz::InvalidTypeException("Invalid type");

	return value.boolean;
}

//Option& OptionList::lookup(List<Option> options, char *key) {
const Option &OptionList::lookup(std::list<Option> options, const std::string &key) {
	for(const_iterator it = options.begin(); it != options.end(); it++) {
		if ( it->key() == key ) {
			return (*it);
		}
	}

	throw vz::OptionNotFoundException("Option '"+ std::string(key) +"' not found");
}

const char *OptionList::lookup_string(std::list<Option> options, const char *key)
{
	Option opt = lookup(options, key);
	return (const char*)opt;
}

const int OptionList::lookup_int(std::list<Option> options, const char *key)
{
	Option opt = lookup(options, key);
	return (int)opt;
}

const bool OptionList::lookup_bool(std::list<Option> options, const char *key)
{
	Option opt = lookup(options, key);
	return (bool)opt;
}

const double OptionList::lookup_double(std::list<Option> options, const char *key)
{
	Option opt = lookup(options, key);
	return (double)opt;
}


const struct addressparam *OptionList::lookup_addressparams(std::list<Option> options, const char *key)
{
	Option opt = lookup(options, key);
	return (struct addressparam *)opt;
}

void OptionList::dump(std::list<Option> options) {
	std::cout<< "OptionList dump\n" ;

	for(const_iterator it = options.begin(); it != options.end(); it++) {
		std::cout << (*it) << std::endl;
	}
}
