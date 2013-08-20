/**
 * Read data from files & fifos
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
#include <stdlib.h>
#include <sys/time.h>
#include <errno.h>

#include "protocols/MeterModbus.hpp"
#include "Options.hpp"
#include <VZException.hpp>
#include <inttypes.h>

#define DATA_BOOLEAN 0
#define DATA_WORD 1
#define DATA_SHORT 2
#define DATA_DWORD 3
#define DATA_LONG 4
#define DATA_FLOAT 5




MeterModbus::MeterModbus(std::list<Option> options)
		: Protocol("modbus")
{
	OptionList optlist;
	modbus_datatype datatype;
	
	try {
		_ip = optlist.lookup_string(options, "ip");
	} catch( vz::VZException &e ) {
		print(log_error, "Missing IP or invalid type", name().c_str());
		throw;
	}
	try {
		 _port = optlist.lookup_int(options, "port");
	} catch( vz::VZException &e ) {
		print(log_error, "Missing Port or invalid type", name().c_str());
		_port = MODBUS_TCP_DEFAULT_PORT;
	}
	try {
		_address = optlist.lookup_int(options, "address");
	} catch( vz::VZException &e ) {
		print(log_error, "Missing address or invalid type", name().c_str());
		throw;
	}
	try {
		_length = optlist.lookup_int(options, "length");
	} catch( vz::VZException &e ) {
		print(log_error, "Missing length or invalid type", name().c_str());
		throw;
	}
	/* use  modbus_read_input_registers by default*/
	try {
		_input_read = optlist.lookup_bool(options, "input_read");
	} catch( vz::OptionNotFoundException &e ) {
		_input_read = FALSE; /* use  modbus_read_registers by default*/
	}
	try {
		std::string type = optlist.lookup_string(options, "type");
		if(type == "bool")
			datatype = BOOL;
		else if( type == "word" )
			datatype = WORD;
		else if ( type == "short" )
			datatype = SHORT;
		else if ( type == "dword" )
			datatype = DWORD;
		else if ( type == "long" )
			datatype = LONG;
		else if ( type == "float" )
			datatype = FLOAT;
		else
			throw vz::VZException(type+"is invalid");
		
		_type = datatype;
		
	} catch( vz::VZException &e ) {
		print(log_error, "Missing type or invalid type", name().c_str());
		throw;
	}
	
	
}

MeterModbus::~MeterModbus() {
}

int MeterModbus::open() {
	
	_mb = modbus_new_tcp(ip(), _port);
	
	if (_mb == NULL) {
		print(log_error, "Unable to allocate libmodbus context: %s, %i", ip(), _port);
		return ERR;
	}
	print(log_debug, "Connecting to %s:%i", name().c_str(), _ip.c_str(), _port);
	if (modbus_connect(_mb) == -1) {
		print(log_error, "Connection failed: %s",name().c_str(), modbus_strerror(errno));
		modbus_free(_mb);
		return ERR;
	}

	return SUCCESS;
}

int MeterModbus::close() {

	modbus_close(_mb);
	modbus_free(_mb);
	return 0;
}

ssize_t MeterModbus::read(std::vector<Reading> &rds, size_t max_readings) {
	uint16_t in;
	int rc;
	rc = modbus_read_registers(_mb, _address, 1, &in);
	if (rc == -1) {
	    print(log_error, "Unable to fetch data: %i %s",name().c_str(),errno, modbus_strerror(errno));
	    return 0;
	}
	rds[0].value((double)in);
	rds[0].time();
	return 1;
}
