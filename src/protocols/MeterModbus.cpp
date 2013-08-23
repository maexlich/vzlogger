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
#include <VZException.hpp>
#include <inttypes.h>




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
	/* use  modbus_read_registers by default*/
	try {
		_input_read = optlist.lookup_bool(options, "input_read");
	} catch( vz::OptionNotFoundException &e ) {
		_input_read = FALSE; /* use  modbus_read_registers by default*/
	}
	
	_addressparams = optlist.lookup_addressparams(options, "addresses");
	
	
	
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
	_reset_connection = false;
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
	
	if(_reset_connection) {
		int success;
		print(log_info, "Resetting Connection to %s because of error", name().c_str(), _ip.c_str());
		rc = open();
		if(rc == SUCCESS)
			_reset_connection = false;
		else
			return 0;
	}
	
	rc = modbus_read_registers(_mb, _address-1, 1, &in);
	if (rc == -1) {
	    print(log_error, "Unable to fetch data: %i %s", name().c_str(), errno, modbus_strerror(errno));
	    if(errno == 104 || errno == 32){
			close();
			_reset_connection = true;
		}
	    return 0;
	}
	
	
	rds[0].value((double)in);
	rds[0].time();
	rds[0].identifier(new AddressIdentifier(_address));
	return 1;
}
